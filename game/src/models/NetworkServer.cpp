#include "NetworkServer.h"

#include "../core/GameConstants.h"
#include "../viewmodels/GameViewModel.h"

#include "RestApiConstants.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QTcpServer>

namespace {

QString generatePin(int length)
{
    static const QString alphabet = QStringLiteral("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
    QString pin;
    pin.reserve(length);
    for (int i = 0; i < length; ++i) {
        const int index = QRandomGenerator::global()->bounded(alphabet.size());
        pin.append(alphabet.at(index));
    }
    return pin;
}

} // namespace

NetworkServer::NetworkServer(QObject *parent)
    : QObject(parent)
{
    m_pinRotationTimer.setInterval(GameConstants::PIN_ROTATION_INTERVAL_MS);
    connect(&m_pinRotationTimer, &QTimer::timeout, this, &NetworkServer::rotatePin);

    m_idleCheckTimer.setInterval(RestApi::REMOTE_IDLE_CHECK_MS);
    connect(&m_idleCheckTimer, &QTimer::timeout, this, &NetworkServer::checkIdleSessions);
}

NetworkServer::~NetworkServer()
{
    stop();
}

void NetworkServer::setGameViewModel(GameViewModel *viewModel)
{
    m_viewModel = viewModel;
}

void NetworkServer::registerRoutes()
{
    m_server->route(RestApi::Paths::Authenticate, QHttpServerRequest::Method::Post,
                    [this](const QHttpServerRequest &request) {
                        return handleAuthenticate(request);
                    });

    m_server->route(RestApi::Paths::Heartbeat, QHttpServerRequest::Method::Post,
                    [this](const QHttpServerRequest &request) {
                        return handleAuthenticatedRequest(request, [this](const QString &) {
                            Q_UNUSED(m_viewModel)
                            return makeJsonResponse({{QStringLiteral("status"), QStringLiteral("acknowledged")}},
                                                    QHttpServerResponse::StatusCode::Ok);
                        });
                    });

    m_server->route(RestApi::Paths::CurrentPuzzle, QHttpServerRequest::Method::Get,
                    [this](const QHttpServerRequest &request) {
                        return handleAuthenticatedRequest(request, [this](const QString &) {
                            if (!m_viewModel) {
                                return makeJsonResponse(
                                    {{QStringLiteral("error"), QStringLiteral("Game unavailable")}},
                                    QHttpServerResponse::StatusCode::ServiceUnavailable);
                            }
                            return makeJsonResponse(m_viewModel->currentPuzzlePayload(),
                                                    QHttpServerResponse::StatusCode::Ok);
                        });
                    });

    m_server->route(RestApi::Paths::Action, QHttpServerRequest::Method::Post,
                    [this](const QHttpServerRequest &request) {
                        const QByteArray body = request.body();
                        return handleAuthenticatedRequest(request, [this, body](const QString &) {
                            if (!m_viewModel) {
                                return makeJsonResponse(
                                    {{QStringLiteral("error"), QStringLiteral("Game unavailable")}},
                                    QHttpServerResponse::StatusCode::ServiceUnavailable);
                            }
                            const QJsonDocument doc = QJsonDocument::fromJson(body);
                            const QString action = doc.object().value(QStringLiteral("action")).toString();
                            if (!m_viewModel->handleRemoteAction(action)) {
                                return makeJsonResponse(
                                    {{QStringLiteral("error"), QStringLiteral("Unknown action")}},
                                    QHttpServerResponse::StatusCode::BadRequest);
                            }
                            return makeJsonResponse({{QStringLiteral("status"), QStringLiteral("ok")}},
                                                    QHttpServerResponse::StatusCode::Ok);
                        });
                    });

    m_server->route(RestApi::Paths::SubmitText, QHttpServerRequest::Method::Post,
                    [this](const QHttpServerRequest &request) {
                        const QByteArray body = request.body();
                        return handleAuthenticatedRequest(request, [this, body](const QString &) {
                            if (!m_viewModel) {
                                return makeJsonResponse(
                                    {{QStringLiteral("error"), QStringLiteral("Game unavailable")}},
                                    QHttpServerResponse::StatusCode::ServiceUnavailable);
                            }
                            const QJsonDocument doc = QJsonDocument::fromJson(body);
                            const QString text = doc.object().value(QStringLiteral("text")).toString();
                            m_viewModel->submitAnswer(text);
                            return makeJsonResponse({{QStringLiteral("status"), QStringLiteral("accepted")}},
                                                    QHttpServerResponse::StatusCode::Ok);
                        });
                    });
}

bool NetworkServer::start(quint16 port)
{
    rotatePin();
    m_pinRotationTimer.start();
    m_idleCheckTimer.start();

    m_server = std::make_unique<QHttpServer>();
    registerRoutes();

    auto *tcpServer = new QTcpServer();
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        delete tcpServer;
        m_server.reset();
        return false;
    }
    if (!m_server->bind(tcpServer)) {
        delete tcpServer;
        m_server.reset();
        return false;
    }
    return true;
}

