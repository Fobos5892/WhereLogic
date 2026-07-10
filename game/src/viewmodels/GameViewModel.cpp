#include "GameViewModel.h"

#include "../core/GameConstants.h"
#include "../core/RoundRevealPolicy.h"
#include "../models/NetworkServer.h"

#include "RestApiConstants.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QHash>
#include <QRegularExpression>
#include <QCoreApplication>

#include <algorithm>

namespace {

constexpr int kTimerTickMs = 100;
constexpr int kGracePeriodSeconds = 10;
const QString kHintDelimiter = QStringLiteral("||");
const QLatin1String kHybridAnimPrefix("__hybrid_anim:");

QString parseHybridAnimStyle(const QString &correctOrderJson)
{
    const QJsonDocument doc = QJsonDocument::fromJson(correctOrderJson.toUtf8());
    if (!doc.isArray()) {
        return QStringLiteral("soft");
    }
    for (const QJsonValue &value : doc.array()) {
        const QString token = value.toString().trimmed();
        if (!token.startsWith(kHybridAnimPrefix)) {
            continue;
        }
        const QString raw = token.mid(kHybridAnimPrefix.size()).trimmed().toLower();
        return raw == QStringLiteral("aggressive") ? QStringLiteral("aggressive")
                                                   : QStringLiteral("soft");
    }
    return QStringLiteral("soft");
}

} // namespace

GameViewModel::GameViewModel(DatabaseManager *database, QObject *parent)
    : QObject(parent)
    , m_database(database)
{
    m_gameTimer.setInterval(kTimerTickMs);
    connect(&m_gameTimer, &QTimer::timeout, this, &GameViewModel::onTimerTick);

    m_flashTimer.setSingleShot(true);
    m_flashTimer.setInterval(800);
    connect(&m_flashTimer, &QTimer::timeout, this, [this]() {
        m_isCorrectFlash = false;
        m_isWrongFlash = false;
        emit evaluationFlashChanged();
    });

    loadPersistedState();

    if (QCoreApplication *app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this, [this]() { persistState(); });
    }
}

GameViewModel::~GameViewModel()
{
    stopTimer();
    persistState();
}

void GameViewModel::setNetworkServer(NetworkServer *server)
{
    if (m_networkServer == server) {
        return;
    }

    if (m_networkServer) {
        disconnect(m_networkServer, nullptr, this, nullptr);
    }

    m_networkServer = server;
    if (!m_networkServer) {
        return;
    }

    connect(m_networkServer, &NetworkServer::remoteConnectedChanged, this, &GameViewModel::onRemoteConnectedChanged);
    onRemoteConnectedChanged(m_networkServer->isRemoteConnected());
}

void GameViewModel::setUserAnswer(const QString &answer)
{
    if (m_userAnswer == answer) {
        return;
    }
    m_userAnswer = answer;
    emit userAnswerChanged();
}

QString GameViewModel::label(const QString &key) const
{
    if (!m_database) {
        return key;
    }
    return m_database->localizedString(key);
}

QString GameViewModel::puzzleImageUrl(int slotIndex) const
{
    if (m_state.puzzleId <= 0) {
        return {};
    }
    return QStringLiteral("image://puzzle/%1/%2?rev=%3")
        .arg(m_state.puzzleId)
        .arg(slotIndex)
        .arg(m_puzzleImageRevision);
}

QString GameViewModel::puzzleHiddenImageUrl() const
{
    if (m_state.puzzleId <= 0 || m_puzzleMasks.isEmpty()) {
        return {};
    }
    const QString suffix = hiddenImageUrlSuffix();
    if (suffix.isEmpty()) {
        return QStringLiteral("image://puzzle/%1/hide?rev=%2")
            .arg(m_state.puzzleId)
            .arg(m_puzzleImageRevision);
    }
    return QStringLiteral("image://puzzle/%1/hide/%2?rev=%3")
        .arg(m_state.puzzleId)
        .arg(suffix)
        .arg(m_puzzleImageRevision);
}

QString GameViewModel::hiddenImageUrlSuffix() const
{
    if (m_revealedMaskNumbers.isEmpty()) {
        return {};
    }
    QList<int> numbers = m_revealedMaskNumbers;
    std::sort(numbers.begin(), numbers.end());
    QStringList parts;
    parts.reserve(numbers.size());
    for (int number : numbers) {
        parts.append(QString::number(number));
    }
    return parts.join(QLatin1Char(','));
}

QStringList GameViewModel::visibleHints() const
{
    const int count = std::clamp(m_revealedHints, 0, static_cast<int>(m_puzzleHints.size()));
    return m_puzzleHints.mid(0, count);
}

bool GameViewModel::canRevealHint() const
{
    const bool gameplayStage = m_stage == GameConstants::Stage::MainTurn
                               || m_stage == GameConstants::Stage::StealTurn;
    return gameplayStage && m_cardsFaceUp && m_hintUnlockReady && remainingHints() > 0;
}

int GameViewModel::remainingHints() const
{
    return std::max(0, static_cast<int>(m_puzzleHints.size()) - m_revealedHints);
}

void GameViewModel::parsePackedHints(const QString &packedHint)
{
    m_puzzleHints.clear();
    const QStringList parts = packedHint.split(kHintDelimiter, Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            m_puzzleHints.append(trimmed);
        }
    }
    while (m_puzzleHints.size() > 3) {
        m_puzzleHints.removeLast();
    }
    m_hintText = m_puzzleHints.isEmpty() ? QString() : m_puzzleHints.first();
}

void GameViewModel::updateHintUnlockState()
{
    const int initialSeconds = m_isStealTurn ? m_stealTimerSeconds : m_baseTimerSeconds;
    const int elapsed = std::max(0, initialSeconds - m_timerSeconds);
    const bool unlock = elapsed >= 30;
    if (m_hintUnlockReady == unlock) {
        return;
    }
    m_hintUnlockReady = unlock;
    emit hintsChanged();
}

