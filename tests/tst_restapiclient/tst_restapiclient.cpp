#include <QtTest>

#include "RestApiClient.h"
#include "RestApiConstants.h"

class TestRestApiClient : public QObject
{
    Q_OBJECT

private slots:
    void testServerHostNormalization()
    {
        RestApiClient client;
        client.setServerHost(QStringLiteral("http://127.0.0.1:8765/"));
        QCOMPARE(client.serverHost(), QStringLiteral("http://127.0.0.1:8765"));

        RestApiClient bareHost;
        bareHost.setServerHost(QStringLiteral("192.168.0.1"));
        QCOMPARE(bareHost.serverHost(), QStringLiteral("http://192.168.0.1:8765"));
    }

    void testAuthTokenStorage()
    {
        RestApiClient client;
        client.setAuthToken(QStringLiteral("token-123"));
        QCOMPARE(client.authToken(), QStringLiteral("token-123"));
    }

    void testApiPathsDefined()
    {
        QVERIFY(!RestApi::Paths::Authenticate.isEmpty());
        QVERIFY(!RestApi::Paths::Heartbeat.isEmpty());
        QVERIFY(!RestApi::Actions::Ready.isEmpty());
    }
};

QTEST_MAIN(TestRestApiClient)
#include "tst_restapiclient.moc"
