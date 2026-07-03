#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

class RestApiClient : public QObject
{
    Q_OBJECT

public:
    explicit RestApiClient(QObject *parent = nullptr);

    void setServerHost(const QString &host);
    QString serverHost() const { return m_serverHost; }

    void setAuthToken(const QString &token);
    QString authToken() const { return m_authToken; }

    void startHeartbeat();
    void stopHeartbeat();

public slots:
    void authenticate(const QString &pin);
    void sendHeartbeat();
    void fetchCurrentPuzzle();
    void sendAction(const QString &action);
    void submitText(const QString &text);

signals:
    void authenticateFinished(bool success, const QString &errorMessage);
    void heartbeatFinished(bool success);
    void currentPuzzleReceived(const QJsonObject &puzzle);
    void currentPuzzleFailed(const QString &errorMessage);
    void actionFinished(bool success, const QString &errorMessage);
    void submitTextFinished(bool success, const QString &errorMessage);

private:
    QNetworkRequest buildRequest(const QString &path) const;
    void applyAuthHeader(QNetworkRequest &request) const;

    QNetworkAccessManager m_network;
    QTimer m_heartbeatTimer;
    QString m_serverHost;
    QString m_authToken;
};
