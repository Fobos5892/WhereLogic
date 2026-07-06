#include "GameViewModel.h"

#include "../core/GameConstants.h"
#include "../core/RoundRevealPolicy.h"
#include "../models/NetworkServer.h"

#include "RestApiConstants.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

namespace {

constexpr int kTimerTickMs = 100;

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
    if (m_state.puzzleId <= 0 || m_puzzleMaskContour.isEmpty()) {
        return {};
    }
    return QStringLiteral("image://puzzle/%1/hide").arg(m_state.puzzleId);
}

QString GameViewModel::puzzleDisplayImageUrl(int slotIndex) const
{
    if (m_layoutType == GameConstants::LayoutType::FullMask
        && slotIndex == 0
        && !m_puzzleMaskContour.isEmpty()) {
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
    m_hasActiveSession = m_state.activeSession;
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
    m_hintText = m_database->localizedString(puzzle.hintTextKey);
    m_state.puzzleId = puzzle.id;

    m_puzzleMaskContour.clear();
    if (puzzle.templateId > 0) {
        m_puzzleMaskContour = m_database->maskTemplateContour(puzzle.templateId);
    }
    emit puzzleMaskChanged();

    m_quoteSlots.clear();
    if (!puzzle.quoteSlotsJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(puzzle.quoteSlotsJson.toUtf8());
        for (const QJsonValue &value : doc.array()) {
            m_quoteSlots.append(value.toString());
        }
    }
    emit quoteSlotsChanged();

    resetPuzzlePresentation();
    emit puzzleNumberChanged();
    emit hintTextChanged();
    emit currentPuzzleIdChanged();
}

void GameViewModel::resetPuzzlePresentation()
{
    stopTimer();
    m_userAnswer.clear();
    m_submittedAnswer.clear();
    m_missingRevealText.clear();
    m_revealedAnswer.clear();
    m_answerWasCorrect = false;
    m_isCorrectFlash = false;
    m_isWrongFlash = false;
    emit userAnswerChanged();
    emit submittedAnswerChanged();
    emit missingRevealTextChanged();
    emit revealedAnswerChanged();
    emit answerWasCorrectChanged();
    emit evaluationFlashChanged();
}

void GameViewModel::startMainTimer()
{
    m_isStealTurn = false;
    m_timerSeconds = m_baseTimerSeconds;
    m_timerMilliseconds = 0;
    emit timerChanged();
    m_gameTimer.start();
}

void GameViewModel::startStealTimer()
{
    m_timerSeconds = m_stealTimerSeconds;
    m_timerMilliseconds = 0;
    emit timerChanged();
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
}

void GameViewModel::evaluateAnswer(const QString &answer)
{
    setStage(GameConstants::Stage::Evaluating);
    const bool correct = normalizeAnswer(answer) == normalizeAnswer(m_correctAnswer);
    m_answerWasCorrect = correct;
    emit answerWasCorrectChanged();

    if (correct) {
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
    m_missingRevealText = RoundRevealPolicy::formatMissingRevealText(m_layoutType, m_correctAnswer);
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