bool GameViewModel::allMasksRevealed() const
{
    return allMaskGroupsRevealed();
}

bool GameViewModel::allMaskGroupsRevealed() const
{
    if (m_puzzleMasks.isEmpty()) {
        return false;
    }
    for (int i = 0; i < m_answerGroups.size(); ++i) {
        if (!isGroupRevealed(i)) {
            return false;
        }
    }
    return !m_answerGroups.isEmpty();
}

void GameViewModel::buildAnswerGroups()
{
    m_answerGroups.clear();
    QHash<QString, int> indexByKey;
    for (const PuzzleMaskInfo &mask : m_puzzleMasks) {
        const QString key = normalizeAnswer(mask.answerText);
        if (key.isEmpty()) {
            continue;
        }
        if (!indexByKey.contains(key)) {
            MaskAnswerGroup group;
            group.answerText = mask.answerText.trimmed();
            m_answerGroups.append(group);
            indexByKey.insert(key, m_answerGroups.size() - 1);
        }
        m_answerGroups[indexByKey.value(key)].maskNumbers.append(mask.sortOrder);
    }

    for (MaskAnswerGroup &group : m_answerGroups) {
        std::sort(group.maskNumbers.begin(), group.maskNumbers.end());
    }
}

int GameViewModel::findAnswerGroupIndex(const QString &answer) const
{
    const QString key = normalizeAnswer(answer);
    for (int i = 0; i < m_answerGroups.size(); ++i) {
        if (normalizeAnswer(m_answerGroups.at(i).answerText) == key) {
            return i;
        }
    }
    return -1;
}

bool GameViewModel::isGroupRevealed(int groupIndex) const
{
    if (groupIndex < 0 || groupIndex >= m_answerGroups.size()) {
        return false;
    }
    for (int number : m_answerGroups.at(groupIndex).maskNumbers) {
        if (!m_revealedMaskNumbers.contains(number)) {
            return false;
        }
    }
    return true;
}

void GameViewModel::revealAnswerGroup(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= m_answerGroups.size()) {
        return;
    }
    for (int number : m_answerGroups.at(groupIndex).maskNumbers) {
        if (!m_revealedMaskNumbers.contains(number)) {
            m_revealedMaskNumbers.append(number);
        }
    }
    m_lastRevealedGroupIndex = groupIndex;
    emit revealedMasksChanged();
}

QString GameViewModel::formatRevealDetail(int groupIndex) const
{
    if (groupIndex < 0 || groupIndex >= m_answerGroups.size()) {
        return primaryCorrectAnswer();
    }
    const MaskAnswerGroup &group = m_answerGroups.at(groupIndex);
    QStringList parts;
    parts.reserve(group.maskNumbers.size());
    for (int number : group.maskNumbers) {
        parts.append(QString::number(number));
    }
    return QStringLiteral("%1 (%2)").arg(group.answerText, parts.join(QStringLiteral(", ")));
}

QString GameViewModel::puzzleDisplayImageUrl(int slotIndex) const
{
    if (m_layoutType == GameConstants::LayoutType::FullMask
        && slotIndex == 0
        && !m_puzzleMasks.isEmpty()) {
        return puzzleHiddenImageUrl();
    }
    return puzzleImageUrl(slotIndex);
}

void GameViewModel::resumeSession()
{
    if (!m_database) {
        return;
    }

    m_state = m_database->loadGameState();
    m_presetId = m_state.presetId;
    reloadTeamsFromDatabase();

    const bool hasPreset = m_presetId > 0;
    const bool hasRounds = hasPreset && !m_database->listRoundsForPreset(m_presetId).isEmpty();
    const bool hasTeams = m_teamAId > 0 && m_teamBId > 0;
    m_hasActiveSession = m_state.activeSession && hasPreset && hasRounds && hasTeams;
    if (!m_hasActiveSession) {
        return;
    }

    m_rounds = m_database->listRoundsForPreset(m_presetId);
    if (m_rounds.isEmpty()) {
        return;
    }

    m_roundIndex = 0;
    for (int i = 0; i < m_rounds.size(); ++i) {
        if (m_rounds.at(i).id == m_state.roundId) {
            m_roundIndex = i;
            break;
        }
    }

    loadCurrentRound();
    m_puzzleIndex = resolvePuzzleIndex(m_currentPuzzles, m_state.puzzleId);
    if (m_puzzleIndex >= m_currentPuzzles.size()) {
        m_puzzleIndex = qMax(0, m_currentPuzzles.size() - 1);
    }

    loadCurrentPuzzle();
    setStage(m_state.stage);
    setSubState(m_state.subState);
    applyPersistedTurnState();

    m_roundScoreTeamA = m_state.roundScoreTeamA;
    m_roundScoreTeamB = m_state.roundScoreTeamB;
    emit roundScoreChanged();
    emit roundProgressChanged();
    emit teamsChanged();

    if (m_stage == GameConstants::Stage::MainTurn) {
        m_cardsFaceUp = false;
        emit cardsFaceUpChanged(false);
        startMainTimer();
        QTimer::singleShot(50, this, [this]() {
            if (m_stage != GameConstants::Stage::MainTurn) {
                return;
            }
            m_cardsFaceUp = true;
            emit cardsFaceUpChanged(true);
        });
    } else if (m_stage == GameConstants::Stage::StealTurn) {
        m_cardsFaceUp = false;
        emit cardsFaceUpChanged(false);
        m_isStealTurn = true;
        startStealTimer();
        QTimer::singleShot(50, this, [this]() {
            if (m_stage != GameConstants::Stage::StealTurn) {
                return;
            }
            m_cardsFaceUp = true;
            emit cardsFaceUpChanged(true);
        });
    } else if (m_stage == GameConstants::Stage::ClosedCards) {
        m_cardsFaceUp = false;
        emit cardsFaceUpChanged(false);
    }

    if (m_stage == GameConstants::Stage::InterRound
        || m_stage == GameConstants::Stage::FinalVictory) {
        m_lastSettledRoundIndex = m_roundIndex;
    }

    persistState();
}

