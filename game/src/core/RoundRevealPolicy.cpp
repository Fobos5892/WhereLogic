#include "RoundRevealPolicy.h"

#include "GameConstants.h"

RevealBehavior RoundRevealPolicy::behaviorForLayout(const QString &layoutType)
{
    if (layoutType == GameConstants::LayoutType::Standard
        || layoutType == GameConstants::LayoutType::SingleHybrid) {
        return RevealBehavior::StaggerCards;
    }
    if (layoutType == GameConstants::LayoutType::Equation) {
        return RevealBehavior::EquationReveal;
    }
    if (layoutType == GameConstants::LayoutType::FullMask) {
        return RevealBehavior::MaskFadeOut;
    }
    if (layoutType == GameConstants::LayoutType::Chronology) {
        return RevealBehavior::ChronologyReorder;
    }
    if (layoutType == GameConstants::LayoutType::TextOnly) {
        return RevealBehavior::QuoteHighlight;
    }
    if (layoutType == GameConstants::LayoutType::BlitzStandard) {
        return RevealBehavior::BlitzStagger;
    }
    return RevealBehavior::None;
}

bool RoundRevealPolicy::requiresMissingReveal(const QString &layoutType)
{
    return behaviorForLayout(layoutType) != RevealBehavior::None;
}

QString RoundRevealPolicy::missingRevealTemplate(const QString &layoutType)
{
    if (layoutType == GameConstants::LayoutType::FullMask) {
        return QStringLiteral("Не хватало: %1");
    }
    if (layoutType == GameConstants::LayoutType::Standard
        || layoutType == GameConstants::LayoutType::BlitzStandard) {
        return QStringLiteral("Общее: %1");
    }
    if (layoutType == GameConstants::LayoutType::Equation) {
        return QStringLiteral("Результат: %1");
    }
    if (layoutType == GameConstants::LayoutType::SingleHybrid) {
        return QStringLiteral("Гибрид: %1");
    }
    if (layoutType == GameConstants::LayoutType::Chronology) {
        return QStringLiteral("Порядок: %1");
    }
    if (layoutType == GameConstants::LayoutType::TextOnly) {
        return QStringLiteral("Отгадка: %1");
    }
    return QStringLiteral("%1");
}

QString RoundRevealPolicy::formatMissingRevealText(const QString &layoutType, const QString &detail)
{
    return missingRevealTemplate(layoutType).arg(detail);
}
