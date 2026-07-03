#include "RestApiClient.h"

#include "RestApiConstants.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrl>

RestApiClient::RestApiClient(QObject *parent)
    : QObject(parent)
{
    m_heartbeatTimer.setInterval(RestApi::HEARTBEAT_INTERVAL_MS);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &RestApiClient::sendHeartbeat);
}

void RestApiClient::setServerHost(const QString &host)
{
    QString normalized = host.trimmed();
    if (normalized.isEmpty()) {
        m_serverHost.clear();
        return;
    }

    if (!normalized.contains(QStringLiteral("://")))
        normalized.prepend(QStringLiteral("http://"));

    if (normalized.endsWith(QLatin1Char('/')))
        normalized.chop(1);

    const QUrl url(normalized);
    if (url.port() == -1)
        normalized += QStringLiteral(":") + QString::number(RestApi::DEFAULT_SERVER_PORT);

    m_serverHost = normalized;
}

void RestApiClient::setAuthToken(const QString &token)
{
    m_authToken = token;
}

void RestApiClient::startHeartbeat()
{
    if (!m_heartbeatTimer.isActive())
        m_heartbeatTimer.start();
    sendHeartbeat();
}

void RestApiClient::stopHeartbeat()
{
    m_heartbeatTimer.stop();
}

void RestApiClient::authenticate(const QString &pin)
{
    QNetworkRequest request = buildRequest(RestApi::Paths::Authenticate);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject body;
    body.insert(QStringLiteral("pin"), pin);

    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit authenticateFinished(false, reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonObject obj = doc.object();
        const QString token = obj.value(QStringLiteral("token")).toString();
        if (token.isEmpty()) {
            emit authenticateFinished(false, QStringLiteral("Authentication failed"));
            return;
        }

        setAuthToken(token);
        emit authenticateFinished(true, {});
    });
}

void RestApiClient::sendHeartbeat()
{
    if (m_authToken.isEmpty())
        return;

    QNetworkRequest request = buildRequest(RestApi::Paths::Heartbeat);
    QNetworkReply *reply = m_network.post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const bool success = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        emit heartbeatFinished(success);
    });
}

void RestApiClient::fetchCurrentPuzzle()
{
    if (m_authToken.isEmpty())
        return;

    QNetworkRequest request = buildRequest(RestApi::Paths::CurrentPuzzle);
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            const QString error = reply->errorString();
            reply->deleteLater();
            emit currentPuzzleFailed(error);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        reply->deleteLater();
        emit currentPuzzleReceived(doc.object());
    });
}

void RestApiClient::sendAction(const QString &action)
{
    if (m_authToken.isEmpty())
        return;

    QNetworkRequest request = buildRequest(RestApi::Paths::Action);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject body;
    body.insert(QStringLiteral("action"), action);

    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const bool success = reply->error() == QNetworkReply::NoError;
        const QString error = success ? QString() : reply->errorString();
        reply->deleteLater();
        emit actionFinished(success, error);
    });
}

void RestApiClient::submitText(const QString &text)
{
    if (m_authToken.isEmpty())
        return;

    QNetworkRequest request = buildRequest(RestApi::Paths::SubmitText);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject body;
    body.insert(QStringLiteral("text"), text);

    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const bool success = reply->error() == QNetworkReply::NoError;
        const QString error = success ? QString() : reply->errorString();
        reply->deleteLater();
        emit submitTextFinished(success, error);
    });
}

QNetworkRequest RestApiClient::buildRequest(const QString &path) const
{
    const QString url = m_serverHost + path;
    QNetworkRequest request{QUrl(url)};
    applyAuthHeader(request);
    return request;
}

void RestApiClient::applyAuthHeader(QNetworkRequest &request) const
{
    if (!m_authToken.isEmpty())
        request.setRawHeader(RestApi::AUTH_HEADER.toUtf8(), m_authToken.toUtf8());
}