void GameViewModel::resetAllStoredScores()
{
    m_roundScoreTeamA = 0;
    m_roundScoreTeamB = 0;
    m_totalScoreTeamA = 0;
    m_totalScoreTeamB = 0;
    m_lastSettledRoundIndex = -1;
    m_teamAId = 0;
    m_teamBId = 0;
    m_teamAName.clear();
    m_teamBName.clear();

    if (m_database) {
        m_database->clearTeams();
    }

    emit roundScoreChanged();
    emit teamsChanged();
}

void GameViewModel::clearSession()
{
    stopTimer();
    m_hasActiveSession = false;
    m_presetId = 0;
    m_rounds.clear();
    m_currentPuzzles.clear();
    m_roundIndex = 0;
    m_puzzleIndex = 0;
    resetAllStoredScores();
    resetPuzzlePresentation();
    setStage(GameConstants::Stage::Welcome);
    setSubState(GameConstants::SubState::AwaitingReady);

    GameStateSnapshot cleared;
    cleared.stage = GameConstants::Stage::Welcome;
    cleared.subState = GameConstants::SubState::AwaitingReady;
    cleared.activeSession = false;
    m_state = cleared;
    m_database->saveGameState(cleared);

    emit hasActiveSessionChanged(false);
    emit currentPresetIdChanged();
}

void GameViewModel::startGame(int presetId)
{
    resetAllStoredScores();

    m_presetId = presetId;
    m_rounds = m_database->listRoundsForPreset(presetId);
    if (m_rounds.isEmpty()) {
        return;
    }

    m_roundIndex = 0;
    m_puzzleIndex = 0;
    m_hasActiveSession = true;
    m_activeTeam = GameConstants::TEAM_A;
    emit activeTeamChanged();

    setStage(GameConstants::Stage::TeamSetup);
    setSubState(GameConstants::SubState::AwaitingReady);
    emit hasActiveSessionChanged(true);
    emit currentPresetIdChanged();
    emit roundScoreChanged();
    emit roundProgressChanged();
    persistState();
}

void GameViewModel::configureTeams(const QString &teamA, const QString &teamB)
{
    QVector<TeamInfo> teams;
    TeamInfo a;
    a.name = teamA.trimmed().isEmpty() ? QStringLiteral("Команда А") : teamA.trimmed();
    a.score = m_totalScoreTeamA;
    TeamInfo b;
    b.name = teamB.trimmed().isEmpty() ? QStringLiteral("Команда Б") : teamB.trimmed();
    b.score = m_totalScoreTeamB;
    teams << a << b;

    if (!m_database->saveTeams(teams)) {
        return;
    }

    const QVector<TeamInfo> saved = m_database->listTeams();
    if (saved.size() >= 2) {
        m_teamAId = saved.at(0).id;
        m_teamBId = saved.at(1).id;
        m_teamAName = saved.at(0).name;
        m_teamBName = saved.at(1).name;
    }

    loadCurrentRound();
    loadCurrentPuzzle();
    setActiveTeamForMainTurn();
    setStage(GameConstants::Stage::ClosedCards);
    setSubState(GameConstants::SubState::AwaitingReady);
    m_cardsFaceUp = false;
    emit cardsFaceUpChanged(false);
    emit teamsChanged();
    persistState();
}

void GameViewModel::ready()
{
    if (m_stage != GameConstants::Stage::ClosedCards) {
        return;
    }

    m_cardsFaceUp = true;
    emit cardsFaceUpChanged(true);

    if (m_isStealTurn) {
        setStage(GameConstants::Stage::StealTurn);
        setSubState(GameConstants::SubState::StealTurn);
        startStealTimer();
    } else {
        setStage(GameConstants::Stage::MainTurn);
        setSubState(GameConstants::SubState::MainTurn);
        startMainTimer();
    }
    persistState();
}

void GameViewModel::submitAnswer(const QString &text)
{
    const QString answer = text.isEmpty() ? m_userAnswer : text;
    if (answer.trimmed().isEmpty()) {
        return;
    }
    if (m_stage != GameConstants::Stage::MainTurn && m_stage != GameConstants::Stage::StealTurn) {
        return;
    }

    if (m_gracePeriodActive) {
        m_gracePeriodActive = false;
        emit gracePeriodChanged();
    }
    stopTimer();
    m_submittedAnswer = answer.trimmed();
    emit submittedAnswerChanged();
    evaluateAnswer(m_submittedAnswer);
}

void GameViewModel::revealNextHint()
{
    if (!canRevealHint()) {
        return;
    }
    if (m_revealedHints >= m_puzzleHints.size()) {
        return;
    }
    ++m_revealedHints;
    emit hintsChanged();
}

void GameViewModel::transferTurn()
{
    if (m_isStealTurn) {
        return;
    }

    stopTimer();
    if (m_gracePeriodActive) {
        m_gracePeriodActive = false;
        emit gracePeriodChanged();
    }

    m_isStealTurn = true;
    setActiveTeamForStealTurn();
    m_cardsFaceUp = false;
    emit cardsFaceUpChanged(false);

    setStage(GameConstants::Stage::ClosedCards);
    setSubState(GameConstants::SubState::StealTurn);
    m_revealedAnswer.clear();
    emit revealedAnswerChanged();
    persistState();
}

