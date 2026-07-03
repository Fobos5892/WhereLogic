#pragma once

#include "RestApiClient.h"

#include <QJsonObject>
#include <QObject>
#include <QTimer>

class PresenterViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString serverHost READ serverHost WRITE setServerHost NOTIFY serverHostChanged)
    Q_PROPERTY(QString pin READ pin WRITE setPin NOTIFY pinChanged)
    Q_PROPERTY(QString roundTitle READ roundTitle NOTIFY roundTitleChanged)
    Q_PROPERTY(int puzzleNum READ puzzleNum NOTIFY puzzleNumChanged)
    Q_PROPERTY(QString activeTeam READ activeTeam NOTIFY activeTeamChanged)
    Q_PROPERTY(QString gameStage READ gameStage NOTIFY gameStageChanged)
    Q_PROPERTY(QString layoutType READ layoutType NOTIFY layoutTypeChanged)
    Q_PROPERTY(QString hintText READ hintText NOTIFY hintTextChanged)
    Q_PROPERTY(QString submittedAnswer READ submittedAnswer NOTIFY submittedAnswerChanged)
    Q_PROPERTY(QString missingRevealText READ missingRevealText NOTIFY missingRevealTextChanged)
    Q_PROPERTY(QString correctAnswerText READ correctAnswerText NOTIFY correctAnswerTextChanged)
    Q_PROPERTY(bool answerWasCorrect READ answerWasCorrect NOTIFY answerWasCorrectChanged)

public:
    explicit PresenterViewModel(QObject *parent = nullptr);

    QString connectionState() const { return m_connectionState; }
    QString serverHost() const { return m_serverHost; }
    QString pin() const { return m_pin; }
    QString roundTitle() const { return m_roundTitle; }
    int puzzleNum() const { return m_puzzleNum; }
    QString activeTeam() const { return m_activeTeam; }
    QString gameStage() const { return m_gameStage; }
    QString layoutType() const { return m_layoutType; }
    QString hintText() const { return m_hintText; }
    QString submittedAnswer() const { return m_submittedAnswer; }
    QString missingRevealText() const { return m_missingRevealText; }
    QString correctAnswerText() const { return m_correctAnswerText; }
    bool answerWasCorrect() const { return m_answerWasCorrect; }

    void setServerHost(const QString &host);
    void setPin(const QString &pin);

    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void ready();
    Q_INVOKABLE void transferTurn();
    Q_INVOKABLE void resolveA();
    Q_INVOKABLE void resolveB();
    Q_INVOKABLE void rejectAll();
    Q_INVOKABLE void submitAnswer(const QString &text);

signals:
    void connectionStateChanged();
    void serverHostChanged();
    void pinChanged();
    void roundTitleChanged();
    void puzzleNumChanged();
    void activeTeamChanged();
    void gameStageChanged();
    void layoutTypeChanged();
    void hintTextChanged();
    void submittedAnswerChanged();
    void missingRevealTextChanged();
    void correctAnswerTextChanged();
    void answerWasCorrectChanged();

private slots:
    void onAuthenticateFinished(bool success, const QString &errorMessage);
    void onCurrentPuzzleReceived(const QJsonObject &puzzle);
    void pollCurrentPuzzle();

private:
    void setConnectionState(const QString &state);
    void applyPuzzleState(const QJsonObject &puzzle);
    void startPolling();
    void stopPolling();

    RestApiClient m_api;
    QTimer m_pollTimer;
    QString m_connectionState;
    QString m_serverHost;
    QString m_pin;
    QString m_roundTitle;
    int m_puzzleNum = 0;
    QString m_activeTeam;
    QString m_gameStage;
    QString m_layoutType;
    QString m_hintText;
    QString m_submittedAnswer;
    QString m_missingRevealText;
    QString m_correctAnswerText;
    bool m_answerWasCorrect = false;
};
