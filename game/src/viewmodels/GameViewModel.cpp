#include "GameViewModel.h"

#include "../core/GameConstants.h"
#include "../core/RoundRevealPolicy.h"
#include "../models/NetworkServer.h"

#include "RestApiConstants.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QHash>
#include <QRegularExpression>

#include <algorithm>

namespace {

constexpr int kTimerTickMs = 100;
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
}

GameViewModel::~GameViewModel()
{
    stopTimer();
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
    return QStringLiteral("image://puzzle/%1/%2").arg(m_state.puzzleId).arg(slotIndex);
}

QString GameViewModel::puzzleHiddenImageUrl() const
{
    if (m_state.puzzleId <= 0 || m_puzzleMasks.isEmpty()) {
        return {};
    }
    const QString suffix = hiddenImageUrlSuffix();
    if (suffix.isEmpty()) {
        return QStringLiteral("image://puzzle/%1/hide").arg(m_state.puzzleId);
    }
    return QStringLiteral("image://puzzle/%1/hide/%2").arg(m_state.puzzleId).arg(suffix);
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
        return m_correctAnswer;
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
    if (!m_hasActiveSession || m_presetId <= 0) {
        return;
    }

    m_rounds = m_database->listRoundsForPreset(m_presetId);
    if (m_rounds.isEmpty()) {
        return;
    }

    for (int i = 0; i < m_rounds.size(); ++i) {
        if (m_rounds.at(i).id == m_state.roundId) {
            m_roundIndex = i;
            break;
        }
    }

    loadCurrentRound();
    const auto puzzles = m_database->listPuzzlesForRound(m_state.roundId);
    m_currentPuzzles = puzzles;
    for (int i = 0; i < puzzles.size(); ++i) {
        if (puzzles.at(i).id == m_state.puzzleId) {
            m_puzzleIndex = i;
            break;
        }
    }

    loadCurrentPuzzle();
    setStage(m_state.stage);
    setSubState(m_state.subState);
    m_roundScoreTeamA = m_state.roundScoreTeamA;
    m_roundScoreTeamB = m_state.roundScoreTeamB;
    emit roundScoreChanged();

    if (m_stage == GameConstants::Stage::MainTurn) {
        m_cardsFaceUp = true;
        emit cardsFaceUpChanged(true);
        startMainTimer();
    } else if (m_stage == GameConstants::Stage::StealTurn) {
        m_cardsFaceUp = true;
        m_isStealTurn = true;
        emit cardsFaceUpChanged(true);
        startStealTimer();
    }
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
    m_roundScoreTeamA = 0;
    m_roundScoreTeamB = 0;
    m_totalScoreTeamA = 0;
    m_totalScoreTeamB = 0;
    m_teamAName.clear();
    m_teamBName.clear();
    resetPuzzlePresentation();
    setStage(GameConstants::Stage::Welcome);
    setSubState(GameConstants::SubState::AwaitingReady);

    GameStateSnapshot cleared;
    cleared.stage = GameConstants::Stage::Welcome;
    cleared.subState = GameConstants::SubState::AwaitingReady;
    cleared.activeSession = false;
    m_state = cleared;
    m_database->saveGameState(cleared);
    m_database->clearTeams();

    emit hasActiveSessionChanged(false);
    emit roundScoreChanged();
    emit teamsChanged();
    emit currentPresetIdChanged();
}

void GameViewModel::startGame(int presetId)
{
    m_presetId = presetId;
    m_rounds = m_database->listRoundsForPreset(presetId);
    if (m_rounds.isEmpty()) {
        return;
    }

    m_roundIndex = 0;
    m_puzzleIndex = 0;
    m_roundScoreTeamA = 0;
    m_roundScoreTeamB = 0;
    m_totalScoreTeamA = 0;
    m_totalScoreTeamB = 0;
    m_hasActiveSession = true;
    m_activeTeam = GameConstants::TEAM_A;

    setStage(GameConstants::Stage::TeamSetup);
    setSubState(GameConstants::SubState::AwaitingReady);
    emit hasActiveSessionChanged(true);
    emit currentPresetIdChanged();
    emit roundScoreChanged();
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

    m_activeTeam = GameConstants::TEAM_A;
    loadCurrentRound();
    loadCurrentPuzzle();
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

    m_isStealTurn = true;
    m_cardsFaceUp = false;
    emit cardsFaceUpChanged(false);
    m_activeTeam = GameConstants::TEAM_B;
    emit activeTeamChanged();

    setStage(GameConstants::Stage::ClosedCards);
    setSubState(GameConstants::SubState::AwaitingReady);
    m_revealedAnswer.clear();
    emit revealedAnswerChanged();
    persistState();
}