void GameViewModel::resolveTeamA()
{
    const bool manualResolve = (m_stage == GameConstants::Stage::MainTurn
                                || m_stage == GameConstants::Stage::StealTurn)
                               && !m_userAnswer.trimmed().isEmpty();
    if (m_stage != GameConstants::Stage::Resolution && !manualResolve) {
        return;
    }
    if (manualResolve) {
        stopTimer();
        m_submittedAnswer = m_userAnswer.trimmed();
        emit submittedAnswerChanged();
        m_userAnswer.clear();
        emit userAnswerChanged();
    }
    m_activeTeam = GameConstants::TEAM_A;
    awardPointToActiveTeam();
    proceedAfterScoring();
}

void GameViewModel::resolveTeamB()
{
    const bool manualResolve = (m_stage == GameConstants::Stage::MainTurn
                                || m_stage == GameConstants::Stage::StealTurn)
                               && !m_userAnswer.trimmed().isEmpty();
    if (m_stage != GameConstants::Stage::Resolution && !manualResolve) {
        return;
    }
    if (manualResolve) {
        stopTimer();
        m_submittedAnswer = m_userAnswer.trimmed();
        emit submittedAnswerChanged();
        m_userAnswer.clear();
        emit userAnswerChanged();
    }
    m_activeTeam = GameConstants::TEAM_B;
    awardPointToActiveTeam();
    proceedAfterScoring();
}

void GameViewModel::rejectAll()
{
    const bool manualResolve = (m_stage == GameConstants::Stage::MainTurn
                                || m_stage == GameConstants::Stage::StealTurn)
                               && !m_userAnswer.trimmed().isEmpty();
    if (m_stage != GameConstants::Stage::Resolution && !manualResolve) {
        return;
    }
    if (manualResolve) {
        stopTimer();
        m_submittedAnswer = m_userAnswer.trimmed();
        emit submittedAnswerChanged();
        m_userAnswer.clear();
        emit userAnswerChanged();
    }
    advancePuzzleOrRound();
}

void GameViewModel::finishMissingReveal()
{
    if (m_stage != GameConstants::Stage::MissingReveal) {
        return;
    }

    if (m_answerWasCorrect) {
        if (m_layoutType == GameConstants::LayoutType::FullMask
            && !m_answerGroups.isEmpty()
            && !allMaskGroupsRevealed()) {
            setStage(GameConstants::Stage::MainTurn);
            m_cardsFaceUp = true;
            emit cardsFaceUpChanged(true);
            startMainTimer();
            persistState();
            return;
        }

        awardPointToActiveTeam();
        proceedAfterScoring();
        return;
    }

    if (m_isStealTurn) {
        enterResolution();
        return;
    }

    transferTurn();
}

void GameViewModel::advanceAfterRound()
{
    if (m_stage != GameConstants::Stage::InterRound) {
        return;
    }

    ++m_roundIndex;
    if (m_roundIndex >= m_rounds.size()) {
        finishGameVictory();
        return;
    }

    m_roundScoreTeamA = 0;
    m_roundScoreTeamB = 0;
    m_puzzleIndex = 0;
    m_isStealTurn = false;
    emit roundScoreChanged();

    loadCurrentRound();
    loadCurrentPuzzle();
    setActiveTeamForMainTurn();
    setStage(GameConstants::Stage::ClosedCards);
    setSubState(GameConstants::SubState::AwaitingReady);
    m_cardsFaceUp = false;
    emit cardsFaceUpChanged(false);
    emit roundProgressChanged();
    persistState();
}

int GameViewModel::layoutImageSlotCount() const
{
    if (m_layoutType == GameConstants::LayoutType::FullMask) {
        return 1;
    }
    if (m_layoutType == GameConstants::LayoutType::TextOnly) {
        return 0;
    }
    if (m_layoutType == GameConstants::LayoutType::SingleHybrid
        || m_layoutType == GameConstants::LayoutType::Equation) {
        return 3;
    }
    if (m_layoutType == GameConstants::LayoutType::Chronology
        || m_layoutType == GameConstants::LayoutType::Standard) {
        return 4;
    }
    if (m_layoutType == GameConstants::LayoutType::BlitzStandard) {
        return 8;
    }
    return 4;
}

QStringList GameViewModel::preparedAnswerList() const
{
    if (m_layoutType == GameConstants::LayoutType::FullMask && !m_answerGroups.isEmpty()) {
        QStringList answers;
        answers.reserve(m_answerGroups.size());
        for (const MaskAnswerGroup &group : m_answerGroups) {
            const QString answer = group.answerText.trimmed();
            if (!answer.isEmpty() && !answers.contains(answer, Qt::CaseInsensitive)) {
                answers.append(answer);
            }
        }
        return answers;
    }

    QStringList answers;
    for (const QString &option : m_answerOptions) {
        const QString trimmed = option.trimmed();
        if (!trimmed.isEmpty() && !answers.contains(trimmed, Qt::CaseInsensitive)) {
            answers.append(trimmed);
        }
    }
    if (!answers.isEmpty()) {
        return answers;
    }

    for (const QString &part : m_correctAnswer.split(QRegularExpression(QStringLiteral("[,;|]+")),
                                                     Qt::SkipEmptyParts)) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty() && !answers.contains(trimmed, Qt::CaseInsensitive)) {
            answers.append(trimmed);
        }
    }
    if (answers.isEmpty() && !m_correctAnswer.trimmed().isEmpty()) {
        answers.append(m_correctAnswer.trimmed());
    }
    return answers;
}

