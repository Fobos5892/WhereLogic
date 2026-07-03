#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <QVector>

#include "../models/DatabaseManager.h"

class DatabaseManager;
class NetworkServer;

class GameViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentStage READ currentStage NOTIFY currentStageChanged)
    Q_PROPERTY(QString currentSubState READ currentSubState NOTIFY currentSubStateChanged)
    Q_PROPERTY(QString layoutType READ layoutType NOTIFY layoutTypeChanged)
    Q_PROPERTY(QString roundTitle READ roundTitle NOTIFY roundTitleChanged)
    Q_PROPERTY(QString ruleText READ ruleText NOTIFY ruleTextChanged)
    Q_PROPERTY(int puzzleNumber READ puzzleNumber NOTIFY puzzleNumberChanged)
    Q_PROPERTY(int timerSeconds READ timerSeconds NOTIFY timerChanged)
    Q_PROPERTY(int timerMilliseconds READ timerMilliseconds NOTIFY timerChanged)
    Q_PROPERTY(bool showMilliseconds READ showMilliseconds NOTIFY timerChanged)
    Q_PROPERTY(QString activeTeam READ activeTeam NOTIFY activeTeamChanged)
    Q_PROPERTY(int roundScoreTeamA READ roundScoreTeamA NOTIFY roundScoreChanged)
    Q_PROPERTY(int roundScoreTeamB READ roundScoreTeamB NOTIFY roundScoreChanged)
    Q_PROPERTY(QString teamAName READ teamAName NOTIFY teamsChanged)
    Q_PROPERTY(QString teamBName READ teamBName NOTIFY teamsChanged)
    Q_PROPERTY(int totalScoreTeamA READ totalScoreTeamA NOTIFY teamsChanged)
    Q_PROPERTY(int totalScoreTeamB READ totalScoreTeamB NOTIFY teamsChanged)
    Q_PROPERTY(bool isRemoteConnected READ isRemoteConnected NOTIFY remoteConnectedChanged)
    Q_PROPERTY(bool showLocalControls READ showLocalControls NOTIFY showLocalControlsChanged)
    Q_PROPERTY(bool cardsFaceUp READ cardsFaceUp NOTIFY cardsFaceUpChanged)
    Q_PROPERTY(bool hasActiveSession READ hasActiveSession NOTIFY hasActiveSessionChanged)
    Q_PROPERTY(QString userAnswer READ userAnswer WRITE setUserAnswer NOTIFY userAnswerChanged)
    Q_PROPERTY(QString submittedAnswer READ submittedAnswer NOTIFY submittedAnswerChanged)
    Q_PROPERTY(QString missingRevealText READ missingRevealText NOTIFY missingRevealTextChanged)
    Q_PROPERTY(QString revealedAnswer READ revealedAnswer NOTIFY revealedAnswerChanged)
    Q_PROPERTY(QString hintText READ hintText NOTIFY hintTextChanged)
    Q_PROPERTY(bool answerWasCorrect READ answerWasCorrect NOTIFY answerWasCorrectChanged)
    Q_PROPERTY(bool isCorrectFlash READ isCorrectFlash NOTIFY evaluationFlashChanged)
    Q_PROPERTY(bool isWrongFlash READ isWrongFlash NOTIFY evaluationFlashChanged)
    Q_PROPERTY(QStringList quoteSlots READ quoteSlots NOTIFY quoteSlotsChanged)
    Q_PROPERTY(int currentPresetId READ currentPresetId NOTIFY currentPresetIdChanged)
    Q_PROPERTY(int currentPuzzleId READ currentPuzzleId NOTIFY currentPuzzleIdChanged)
    Q_PROPERTY(bool hasPuzzleMask READ hasPuzzleMask NOTIFY puzzleMaskChanged)