void GameViewModel::resolveTeamA()
{
    if (m_stage != GameConstants::Stage::Resolution) {
        return;
    }
    m_activeTeam = GameConstants::TEAM_A;
    awardPointToActiveTeam();
    advancePuzzleOrRound();
}

void GameViewModel::resolveTeamB()
{
    if (m_stage != GameConstants::Stage::Resolution) {
        return;
    }
    m_activeTeam = GameConstants::TEAM_B;
    awardPointToActiveTeam();
    advancePuzzleOrRound();
}

void GameViewModel::rejectAll()
{
    if (m_stage != GameConstants::Stage::Resolution) {
        return;
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
        advancePuzzleOrRound();
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
        setStage(GameConstants::Stage::FinalVictory);
        m_hasActiveSession = false;
        emit hasActiveSessionChanged(false);
        persistState();
        emit gameFinished();
        return;
    }

    m_roundScoreTeamA = 0;
    m_roundScoreTeamB = 0;
    m_puzzleIndex = 0;
    m_isStealTurn = false;
    m_activeTeam = GameConstants::TEAM_A;
    emit activeTeamChanged();
    emit roundScoreChanged();

    loadCurrentRound();
    loadCurrentPuzzle();
    setStage(GameConstants::Stage::ClosedCards);
    setSubState(GameConstants::SubState::AwaitingReady);
    m_cardsFaceUp = false;
    emit cardsFaceUpChanged(false);
    persistState();
}

QJsonObject GameViewModel::currentPuzzlePayload() const
{
    QJsonObject payload;
    payload.insert(QStringLiteral("round_title"), m_roundTitle);
    payload.insert(QStringLiteral("puzzle_num"), m_puzzleNumber);
    payload.insert(QStringLiteral("layout_type"), m_layoutType);
    payload.insert(QStringLiteral("hint"), m_hintText);
    payload.insert(QStringLiteral("active_team"), m_activeTeam);
    payload.insert(QStringLiteral("game_stage"), m_stage);
    payload.insert(QStringLiteral("submitted_answer"), m_submittedAnswer);
    payload.insert(QStringLiteral("missing_reveal_text"), m_missingRevealText);
    payload.insert(QStringLiteral("answer_was_correct"), m_answerWasCorrect);

    if (m_stage == GameConstants::Stage::Resolution || m_answerWasCorrect) {
        payload.insert(QStringLiteral("correct_answer"), m_correctAnswer);
    } else {
        payload.insert(QStringLiteral("correct_answer"), QString());
    }

    QJsonArray quotes;
    for (const QString &quote : m_quoteSlots) {
        quotes.append(quote);
    }
    payload.insert(QStringLiteral("quote_slots"), quotes);
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

    const QVector<TeamInfo> teams = m_database->listTeams();
    if (teams.size() >= 2) {
        m_teamAId = teams.at(0).id;
        m_teamBId = teams.at(1).id;
        m_teamAName = teams.at(0).name;
        m_teamBName = teams.at(1).name;
        m_totalScoreTeamA = teams.at(0).score;
        m_totalScoreTeamB = teams.at(1).score;
    }

    const bool hasPreset = m_presetId > 0;
    const bool hasRounds = hasPreset && !m_database->listRoundsForPreset(m_presetId).isEmpty();
    const bool hasTeams = teams.size() >= 2;
    m_hasActiveSession = m_state.activeSession && hasPreset && hasRounds && hasTeams;
    if (!m_hasActiveSession) {
        m_presetId = 0;
        m_state = GameStateSnapshot();
        m_state.stage = GameConstants::Stage::Welcome;
        m_state.subState = GameConstants::SubState::AwaitingReady;
        m_database->saveGameState(m_state);
    }

    setStage(GameConstants::Stage::Welcome);
    setSubState(GameConstants::SubState::AwaitingReady);
    emit hasActiveSessionChanged(m_hasActiveSession);
    emit teamsChanged();
    emit currentPresetIdChanged();
}

