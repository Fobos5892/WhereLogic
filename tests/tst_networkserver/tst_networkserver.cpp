#include <QtTest>

#include "TestEnvironment.h"
#include "models/DatabaseManager.h"
#include "models/NetworkServer.h"
#include "viewmodels/GameViewModel.h"

class TestNetworkServer : public QObject
{
    Q_OBJECT

private slots:
    void testPinGenerated()
    {
        DatabaseManager db;
        QVERIFY(db.initialize());

        GameViewModel vm(&db);
        NetworkServer server;
        server.setGameViewModel(&vm);

        QVERIFY(server.start(18765));
        QCOMPARE(server.currentPin().size(), 5);
        QVERIFY(!server.currentPin().isEmpty());
        server.stop();
    }
};

WHERLOGIC_QTEST_MAIN(TestNetworkServer)
#include "tst_networkserver.moc"