public:
    explicit GameViewModel(DatabaseManager *database, QObject *parent = nullptr);
    ~GameViewModel() override;

    void setNetworkServer(NetworkServer *server);

    QString currentStage() const { return m_stage; }
    QString currentSubState() const { return m_subState; }
    QString layoutType() const { return m_layoutType; }
    QString roundTitle() const { return m_roundTitle; }
    QString ruleText() const { return m_ruleText; }
    int puzzleNumber() const { return m_puzzleNumber; }
    int timerSeconds() const { return m_timerSeconds; }
    int timerMilliseconds() const { return m_timerMilliseconds; }
    bool showMilliseconds() const { return m_showMilliseconds; }
    QString activeTeam() const { return m_activeTeam; }
    int roundScoreTeamA() const { return m_roundScoreTeamA; }
    int roundScoreTeamB() const { return m_roundScoreTeamB; }
    QString teamAName() const { return m_teamAName; }
    QString teamBName() const { return m_teamBName; }
    int totalScoreTeamA() const { return m_totalScoreTeamA; }
    int totalScoreTeamB() const { return m_totalScoreTeamB; }
    bool isRemoteConnected() const { return m_remoteConnected; }
    bool showLocalControls() const { return m_showLocalControls; }
    bool cardsFaceUp() const { return m_cardsFaceUp; }
    bool hasActiveSession() const { return m_hasActiveSession; }
    QString userAnswer() const { return m_userAnswer; }
    QString submittedAnswer() const { return m_submittedAnswer; }
    QString missingRevealText() const { return m_missingRevealText; }
    QString revealedAnswer() const { return m_revealedAnswer; }
    QString hintText() const { return m_hintText; }
    bool answerWasCorrect() const { return m_answerWasCorrect; }
    bool isCorrectFlash() const { return m_isCorrectFlash; }
    bool isWrongFlash() const { return m_isWrongFlash; }
    QStringList quoteSlots() const { return m_quoteSlots; }
    int currentPresetId() const { return m_presetId; }
    int currentPuzzleId() const { return m_state.puzzleId; }
    bool hasPuzzleMask() const { return !m_puzzleMaskContour.isEmpty(); }

    void setUserAnswer(const QString &answer);

    Q_INVOKABLE QString puzzleImageUrl(int slotIndex = 0) const;
    Q_INVOKABLE QString puzzleHiddenImageUrl() const;
    Q_INVOKABLE QString puzzleDisplayImageUrl(int slotIndex = 0) const;

    Q_INVOKABLE void resumeSession();
    Q_INVOKABLE void clearSession();
    Q_INVOKABLE void startGame(int presetId);
    Q_INVOKABLE void configureTeams(const QString &teamA, const QString &teamB);
    Q_INVOKABLE void ready();
    Q_INVOKABLE void submitAnswer(const QString &text = {});
    Q_INVOKABLE void transferTurn();
    Q_INVOKABLE void resolveTeamA();
    Q_INVOKABLE void resolveTeamB();
    Q_INVOKABLE void rejectAll();
    Q_INVOKABLE void finishMissingReveal();
    Q_INVOKABLE void advanceAfterRound();

    Q_INVOKABLE QString label(const QString &key) const;

    QJsonObject currentPuzzlePayload() const;
    bool handleRemoteAction(const QString &action);

public slots:
    void onRemoteConnectedChanged(bool connected);

signals:
    void currentStageChanged();
    void currentSubStateChanged();
    void layoutTypeChanged();
    void roundTitleChanged();
    void ruleTextChanged();
    void puzzleNumberChanged();
    void timerChanged();
    void activeTeamChanged();
    void roundScoreChanged();
    void teamsChanged();
    void remoteConnectedChanged(bool connected);
    void showLocalControlsChanged(bool visible);
    void cardsFaceUpChanged(bool faceUp);
    void hasActiveSessionChanged(bool active);
    void userAnswerChanged();
    void submittedAnswerChanged();
    void missingRevealTextChanged();
    void revealedAnswerChanged();
    void hintTextChanged();
    void answerWasCorrectChanged();
    void evaluationFlashChanged();
    void quoteSlotsChanged();
    void currentPresetIdChanged();
    void currentPuzzleIdChanged();
    void puzzleMaskChanged();
    void gameFinished();

private:
    void loadPersistedState();
    void persistState();
    void setStage(const QString &stage);
    void setSubState(const QString &subState);
    void setRemoteConnected(bool connected);
    void updateLocalControlsVisibility();
    void loadCurrentRound();
    void loadCurrentPuzzle();
    void resetPuzzlePresentation();
    void startMainTimer();
    void startStealTimer();
    void stopTimer();
    void onTimerTick();
    void evaluateAnswer(const QString &answer);
    void enterMissingReveal(bool wasCorrect);
    void enterResolution();
    void awardPointToActiveTeam();
    void advancePuzzleOrRound();
    QString normalizeAnswer(const QString &answer) const;
    QString activeTeamIdLabel() const;

    DatabaseManager *m_database = nullptr;
    NetworkServer *m_networkServer = nullptr;
    QTimer m_gameTimer;
    QTimer m_flashTimer;

    GameStateSnapshot m_state;
    QVector<RoundInfo> m_rounds;
    QVector<PuzzleInfo> m_currentPuzzles;
    int m_roundIndex = 0;
    int m_puzzleIndex = 0;
    int m_presetId = 0;
    int m_teamAId = 0;
    int m_teamBId = 0;

    QString m_stage;
    QString m_subState;
    QString m_layoutType;
    QString m_roundTitle;
    QString m_ruleText;
    QString m_activeTeam;
    QString m_teamAName;
    QString m_teamBName;
    QString m_userAnswer;
    QString m_submittedAnswer;
    QString m_missingRevealText;
    QString m_revealedAnswer;
    QString m_hintText;
    QString m_correctAnswer;
    QString m_puzzleMaskContour;
    QStringList m_quoteSlots;

    int m_puzzleNumber = 0;
    int m_timerSeconds = 0;
    int m_timerMilliseconds = 0;
    int m_baseTimerSeconds = 60;
    int m_stealTimerSeconds = 15;
    bool m_showMilliseconds = false;
    bool m_isStealTurn = false;
    bool m_remoteConnected = false;
    bool m_showLocalControls = true;
    bool m_cardsFaceUp = false;
    bool m_hasActiveSession = false;
    bool m_answerWasCorrect = false;
    bool m_isCorrectFlash = false;
    bool m_isWrongFlash = false;
    int m_roundScoreTeamA = 0;
    int m_roundScoreTeamB = 0;
    int m_totalScoreTeamA = 0;
    int m_totalScoreTeamB = 0;
};
