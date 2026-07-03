#include <QtTest>
#include <QStandardPaths>

#include "models/DatabaseManager.h"

class TestDatabaseManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

    void testInitialize()
    {
        DatabaseManager db;
        QVERIFY(db.initialize());
        QVERIFY(!db.databasePath().isEmpty());
    }

    void testSeedPreset()
    {
        DatabaseManager db;
        QVERIFY(db.initialize());

        const QVector<GamePresetInfo> presets = db.listPresets();
        QVERIFY(!presets.isEmpty());

        const QVector<RoundInfo> rounds = db.listRoundsForPreset(presets.first().id);
        QVERIFY(!rounds.isEmpty());
    }
};

QTEST_MAIN(TestDatabaseManager)
#include "tst_databasemanager.moc"