QJsonObject GameViewModel::currentPuzzlePayload() const
{
    QJsonObject payload;
    payload.insert(QStringLiteral("round_title"), m_roundTitle);
    payload.insert(QStringLiteral("round_rule"), m_ruleText);
    payload.insert(QStringLiteral("puzzle_num"), m_puzzleNumber);
    payload.insert(QStringLiteral("layout_type"), m_layoutType);
    payload.insert(QStringLiteral("hint"), m_hintText);
    payload.insert(QStringLiteral("hint_text"), m_hintText);
    payload.insert(QStringLiteral("active_team"), m_activeTeam);
    payload.insert(QStringLiteral("game_stage"), m_stage);
    payload.insert(QStringLiteral("submitted_answer"), m_submittedAnswer);
    payload.insert(QStringLiteral("missing_reveal_text"), m_missingRevealText);
    payload.insert(QStringLiteral("answer_was_correct"), m_answerWasCorrect);

    const bool shouldExposeAnswer = m_stage == GameConstants::Stage::Resolution || m_answerWasCorrect;
    const QString revealedAnswer = primaryCorrectAnswer();
    payload.insert(QStringLiteral("correct_answer"), shouldExposeAnswer ? revealedAnswer : QString());
    payload.insert(QStringLiteral("correct_answer_text"), shouldExposeAnswer ? revealedAnswer : QString());

    QJsonArray quotes;
    for (const QString &quote : m_quoteSlots) {
        quotes.append(quote);
    }
    payload.insert(QStringLiteral("quote_slots"), quotes);

    QJsonArray hints;
    for (const QString &hint : m_puzzleHints) {
        hints.append(hint);
    }
    payload.insert(QStringLiteral("hints"), hints);

    QJsonArray visibleHintsArray;
    for (const QString &hint : visibleHints()) {
        visibleHintsArray.append(hint);
    }
    payload.insert(QStringLiteral("visible_hints"), visibleHintsArray);

    QJsonArray answers;
    for (const QString &answer : preparedAnswerList()) {
        answers.append(answer);
    }
    payload.insert(QStringLiteral("answers"), answers);

    QJsonArray images;
    const int slotCount = layoutImageSlotCount();
    for (int slot = 0; slot < slotCount; ++slot) {
        images.append(puzzleDisplayImageUrl(slot));
    }
    payload.insert(QStringLiteral("images"), images);
    return payload;
}

bool GameViewModel::handleRemoteAction(const QString &action)
{
    if (action == RestApi::Actions::Ready) {
        ready();
        return true;
    }
    if (action == RestApi::Actions::TransferTurn) {
        transferTurn();
        return true;
    }
    if (action == RestApi::Actions::ResolveA) {
        resolveTeamA();
        return true;
    }
    if (action == RestApi::Actions::ResolveB) {
        resolveTeamB();
        return true;
    }
    if (action == RestApi::Actions::RejectAll) {
        rejectAll();
        return true;
    }
    return false;
}

void GameViewModel::onRemoteConnectedChanged(bool connected)
{
    setRemoteConnected(connected);
}

void GameViewModel::loadPersistedState()
{
    m_state = m_database->loadGameState();
    m_presetId = m_state.presetId;
    m_roundScoreTeamA = m_state.roundScoreTeamA;
    m_roundScoreTeamB = m_state.roundScoreTeamB;
    reloadTeamsFromDatabase();
    const bool hasPreset = m_presetId > 0;
    const bool hasRounds = hasPreset && !m_database->listRoundsForPreset(m_presetId).isEmpty();
    const bool hasTeams = m_teamAId > 0 && m_teamBId > 0;
    m_hasActiveSession = m_state.activeSession && hasPreset && hasRounds && hasTeams;
    if (!m_hasActiveSession) {
        m_presetId = 0;
        m_state = GameStateSnapshot();
        m_state.stage = GameConstants::Stage::Welcome;
        m_state.subState = GameConstants::SubState::AwaitingReady;
        m_database->saveGameState(m_state);
        resetAllStoredScores();
    }

    setStage(GameConstants::Stage::Welcome);
    setSubState(GameConstants::SubState::AwaitingReady);
    emit hasActiveSessionChanged(m_hasActiveSession);
    emit teamsChanged();
    emit currentPresetIdChanged();
}

void GameViewModel::persistState()
{
    if (!m_database) {
        return;
    }

    m_state.presetId = m_presetId;
    m_state.roundId = m_rounds.isEmpty() ? 0 : m_rounds.at(m_roundIndex).id;

    int puzzleId = m_currentPuzzles.isEmpty() ? 0 : m_currentPuzzles.at(m_puzzleIndex).id;
    if (puzzleId > 0) {
        puzzleId = m_database->canonicalPuzzleId(puzzleId);
    }
    m_state.puzzleId = puzzleId;

    m_state.turnTeamId = m_activeTeam == GameConstants::TEAM_B ? m_teamBId : m_teamAId;
    if (!(m_hasActiveSession && m_stage == GameConstants::Stage::Welcome)) {
        m_state.stage = m_stage;
        m_state.subState = m_subState;
    }
    m_state.roundScoreTeamA = m_roundScoreTeamA;
    m_state.roundScoreTeamB = m_roundScoreTeamB;
    m_state.activeSession = m_hasActiveSession;
    m_database->saveGameState(m_state);
}

void GameViewModel::reloadTeamsFromDatabase()
{
    if (!m_database) {
        return;
    }

    const QVector<TeamInfo> teams = m_database->listTeams();
    if (teams.size() < 2) {
        return;
    }

    m_teamAId = teams.at(0).id;
    m_teamBId = teams.at(1).id;
    m_teamAName = teams.at(0).name;
    m_teamBName = teams.at(1).name;
    m_totalScoreTeamA = teams.at(0).score;
    m_totalScoreTeamB = teams.at(1).score;
}

int GameViewModel::resolvePuzzleIndex(const QVector<PuzzleInfo> &puzzles, int savedPuzzleId) const
{
    if (puzzles.isEmpty() || savedPuzzleId <= 0 || !m_database) {
        return 0;
    }

    const int canonicalId = m_database->canonicalPuzzleId(savedPuzzleId);
    for (int i = 0; i < puzzles.size(); ++i) {
        if (puzzles.at(i).id == canonicalId) {
            return i;
        }
    }

    const PuzzleInfo saved = m_database->puzzleById(savedPuzzleId);
    if (saved.id > 0) {
        for (int i = 0; i < puzzles.size(); ++i) {
            if (puzzles.at(i).sortOrder == saved.sortOrder) {
                return i;
            }
        }
    }

    return 0;
}

