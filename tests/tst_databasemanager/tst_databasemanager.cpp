#include <QtTest>

#include "TestEnvironment.h"
#include "models/DatabaseManager.h"

class TestDatabaseManager : public QObject
{
    Q_OBJECT

private slots:
    void testInitialize()
    {
        DatabaseManager db;
        QString err;
        QObject::connect(&db, &DatabaseManager::databaseError, [&](const QString &m) { err = m; });
        QVERIFY2(db.initialize(), qPrintable(err.isEmpty() ? QStringLiteral("unknown error") : err));
        QVERIFY(!db.databasePath().isEmpty());
    }

    void testSeedPreset()
    {
        DatabaseManager db;
        QString err;
        QObject::connect(&db, &DatabaseManager::databaseError, [&](const QString &m) { err = m; });
        QVERIFY2(db.initialize(), qPrintable(err.isEmpty() ? QStringLiteral("unknown error") : err));

        const QVector<GamePresetInfo> presets = db.listPresets();
        QVERIFY(!presets.isEmpty());

        const QVector<RoundInfo> rounds = db.listRoundsForPreset(presets.first().id);
        QVERIFY(!rounds.isEmpty());
    }
};

WHERLOGIC_QTEST_MAIN(TestDatabaseManager)
#include "tst_databasemanager.moc"
