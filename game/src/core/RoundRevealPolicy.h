#pragma once

#include <QString>

enum class RevealBehavior {
    None,
    StaggerCards,
    MaskFadeOut,
    EquationReveal,
    HybridPulse,
    ChronologyReorder,
    QuoteHighlight,
    BlitzStagger
};

class RoundRevealPolicy
{
public:
    static RevealBehavior behaviorForLayout(const QString &layoutType);
    static bool requiresMissingReveal(const QString &layoutType);
    static QString missingRevealTemplate(const QString &layoutType);
    static QString formatMissingRevealText(const QString &layoutType, const QString &detail);
};