void GameViewModel::applyPersistedTurnState()
{
    const QString mainTeam = mainTeamForPuzzle(m_puzzleNumber);
    const QString stealTeam = opponentTeam(mainTeam);

    if (m_stage == GameConstants::Stage::StealTurn) {
        m_isStealTurn = true;
        m_activeTeam = stealTeam;
    } else if (m_stage == GameConstants::Stage::MainTurn) {
        m_isStealTurn = false;
        m_activeTeam = mainTeam;
    } else if (m_stage == GameConstants::Stage::ClosedCards) {
        if (m_subState == GameConstants::SubState::StealTurn) {
            m_isStealTurn = true;
            m_activeTeam = stealTeam;
        } else {
            m_isStealTurn = false;
            m_activeTeam = mainTeam;
        }
    } else if (m_teamAId > 0 && m_teamBId > 0 && m_state.turnTeamId > 0) {
        m_activeTeam = m_state.turnTeamId == m_teamBId ? GameConstants::TEAM_B : GameConstants::TEAM_A;
        m_isStealTurn = m_activeTeam != mainTeam
                        && (m_stage == GameConstants::Stage::StealTurn
                            || m_stage == GameConstants::Stage::ClosedCards);
    } else if (m_activeTeam.isEmpty()) {
        m_activeTeam = mainTeam;
        m_isStealTurn = false;
    }

    emit activeTeamChanged();
}

QString GameViewModel::mainTeamForPuzzle(int puzzleNumber) const
{
    return puzzleNumber % 2 == 1 ? GameConstants::TEAM_A : GameConstants::TEAM_B;
}

QString GameViewModel::opponentTeam(const QString &team) const
{
    return team == GameConstants::TEAM_B ? GameConstants::TEAM_A : GameConstants::TEAM_B;
}

void GameViewModel::setActiveTeamForMainTurn()
{
    m_isStealTurn = false;
    m_activeTeam = mainTeamForPuzzle(m_puzzleNumber);
    emit activeTeamChanged();
}

void GameViewModel::setActiveTeamForStealTurn()
{
    m_isStealTurn = true;
    m_activeTeam = opponentTeam(mainTeamForPuzzle(m_puzzleNumber));
    emit activeTeamChanged();
}

void GameViewModel::setStage(const QString &stage)
{
    if (m_stage == stage) {
        return;
    }
    m_stage = stage;
    emit currentStageChanged();
    updateLocalControlsVisibility();
}

void GameViewModel::setSubState(const QString &subState)
{
    if (m_subState == subState) {
        return;
    }
    m_subState = subState;
    emit currentSubStateChanged();
}

void GameViewModel::setRemoteConnected(bool connected)
{
    if (m_remoteConnected == connected) {
        return;
    }
    m_remoteConnected = connected;
    emit remoteConnectedChanged(connected);
    updateLocalControlsVisibility();
}

void GameViewModel::updateLocalControlsVisibility()
{
    const bool show = !m_remoteConnected;
    if (m_showLocalControls == show) {
        return;
    }
    m_showLocalControls = show;
    emit showLocalControlsChanged(show);
}

void GameViewModel::loadCurrentRound()
{
    if (m_rounds.isEmpty()) {
        return;
    }

    const RoundInfo round = m_rounds.at(m_roundIndex);
    m_layoutType = round.layoutType;
    m_roundTitle = m_database->localizedString(round.titleKey);
    m_ruleText = m_database->localizedString(round.ruleTextKey);
    m_baseTimerSeconds = round.timerDuration;
    m_stealTimerSeconds = round.stealDuration;
    m_showMilliseconds = round.layoutType == GameConstants::LayoutType::BlitzStandard;

    m_currentPuzzles = m_database->listPuzzlesForRound(round.id);
    if (m_currentPuzzles.size() > GameConstants::MAX_PUZZLES_PER_ROUND) {
        m_currentPuzzles.resize(GameConstants::MAX_PUZZLES_PER_ROUND);
    }
    m_state.roundId = round.id;

    emit layoutTypeChanged();
    emit roundTitleChanged();
    emit ruleTextChanged();
    emit roundProgressChanged();
    emit roundScoreChanged();
}

void GameViewModel::loadCurrentPuzzle()
{
    if (m_currentPuzzles.isEmpty()) {
        return;
    }

    const PuzzleInfo puzzle = m_currentPuzzles.at(m_puzzleIndex);
    m_puzzleNumber = m_puzzleIndex + 1;
    m_correctAnswer = puzzle.correctAnswer;
    loadAnswerOptionsFromPuzzle(puzzle);
    parsePackedHints(m_database->localizedString(puzzle.hintTextKey));
    m_state.puzzleId = puzzle.id;

    m_puzzleMasks.clear();
    m_revealedMaskNumbers.clear();
    m_answerGroups.clear();
    m_lastRevealedGroupIndex = -1;
    m_puzzleMasks = m_database->listPuzzleMasks(puzzle.id);
    if (m_puzzleMasks.isEmpty() && puzzle.templateId > 0) {
        const QString contour = m_database->maskTemplateContour(puzzle.templateId);
        if (!contour.isEmpty()) {
            PuzzleMaskInfo legacy;
            legacy.sortOrder = 1;
            legacy.answerText = primaryCorrectAnswer();
            legacy.contourPoints = contour;
            m_puzzleMasks.append(legacy);
        }
    }
    buildAnswerGroups();
    emit puzzleMaskChanged();
    emit revealedMasksChanged();

    m_quoteSlots.clear();
    if (!puzzle.quoteSlotsJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(puzzle.quoteSlotsJson.toUtf8());
        for (const QJsonValue &value : doc.array()) {
            m_quoteSlots.append(value.toString());
        }
    }
    emit quoteSlotsChanged();

    const QString nextHybridStyle = parseHybridAnimStyle(puzzle.correctOrder);
    if (m_hybridAnimationStyle != nextHybridStyle) {
        m_hybridAnimationStyle = nextHybridStyle;
        emit hybridAnimationStyleChanged();
    }

    resetPuzzlePresentation();
    ++m_puzzleImageRevision;
    emit puzzleNumberChanged();
    emit hintTextChanged();
    emit hintsChanged();
    emit currentPuzzleIdChanged();
}

