#include <QtTest>

#include "models/TriggerParser.h"

class TestTriggerParser : public QObject
{
    Q_OBJECT

private slots:
    void testRussianTrigger()
    {
        const TriggerParseResult result =
            TriggerParser::parse(QString::fromUtf8(u8"Наш ответ: Москва"));
        QVERIFY(result.triggered);
        QCOMPARE(result.answer, QString::fromUtf8(u8"Москва"));
    }

    void testEnglishTrigger()
    {
        const TriggerParseResult result = TriggerParser::parse(QStringLiteral("Our answer is: Paris"));
        QVERIFY(result.triggered);
        QCOMPARE(result.answer, QStringLiteral("Paris"));
    }

    void testNoTrigger()
    {
        const TriggerParseResult result = TriggerParser::parse(QStringLiteral("просто текст"));
        QVERIFY(!result.triggered);
    }
};

QTEST_MAIN(TestTriggerParser)
#include "tst_triggerparser.moc"
