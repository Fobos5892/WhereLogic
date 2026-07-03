#include "PresenterViewModel.h"

#include "RestApiConstants.h"

PresenterViewModel::PresenterViewModel(QObject *parent)
    : QObject(parent)
    , m_connectionState(QStringLiteral("disconnected"))
{
    m_pollTimer.setInterval(RestApi::PUZZLE_POLL_INTERVAL_MS);
    connect(&m_pollTimer, &QTimer::timeout, this, &PresenterViewModel::pollCurrentPuzzle);

    connect(&m_api, &RestApiClient::authenticateFinished,
            this, &PresenterViewModel::onAuthenticateFinished);
    connect(&m_api, &RestApiClient::currentPuzzleReceived,
            this, &PresenterViewModel::onCurrentPuzzleReceived);
}

void PresenterViewModel::setServerHost(const QString &host)
{
    if (m_serverHost == host)
        return;
    m_serverHost = host;
    m_api.setServerHost(host);
    emit serverHostChanged();
}

void PresenterViewModel::setPin(const QString &pin)
{
    if (m_pin == pin)
        return;
    m_pin = pin;
    emit pinChanged();
}

void PresenterViewModel::connectToServer()
{
    if (m_serverHost.isEmpty() || m_pin.isEmpty())
        return;

    setConnectionState(QStringLiteral("connecting"));
    m_api.setServerHost(m_serverHost);
    m_api.authenticate(m_pin);
}

void PresenterViewModel::disconnectFromServer()
{
    stopPolling();
    m_api.stopHeartbeat();
    m_api.setAuthToken({});
    setConnectionState(QStringLiteral("disconnected"));
}

void PresenterViewModel::ready()
{
    m_api.sendAction(RestApi::Actions::Ready);
}

void PresenterViewModel::transferTurn()
{
    m_api.sendAction(RestApi::Actions::TransferTurn);
}

void PresenterViewModel::resolveA()
{
    m_api.sendAction(RestApi::Actions::ResolveA);
}

void PresenterViewModel::resolveB()
{
    m_api.sendAction(RestApi::Actions::ResolveB);
}

void PresenterViewModel::rejectAll()
{
    m_api.sendAction(RestApi::Actions::RejectAll);
}

void PresenterViewModel::submitAnswer(const QString &text)
{
    m_api.submitText(text);
}

void PresenterViewModel::onAuthenticateFinished(bool success, const QString &errorMessage)
{
    Q_UNUSED(errorMessage);

    if (!success) {
        setConnectionState(QStringLiteral("error"));
        return;
    }

    setConnectionState(QStringLiteral("connected"));
    m_api.startHeartbeat();
    startPolling();
    pollCurrentPuzzle();
}

void PresenterViewModel::onCurrentPuzzleReceived(const QJsonObject &puzzle)
{
    applyPuzzleState(puzzle);
}

void PresenterViewModel::pollCurrentPuzzle()
{
    m_api.fetchCurrentPuzzle();
}

void PresenterViewModel::setConnectionState(const QString &state)
{
    if (m_connectionState == state)
        return;
    m_connectionState = state;
    emit connectionStateChanged();
}

void PresenterViewModel::applyPuzzleState(const QJsonObject &puzzle)
{
    const auto setString = [puzzle](const char *key) {
        return puzzle.value(QString::fromLatin1(key)).toString();
    };

    const QString roundTitle = setString("round_title");
    if (m_roundTitle != roundTitle) {
        m_roundTitle = roundTitle;
        emit roundTitleChanged();
    }

    const int puzzleNum = puzzle.value(QStringLiteral("puzzle_num")).toInt();
    if (m_puzzleNum != puzzleNum) {
        m_puzzleNum = puzzleNum;
        emit puzzleNumChanged();
    }

    const QString activeTeam = setString("active_team");
    if (m_activeTeam != activeTeam) {
        m_activeTeam = activeTeam;
        emit activeTeamChanged();
    }

    const QString gameStage = setString("game_stage");
    if (m_gameStage != gameStage) {
        m_gameStage = gameStage;
        emit gameStageChanged();
    }

    const QString layoutType = setString("layout_type");
    if (m_layoutType != layoutType) {
        m_layoutType = layoutType;
        emit layoutTypeChanged();
    }

    const QString hintText = setString("hint_text");
    if (m_hintText != hintText) {
        m_hintText = hintText;
        emit hintTextChanged();
    }

    const QString submittedAnswer = setString("submitted_answer");
    if (m_submittedAnswer != submittedAnswer) {
        m_submittedAnswer = submittedAnswer;
        emit submittedAnswerChanged();
    }

    const QString missingRevealText = setString("missing_reveal_text");
    if (m_missingRevealText != missingRevealText) {
        m_missingRevealText = missingRevealText;
        emit missingRevealTextChanged();
    }

    const QString correctAnswerText = setString("correct_answer_text");
    if (m_correctAnswerText != correctAnswerText) {
        m_correctAnswerText = correctAnswerText;
        emit correctAnswerTextChanged();
    }

    const bool answerWasCorrect = puzzle.value(QStringLiteral("answer_was_correct")).toBool();
    if (m_answerWasCorrect != answerWasCorrect) {
        m_answerWasCorrect = answerWasCorrect;
        emit answerWasCorrectChanged();
    }
}

void PresenterViewModel::startPolling()
{
    if (!m_pollTimer.isActive())
        m_pollTimer.start();
}

void PresenterViewModel::stopPolling()
{
    m_pollTimer.stop();
}