void GameViewModel::resetPuzzlePresentation()
{
    stopTimer();
    if (m_gracePeriodActive) {
        m_gracePeriodActive = false;
        emit gracePeriodChanged();
    }
    m_userAnswer.clear();
    m_submittedAnswer.clear();
    m_missingRevealText.clear();
    m_revealedAnswer.clear();
    m_revealedMaskNumbers.clear();
    m_lastRevealedGroupIndex = -1;
    m_revealedHints = 0;
    m_hintUnlockReady = false;
    m_answerWasCorrect = false;
    m_isCorrectFlash = false;
    m_isWrongFlash = false;
    emit userAnswerChanged();
    emit submittedAnswerChanged();
    emit missingRevealTextChanged();
    emit revealedAnswerChanged();
    emit revealedMasksChanged();
    emit hintsChanged();
    emit answerWasCorrectChanged();
    emit evaluationFlashChanged();
}

void GameViewModel::startMainTimer()
{
    m_isStealTurn = false;
    m_gracePeriodActive = false;
    emit gracePeriodChanged();
    m_timerSeconds = m_baseTimerSeconds;
    m_timerMilliseconds = 0;
    emit timerChanged();
    updateHintUnlockState();
    m_gameTimer.start();
}

void GameViewModel::startStealTimer()
{
    m_gracePeriodActive = false;
    emit gracePeriodChanged();
    m_timerSeconds = m_stealTimerSeconds;
    m_timerMilliseconds = 0;
    emit timerChanged();
    updateHintUnlockState();
    m_gameTimer.start();
}

void GameViewModel::startGracePeriod()
{
    m_gracePeriodActive = true;
    m_timerSeconds = kGracePeriodSeconds;
    m_timerMilliseconds = 0;
    emit gracePeriodChanged();
    emit timerChanged();
    m_gameTimer.start();
}

void GameViewModel::finishGracePeriod()
{
    m_gracePeriodActive = false;
    emit gracePeriodChanged();
    stopTimer();

    if (!m_userAnswer.trimmed().isEmpty()) {
        submitAnswer(m_userAnswer);
        return;
    }

    if (m_isStealTurn) {
        enterResolution();
    } else {
        transferTurn();
    }
}

void GameViewModel::stopTimer()
{
    m_gameTimer.stop();
}

void GameViewModel::onTimerTick()
{
    if (m_timerMilliseconds > 0) {
        m_timerMilliseconds -= kTimerTickMs;
        if (m_timerMilliseconds < 0) {
            m_timerMilliseconds = 0;
        }
    } else if (m_timerSeconds > 0) {
        m_timerMilliseconds = 1000 - kTimerTickMs;
        --m_timerSeconds;
    } else if (m_gracePeriodActive) {
        finishGracePeriod();
        return;
    } else {
        startGracePeriod();
        return;
    }

    emit timerChanged();
    updateHintUnlockState();
}

void GameViewModel::evaluateAnswer(const QString &answer)
{
    setStage(GameConstants::Stage::Evaluating);

    bool correct = false;
    int matchedGroupIndex = -1;
    if (m_layoutType == GameConstants::LayoutType::FullMask && !m_answerGroups.isEmpty()) {
        matchedGroupIndex = findAnswerGroupIndex(answer);
        correct = matchedGroupIndex >= 0 && !isGroupRevealed(matchedGroupIndex);
    } else {
        correct = matchesAcceptedAnswer(answer);
    }

    m_answerWasCorrect = correct;
    emit answerWasCorrectChanged();

    if (correct) {
        if (m_layoutType == GameConstants::LayoutType::FullMask && matchedGroupIndex >= 0) {
            revealAnswerGroup(matchedGroupIndex);
        }
        m_userAnswer.clear();
        emit userAnswerChanged();
        m_isCorrectFlash = true;
        emit evaluationFlashChanged();
        m_flashTimer.start();
        enterMissingReveal(true);
        return;
    }

    m_isWrongFlash = true;
    emit evaluationFlashChanged();
    m_flashTimer.start();

    if (RoundRevealPolicy::requiresMissingReveal(m_layoutType)) {
        if (m_layoutType == GameConstants::LayoutType::FullMask && !m_answerGroups.isEmpty()) {
            for (int i = 0; i < m_answerGroups.size(); ++i) {
                if (!isGroupRevealed(i)) {
                    revealAnswerGroup(i);
                    m_lastRevealedGroupIndex = i;
                    break;
                }
            }
            emit revealedMasksChanged();
        }
        enterMissingReveal(false);
        return;
    }

    if (m_isStealTurn) {
        enterResolution();
    } else {
        transferTurn();
    }
}

void GameViewModel::enterMissingReveal(bool /*wasCorrect*/)
{
    QString detail = primaryCorrectAnswer();
    if (m_layoutType == GameConstants::LayoutType::FullMask && m_lastRevealedGroupIndex >= 0) {
        detail = formatRevealDetail(m_lastRevealedGroupIndex);
    }
    m_missingRevealText = RoundRevealPolicy::formatMissingRevealText(m_layoutType, detail);
    emit missingRevealTextChanged();
    setStage(GameConstants::Stage::MissingReveal);
    persistState();

    if (m_answerWasCorrect) {
        QTimer::singleShot(1200, this, &GameViewModel::finishMissingReveal);
    }
}

