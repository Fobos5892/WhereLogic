#include "TriggerParser.h"

#include <QRegularExpression>

namespace {

const QString kRussianTrigger = QStringLiteral(u"Наш ответ:");
const QString kEnglishTrigger = QStringLiteral("Our answer is:");

QString extractAnswer(const QString &transcript, const QString &trigger)
{
    const int index = transcript.indexOf(trigger, 0, Qt::CaseInsensitive);
    if (index < 0) {
        return {};
    }

    QString remainder = transcript.mid(index + trigger.size()).trimmed();
    remainder.remove(QRegularExpression(QStringLiteral(R"(^[:\-\s]+)")));
    return remainder.trimmed();
}

} // namespace

TriggerParseResult TriggerParser::parse(const QString &transcript)
{
    TriggerParseResult result;

    const QString russianAnswer = extractAnswer(transcript, kRussianTrigger);
    if (!russianAnswer.isEmpty()) {
        result.triggered = true;
        result.answer = russianAnswer;
        return result;
    }

    const QString englishAnswer = extractAnswer(transcript, kEnglishTrigger);
    if (!englishAnswer.isEmpty()) {
        result.triggered = true;
        result.answer = englishAnswer;
        return result;
    }

    return result;
}
