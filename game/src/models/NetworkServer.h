#pragma once

#include "RestApiConstants.h"

#include <QDateTime>
#include <QHash>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QString>
#include <QUuid>

#include <functional>
#include <memory>

class GameViewModel;

struct AuthSession {
    QString token;
    QDateTime lastTouched;
};

class NetworkServer : public QObject
{
    Q_OBJECT

public:
    explicit NetworkServer(QObject *parent = nullptr);
    ~NetworkServer() override;

    void setGameViewModel(GameViewModel *viewModel);

    Q_PROPERTY(QString currentPin READ currentPin NOTIFY currentPinChanged)
    Q_PROPERTY(bool remoteConnected READ isRemoteConnected NOTIFY remoteConnectedChanged)

    QString currentPin() const { return m_currentPin; }
    bool isRemoteConnected() const { return m_remoteConnected; }

    bool start(quint16 port = static_cast<quint16>(RestApi::DEFAULT_SERVER_PORT));
    void stop();

signals:
    void currentPinChanged(const QString &pin);
    void remoteConnectedChanged(bool connected);
    void clientAuthenticated();
    void clientDisconnected();

private:
    void rotatePin();
    void checkIdleSessions();
    bool touchSession(const QString &token);
    void invalidateSession(const QString &token);
    void setRemoteConnected(bool connected);
    void registerRoutes();

    QHttpServerResponse makeJsonResponse(const QJsonObject &body,
                                         QHttpServerResponse::StatusCode status) const;
    QHttpServerResponse unauthorized(const QString &message) const;
    QHttpServerResponse handleAuthenticate(const QHttpServerRequest &request);
    QHttpServerResponse handleAuthenticatedRequest(
        const QHttpServerRequest &request,
        const std::function<QHttpServerResponse(const QString &)> &handler);
    QString extractToken(const QHttpServerRequest &request) const;

    std::unique_ptr<QHttpServer> m_server;
    GameViewModel *m_viewModel = nullptr;
    QTimer m_pinRotationTimer;
    QTimer m_idleCheckTimer;
    QString m_currentPin;
    bool m_remoteConnected = false;
    QHash<QString, AuthSession> m_sessions;
};
