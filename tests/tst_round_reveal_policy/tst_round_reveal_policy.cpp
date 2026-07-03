#include <QtTest>

#include "core/GameConstants.h"
#include "core/RoundRevealPolicy.h"

class TestRoundRevealPolicy : public QObject
{
    Q_OBJECT

private slots:
    void testStandardLayoutBehavior()
    {
        QCOMPARE(RoundRevealPolicy::behaviorForLayout(GameConstants::LayoutType::Standard),
                 RevealBehavior::StaggerCards);
        QVERIFY(RoundRevealPolicy::requiresMissingReveal(GameConstants::LayoutType::Standard));
    }

    void testMissingRevealFormatting()
    {
        const QString text = RoundRevealPolicy::formatMissingRevealText(
            GameConstants::LayoutType::Equation, QStringLiteral("42"));
        QVERIFY(text.contains(QStringLiteral("42")));
    }
};

QTEST_MAIN(TestRoundRevealPolicy)
#include "tst_round_reveal_policy.moc"