void GameViewModel::enterResolution()
{
    m_revealedAnswer = primaryCorrectAnswer();
    emit revealedAnswerChanged();
    setStage(GameConstants::Stage::Resolution);
    setSubState(GameConstants::SubState::AwaitingResolve);
    persistState();
}

void GameViewModel::awardPointToActiveTeam()
{
    const int maxStars = maxRoundStars();
    bool changed = false;
    if (m_activeTeam == GameConstants::TEAM_A) {
        if (m_roundScoreTeamA < maxStars) {
            ++m_roundScoreTeamA;
            changed = true;
        }
    } else if (m_activeTeam == GameConstants::TEAM_B && m_roundScoreTeamB < maxStars) {
        ++m_roundScoreTeamB;
        changed = true;
    }

    if (!changed) {
        return;
    }

    emit roundScoreChanged();
    persistState();
}

void GameViewModel::settleRoundScores()
{
    if (m_lastSettledRoundIndex == m_roundIndex) {
        return;
    }
    m_lastSettledRoundIndex = m_roundIndex;

    if (m_roundScoreTeamA > m_roundScoreTeamB) {
        ++m_totalScoreTeamA;
    } else if (m_roundScoreTeamB > m_roundScoreTeamA) {
        ++m_totalScoreTeamB;
    } else {
        ++m_totalScoreTeamA;
        ++m_totalScoreTeamB;
    }

    QVector<TeamInfo> teams;
    teams << TeamInfo{m_teamAId, m_teamAName, m_totalScoreTeamA}
          << TeamInfo{m_teamBId, m_teamBName, m_totalScoreTeamB};
    m_database->saveTeams(teams);

    emit teamsChanged();
    persistState();
}

bool GameViewModel::shouldEndRoundEarlyAfterScoring(const QString &scoringTeam) const
{
    if (m_puzzleNumber != 5) {
        return false;
    }

    const int a = m_roundScoreTeamA;
    const int b = m_roundScoreTeamB;
    if (scoringTeam == GameConstants::TEAM_A) {
        return a == 5 && b == 3;
    }
    if (scoringTeam == GameConstants::TEAM_B) {
        return b == 5 && a == 3;
    }
    return false;
}

bool GameViewModel::tryCompleteRoundEarly()
{
    stopTimer();
    resetPuzzlePresentation();
    m_cardsFaceUp = false;
    emit cardsFaceUpChanged(false);
    setStage(GameConstants::Stage::RoundEnded);
    persistState();
    return true;
}

void GameViewModel::proceedAfterScoring()
{
    const QString scoringTeam = m_activeTeam;
    if (shouldEndRoundEarlyAfterScoring(scoringTeam) && tryCompleteRoundEarly()) {
        return;
    }
    advancePuzzleOrRound();
}

void GameViewModel::confirmRoundEnd()
{
    if (m_stage != GameConstants::Stage::RoundEnded) {
        return;
    }

    settleRoundScores();

    if (m_roundIndex + 1 >= m_rounds.size()) {
        finishGameVictory();
        return;
    }

    setStage(GameConstants::Stage::InterRound);
    persistState();
}

void GameViewModel::finishGameVictory()
{
    setStage(GameConstants::Stage::FinalVictory);
    m_hasActiveSession = false;
    emit hasActiveSessionChanged(false);
    persistState();
    emit gameFinished();
}

void GameViewModel::advancePuzzleOrRound()
{
    m_isStealTurn = false;
    resetPuzzlePresentation();

    if (m_puzzleIndex + 1 < m_currentPuzzles.size()) {
        ++m_puzzleIndex;
        loadCurrentPuzzle();
        setStage(GameConstants::Stage::ClosedCards);
        setSubState(GameConstants::SubState::AwaitingReady);
        m_cardsFaceUp = false;
        emit cardsFaceUpChanged(false);
        setActiveTeamForMainTurn();
        persistState();
        return;
    }

    settleRoundScores();

    if (m_roundIndex + 1 >= m_rounds.size()) {
        finishGameVictory();
    } else {
        setStage(GameConstants::Stage::InterRound);
        persistState();
    }
}

QString GameViewModel::normalizeAnswer(const QString &answer) const
{
    QString normalized = answer.trimmed().toLower();
    normalized.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return normalized;
}

void GameViewModel::loadAnswerOptionsFromPuzzle(const PuzzleInfo &puzzle)
{
    m_answerOptions.clear();
    if (puzzle.correctOrder.isEmpty()) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(puzzle.correctOrder.toUtf8());
    if (!doc.isArray()) {
        return;
    }

    for (const QJsonValue &value : doc.array()) {
        const QString option = value.toString().trimmed();
        if (option.isEmpty() || option.startsWith(kHybridAnimPrefix)) {
            continue;
        }
        m_answerOptions.append(option);
    }
}

QString GameViewModel::primaryCorrectAnswer() const
{
    for (const QString &option : m_answerOptions) {
        const QString trimmed = option.trimmed();
        if (!trimmed.isEmpty()) {
            return trimmed;
        }
    }

    const QStringList parts = m_correctAnswer.split(QRegularExpression(QStringLiteral("[,;|]+")),
                                                  Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            return trimmed;
        }
    }

    return m_correctAnswer.trimmed();
}

QStringList GameViewModel::acceptedAnswers() const
{
    return preparedAnswerList();
}

bool GameViewModel::matchesAcceptedAnswer(const QString &answer) const
{
    const QString normalized = normalizeAnswer(answer);
    if (normalized.isEmpty()) {
        return false;
    }

    for (const QString &candidate : acceptedAnswers()) {
        if (normalizeAnswer(candidate) == normalized) {
            return true;
        }
    }
    return false;
}

QString GameViewModel::activeTeamIdLabel() const
{
    return m_activeTeam;
}
