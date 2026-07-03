#pragma once

#include <QString>

struct TriggerParseResult {
    bool triggered = false;
    QString answer;
};

class TriggerParser
{
public:
    static TriggerParseResult parse(const QString &transcript);
};