void NetworkServer::stop()
{
    m_pinRotationTimer.stop();
    m_idleCheckTimer.stop();
    m_sessions.clear();
    setRemoteConnected(false);
    m_server.reset();
}

void NetworkServer::rotatePin()
{
    m_currentPin = generatePin(GameConstants::PIN_LENGTH);
    emit currentPinChanged(m_currentPin);
}

void NetworkServer::checkIdleSessions()
{
    if (m_sessions.isEmpty()) {
        return;
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    QStringList expired;
    for (auto it = m_sessions.constBegin(); it != m_sessions.constEnd(); ++it) {
        if (it->lastTouched.msecsTo(now) > RestApi::REMOTE_IDLE_TIMEOUT_MS) {
            expired.append(it.key());
        }
    }

    for (const QString &token : expired) {
        invalidateSession(token);
    }
}

bool NetworkServer::touchSession(const QString &token)
{
    auto it = m_sessions.find(token);
    if (it == m_sessions.end()) {
        return false;
    }
    it->lastTouched = QDateTime::currentDateTimeUtc();
    if (!m_remoteConnected) {
        setRemoteConnected(true);
    }
    return true;
}

void NetworkServer::invalidateSession(const QString &token)
{
    if (!m_sessions.remove(token)) {
        return;
    }
    if (m_sessions.isEmpty()) {
        setRemoteConnected(false);
        emit clientDisconnected();
    }
}

void NetworkServer::setRemoteConnected(bool connected)
{
    if (m_remoteConnected == connected) {
        return;
    }
    m_remoteConnected = connected;
    emit remoteConnectedChanged(m_remoteConnected);
}

QHttpServerResponse NetworkServer::makeJsonResponse(const QJsonObject &body,
                                                    QHttpServerResponse::StatusCode status) const
{
    return QHttpServerResponse(QByteArray("application/json"),
                             QJsonDocument(body).toJson(QJsonDocument::Compact),
                             status);
}

QHttpServerResponse NetworkServer::unauthorized(const QString &message) const
{
    return makeJsonResponse({{QStringLiteral("error"), message}},
                            QHttpServerResponse::StatusCode::Unauthorized);
}

QHttpServerResponse NetworkServer::handleAuthenticate(const QHttpServerRequest &request)
{
    const QJsonDocument doc = QJsonDocument::fromJson(request.body());
    const QString pin = doc.object().value(QStringLiteral("pin")).toString().trimmed().toUpper();
    if (pin.isEmpty() || pin != m_currentPin) {
        return unauthorized(QStringLiteral("Invalid PIN"));
    }

    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    AuthSession session;
    session.token = token;
    session.lastTouched = QDateTime::currentDateTimeUtc();
    m_sessions.insert(token, session);
    setRemoteConnected(true);
    emit clientAuthenticated();

    return makeJsonResponse(
        {{QStringLiteral("token"), token}, {QStringLiteral("status"), QStringLiteral("authorized")}},
        QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse NetworkServer::handleAuthenticatedRequest(
    const QHttpServerRequest &request,
    const std::function<QHttpServerResponse(const QString &)> &handler)
{
    const QString token = extractToken(request);
    if (token.isEmpty() || !touchSession(token)) {
        return unauthorized(QStringLiteral("Invalid or expired token"));
    }
    return handler(token);
}

QString NetworkServer::extractToken(const QHttpServerRequest &request) const
{
    return QString::fromUtf8(request.value(RestApi::AUTH_HEADER.toUtf8()));
}
