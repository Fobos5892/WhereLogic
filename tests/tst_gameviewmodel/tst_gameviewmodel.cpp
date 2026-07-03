#include <QtTest>
#include <QStandardPaths>

#include "core/GameConstants.h"
#include "models/DatabaseManager.h"
#include "viewmodels/GameViewModel.h"

class TestGameViewModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

    void testStartGameMovesToTeamSetup()
    {
        DatabaseManager db;
        QVERIFY(db.initialize());

        GameViewModel vm(&db);
        const QVector<GamePresetInfo> presets = db.listPresets();
        QVERIFY(!presets.isEmpty());

        vm.startGame(presets.first().id);
        QCOMPARE(vm.currentStage(), GameConstants::Stage::TeamSetup);
        QVERIFY(vm.hasActiveSession());
    }

    void testConfigureTeams()
    {
        DatabaseManager db;
        QVERIFY(db.initialize());

        GameViewModel vm(&db);
        vm.startGame(db.listPresets().first().id);
        vm.configureTeams(QStringLiteral("Alpha"), QStringLiteral("Beta"));

        QCOMPARE(vm.teamAName(), QStringLiteral("Alpha"));
        QCOMPARE(vm.teamBName(), QStringLiteral("Beta"));
    }
};

QTEST_MAIN(TestGameViewModel)
#include "tst_gameviewmodel.moc"