void GameViewModel::persistState()
{
    m_state.presetId = m_presetId;
    m_state.roundId = m_rounds.isEmpty() ? 0 : m_rounds.at(m_roundIndex).id;
    m_state.puzzleId = m_currentPuzzles.isEmpty() ? 0 : m_currentPuzzles.at(m_puzzleIndex).id;
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
    m_state.roundId = round.id;

    emit layoutTypeChanged();
    emit roundTitleChanged();
    emit ruleTextChanged();
}

void GameViewModel::loadCurrentPuzzle()
{
    if (m_currentPuzzles.isEmpty()) {
        return;
    }

    const PuzzleInfo puzzle = m_currentPuzzles.at(m_puzzleIndex);
    m_puzzleNumber = puzzle.sortOrder;
    m_correctAnswer = puzzle.correctAnswer;
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
            legacy.answerText = puzzle.correctAnswer;
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
    emit puzzleNumberChanged();
    emit hintTextChanged();
    emit hintsChanged();
    emit currentPuzzleIdChanged();
}

void GameViewModel::resetPuzzlePresentation()
{
    stopTimer();
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
    m_timerSeconds = m_baseTimerSeconds;
    m_timerMilliseconds = 0;
    emit timerChanged();
    updateHintUnlockState();
    m_gameTimer.start();
}

void GameViewModel::startStealTimer()
{
    m_timerSeconds = m_stealTimerSeconds;
    m_timerMilliseconds = 0;
    emit timerChanged();
    updateHintUnlockState();
    m_gameTimer.start();
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
    } else {
        stopTimer();
        submitAnswer(QString());
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
        correct = normalizeAnswer(answer) == normalizeAnswer(m_correctAnswer);
    }

    m_answerWasCorrect = correct;
    emit answerWasCorrectChanged();

    if (correct) {
        if (m_layoutType == GameConstants::LayoutType::FullMask && matchedGroupIndex >= 0) {
            revealAnswerGroup(matchedGroupIndex);
        }
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
    QString detail = m_correctAnswer;
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
    m_revealedAnswer = m_correctAnswer;
    emit revealedAnswerChanged();
    setStage(GameConstants::Stage::Resolution);
    setSubState(GameConstants::SubState::AwaitingResolve);
    persistState();
}

void GameViewModel::awardPointToActiveTeam()
{
    if (m_activeTeam == GameConstants::TEAM_A) {
        if (m_roundScoreTeamA < GameConstants::MAX_ROUND_STARS) {
            ++m_roundScoreTeamA;
            ++m_totalScoreTeamA;
        }
    } else if (m_roundScoreTeamB < GameConstants::MAX_ROUND_STARS) {
        ++m_roundScoreTeamB;
        ++m_totalScoreTeamB;
    }

    QVector<TeamInfo> teams;
    TeamInfo a{m_teamAId, m_teamAName, m_totalScoreTeamA};
    TeamInfo b{m_teamBId, m_teamBName, m_totalScoreTeamB};
    teams << a << b;
    m_database->saveTeams(teams);

    emit roundScoreChanged();
    emit teamsChanged();
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
        m_activeTeam = GameConstants::TEAM_A;
        emit activeTeamChanged();
        persistState();
        return;
    }

    setStage(GameConstants::Stage::InterRound);
    persistState();
}

QString GameViewModel::normalizeAnswer(const QString &answer) const
{
    QString normalized = answer.trimmed().toLower();
    normalized.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return normalized;
}

QString GameViewModel::activeTeamIdLabel() const
{
    return m_activeTeam;
}
