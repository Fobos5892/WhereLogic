#include "AdminViewModel.h"

#include "../core/GameConstants.h"
#include "../models/DatabaseManager.h"
#include "../models/ImageProcessor.h"
#include "../models/PuzzleImageProvider.h"

#include <QBuffer>
#include <QFile>
#include <QFutureWatcher>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QVariantMap>
#include <QtConcurrent>

#include <algorithm>

namespace {
const QLatin1String kHybridAnimPrefix("__hybrid_anim:");

QString normalizeHybridAnimStyleValue(const QString &style)
{
    const QString lowered = style.trimmed().toLower();
    if (lowered == QStringLiteral("aggressive")) {
        return QStringLiteral("aggressive");
    }
    return QStringLiteral("soft");
}

QString parseHybridAnimStyleFromCorrectOrder(const QString &correctOrderJson)
{
    if (correctOrderJson.isEmpty()) {
        return QStringLiteral("soft");
    }
    const QJsonDocument doc = QJsonDocument::fromJson(correctOrderJson.toUtf8());
    if (!doc.isArray()) {
        return QStringLiteral("soft");
    }
    for (const QJsonValue &value : doc.array()) {
        const QString token = value.toString().trimmed();
        if (token.startsWith(kHybridAnimPrefix)) {
            return normalizeHybridAnimStyleValue(token.mid(kHybridAnimPrefix.size()));
        }
    }
    return QStringLiteral("soft");
}
} // namespace

struct PuzzleSaveOutcome {
    int puzzleId = 0;
    bool success = false;
    QString errorMessage;
};

QString AdminViewModel::maskContour() const
{
    if (m_editorMasks.isEmpty()) {
        return {};
    }
    return m_editorMasks.last().contour;
}

QString AdminViewModel::normalizeAnswerKey(const QString &answer)
{
    return answer.trimmed().toLower();
}

QString AdminViewModel::formatGroupedAnswerLabel(const QString &answer, const QList<int> &maskNumbers)
{
    QList<int> numbers = maskNumbers;
    std::sort(numbers.begin(), numbers.end());
    QStringList parts;
    parts.reserve(numbers.size());
    for (int number : numbers) {
        parts.append(QString::number(number));
    }
    return QStringLiteral("%1 (%2)").arg(answer.trimmed(), parts.join(QStringLiteral(", ")));
}

void AdminViewModel::regenerateGroupedAnswerOptions()
{
    if (!isPhotoMaskRound()) {
        return;
    }

    QHash<QString, QString> displayByKey;
    QHash<QString, QList<int>> numbersByKey;
    for (const EditorMaskEntry &mask : m_editorMasks) {
        const QString key = normalizeAnswerKey(mask.answer);
        if (key.isEmpty()) {
            continue;
        }
        if (!displayByKey.contains(key)) {
            displayByKey.insert(key, mask.answer.trimmed());
        }
        numbersByKey[key].append(mask.number);
    }

    QStringList grouped;
    grouped.reserve(displayByKey.size());
    for (auto it = displayByKey.constBegin(); it != displayByKey.constEnd(); ++it) {
        grouped.append(formatGroupedAnswerLabel(it.value(), numbersByKey.value(it.key())));
    }
    std::sort(grouped.begin(), grouped.end(), [](const QString &a, const QString &b) {
        return a.localeAwareCompare(b) < 0;
    });

    m_answerOptions = grouped;
    emit answerOptionsChanged();
}

void AdminViewModel::loadEditorMasks(int puzzleId)
{
    m_editorMasks.clear();
    if (!m_database || puzzleId <= 0) {
        emit masksChanged();
        return;
    }

    const QVector<PuzzleMaskInfo> masks = m_database->listPuzzleMasks(puzzleId);
    m_editorMasks.reserve(masks.size());
    for (const PuzzleMaskInfo &mask : masks) {
        EditorMaskEntry entry;
        entry.number = mask.sortOrder;
        entry.contour = mask.contourPoints;
        entry.answer = mask.answerText;
        m_editorMasks.append(entry);
    }

    if (m_editorMasks.isEmpty() && m_selectedTemplateId > 0) {
        const QString contour = m_database->maskTemplateContour(m_selectedTemplateId);
        if (!contour.isEmpty()) {
            EditorMaskEntry entry;
            entry.number = 1;
            entry.contour = contour;
            entry.answer = m_editAnswer;
            m_editorMasks.append(entry);
        }
    }

    emit masksChanged();
}

QVector<PuzzleMaskInfo> AdminViewModel::editorMasksForSave() const
{
    QVector<PuzzleMaskInfo> masks;
    masks.reserve(m_editorMasks.size());
    for (const EditorMaskEntry &entry : m_editorMasks) {
        if (entry.contour.trimmed().isEmpty()) {
            continue;
        }
        PuzzleMaskInfo mask;
        mask.sortOrder = entry.number;
        mask.answerText = entry.answer.trimmed();
        mask.contourPoints = entry.contour;
        masks.append(mask);
    }
    return masks;
}

int AdminViewModel::maskNumberAt(int index) const
{
    if (index < 0 || index >= m_editorMasks.size()) {
        return 0;
    }
    return m_editorMasks.at(index).number;
}

QString AdminViewModel::maskContourAt(int index) const
{
    if (index < 0 || index >= m_editorMasks.size()) {
        return {};
    }
    return m_editorMasks.at(index).contour;
}

QString AdminViewModel::maskAnswerAt(int index) const
{
    if (index < 0 || index >= m_editorMasks.size()) {
        return {};
    }
    return m_editorMasks.at(index).answer;
}

void AdminViewModel::setMaskAnswerAt(int index, const QString &text)
{
    if (index < 0 || index >= m_editorMasks.size()) {
        return;
    }
    if (m_editorMasks[index].answer == text) {
        return;
    }
    m_editorMasks[index].answer = text;
    regenerateGroupedAnswerOptions();
    emit masksChanged();
}

void AdminViewModel::removeMaskAt(int index)
{
    if (index < 0 || index >= m_editorMasks.size()) {
        return;
    }
    m_editorMasks.removeAt(index);
    for (int i = 0; i < m_editorMasks.size(); ++i) {
        m_editorMasks[i].number = i + 1;
    }
    regenerateGroupedAnswerOptions();
    emit masksChanged();
    refreshEditorPreview();
}

AdminViewModel::AdminViewModel(DatabaseManager *database, QObject *parent)
    : QObject(parent)
    , m_database(database)
{
    refreshPresets();
    refreshCatalogRounds();
}

void AdminViewModel::setImageProcessor(ImageProcessor *processor)
{
    m_imageProcessor = processor;
}

void AdminViewModel::setImageProvider(PuzzleImageProvider *provider)
{
    m_imageProvider = provider;
}

QString AdminViewModel::selectedPresetName() const
{
    for (const QVariant &item : m_presets) {
        const QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("id")).toInt() == m_selectedPresetId) {
            return map.value(QStringLiteral("name")).toString();
        }
    }
    return {};
}

QString AdminViewModel::previewImageUrl() const
{
    if (m_previewImage.isNull()) {
        return {};
    }
    return QStringLiteral("image://puzzle/preview?rev=%1").arg(m_previewRevision);
}

QString AdminViewModel::puzzleImageUrl() const
{
    if (m_selectedPuzzleId <= 0) {
        return {};
    }
    return QStringLiteral("image://puzzle/%1/%2?rev=%3")
        .arg(m_selectedPuzzleId)
        .arg(m_selectedImageSlot)
        .arg(m_puzzleImageRevision);
}

int AdminViewModel::imageSlotCount() const
{
    return imageSlotCountForLayout(m_selectedRoundLayoutType);
}

int AdminViewModel::configImageSlotCount() const
{
    return imageSlotCountForLayout(m_selectedRoundLayoutType);
}

int AdminViewModel::configTextSlotCount() const
{
    return textSlotCountForLayout(m_selectedRoundLayoutType);
}

int AdminViewModel::imageSlotCountForLayout(const QString &layoutType)
{
    if (layoutType == QStringLiteral("FULL_MASK")) {
        return 1;
    }
    if (layoutType == QStringLiteral("TEXT_ONLY")) {
        return 0;
    }
    if (layoutType == QStringLiteral("SINGLE_HYBRID")) {
        return 3;
    }
    if (layoutType == QStringLiteral("EQUATION")) {
        return 3;
    }
    if (layoutType == QStringLiteral("CHRONOLOGY")) {
        return 4;
    }
    if (layoutType == QStringLiteral("BLITZ_STANDARD")) {
        return 8;
    }
    return 4;
}

int AdminViewModel::textSlotCountForLayout(const QString &layoutType)
{
    if (layoutType == QStringLiteral("TEXT_ONLY")) {
        return 4;
    }
    return 0;
}

bool AdminViewModel::showGamePreview() const
{
    return isPhotoMaskRound() && m_selectedImageSlot == 0 && hasMaskContour() && hasPreviewImage();
}

void AdminViewModel::setSelectedImageSlot(int slot)
{
    if (slot < 0 || slot >= imageSlotCount()) {
        return;
    }
    if (m_selectedImageSlot == slot) {
        return;
    }

    cacheCurrentSlotState();
    m_selectedImageSlot = slot;
    emit selectedImageSlotChanged();
    loadImageSlot(slot);
}

void AdminViewModel::cacheCurrentSlotState()
{
    if (m_selectedPuzzleId <= 0) {
        return;
    }

    if (!m_sourceImage.isNull()) {
        m_slotImageCache.insert(m_selectedImageSlot, m_sourceImage);
    }
    if (!m_pendingImageBytes.isEmpty()) {
        m_slotPendingBytes.insert(m_selectedImageSlot, m_pendingImageBytes);
    }
}

void AdminViewModel::loadImageSlot(int slotIndex)
{
    m_sourceImage = QImage();
    m_previewImage = QImage();
    m_pendingImageBytes.clear();
    m_editorMasks.clear();

    if (m_slotImageCache.contains(slotIndex)) {
        m_sourceImage = m_slotImageCache.value(slotIndex);
    } else if (m_database && m_selectedPuzzleId > 0) {
        const QByteArray imageData = m_database->puzzleImageData(m_selectedPuzzleId, slotIndex);
        if (!imageData.isEmpty()) {
            m_sourceImage.loadFromData(imageData);
        }
    }

    if (m_slotPendingBytes.contains(slotIndex)) {
        m_pendingImageBytes = m_slotPendingBytes.value(slotIndex);
        if (m_sourceImage.isNull()) {
            m_sourceImage.loadFromData(m_pendingImageBytes);
        }
    }

    if (slotIndex == 0) {
        if (m_selectedPuzzleId > 0) {
            loadEditorMasks(m_selectedPuzzleId);
        } else if (m_selectedTemplateId > 0 && m_database) {
            m_editorMasks.clear();
            const QString contour = m_database->maskTemplateContour(m_selectedTemplateId);
            if (!contour.isEmpty()) {
                EditorMaskEntry entry;
                entry.number = 1;
                entry.contour = contour;
                entry.answer = m_editAnswer;
                m_editorMasks.append(entry);
            }
            emit masksChanged();
        }
    }

    if (m_sourceImage.isNull() && slotIndex == 0 && m_selectedTemplateId > 0 && m_database) {
        const QByteArray templateData = m_database->maskTemplateImageData(m_selectedTemplateId);
        if (!templateData.isEmpty()) {
            m_sourceImage.loadFromData(templateData);
        }
    }

    refreshEditorPreview();
    ++m_puzzleImageRevision;
    emit puzzleImageChanged();
    notifySlotImagesChanged();
}

void AdminViewModel::refreshEditorPreview()
{
    if (m_sourceImage.isNull()) {
        m_previewImage = QImage();
        updatePreviewProvider();
        return;
    }

    const bool hide = isPhotoMaskRound() && m_selectedImageSlot == 0 && !m_editorMasks.isEmpty()
                      && m_imageProcessor;
    if (!hide) {
        m_previewImage = m_sourceImage;
        updatePreviewProvider();
        return;
    }

    const int jobId = ++m_previewJobId;
    const QImage source = m_sourceImage;
    const QVector<EditorMaskEntry> masks = m_editorMasks;
    ImageProcessor *processor = m_imageProcessor;
    setImageProcessingBusy(true);

    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, jobId]() {
        if (jobId == m_previewJobId) {
            const QImage result = watcher->result();
            m_previewImage = result.isNull() ? m_sourceImage : result;
            updatePreviewProvider();
        }
        setImageProcessingBusy(false);
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([processor, source, masks]() -> QImage {
        if (!processor) {
            return source;
        }
        QVector<MaskLayer> layers;
        layers.reserve(masks.size());
        for (const AdminViewModel::EditorMaskEntry &entry : masks) {
            MaskLayer layer;
            layer.number = entry.number;
            layer.contour = entry.contour;
            layers.append(layer);
        }
        const QImage hidden = processor->applyHideMasks(source, layers);
        return hidden.isNull() ? source : hidden;
    }));
}

void AdminViewModel::setImageProcessingBusy(bool busy)
{
    if (busy) {
        ++m_processingJobs;
    } else if (m_processingJobs > 0) {
        --m_processingJobs;
    }

    const bool processing = m_processingJobs > 0;
    if (m_imageProcessing == processing) {
        return;
    }
    m_imageProcessing = processing;
    emit imageProcessingChanged();
}

bool AdminViewModel::slotHasImage(int slotIndex) const
{
    if (m_slotImageCache.contains(slotIndex) && !m_slotImageCache.value(slotIndex).isNull()) {
        return true;
    }
    if (m_slotPendingBytes.contains(slotIndex)) {
        return true;
    }
    if (slotIndex == m_selectedImageSlot && !m_sourceImage.isNull()) {
        return true;
    }
    if (m_database && m_selectedPuzzleId > 0) {
        return !m_database->puzzleImageData(m_selectedPuzzleId, slotIndex).isEmpty();
    }
    return false;
}

QString AdminViewModel::slotThumbnailUrl(int slotIndex) const
{
    if (!slotHasImage(slotIndex)) {
        return {};
    }
    if (m_selectedPuzzleId > 0 && slotIndex != m_selectedImageSlot) {
        return QStringLiteral("image://puzzle/%1/%2?rev=%3")
            .arg(m_selectedPuzzleId)
            .arg(slotIndex)
            .arg(m_puzzleImageRevision);
    }
    if (!m_previewImage.isNull() && slotIndex == m_selectedImageSlot) {
        return previewImageUrl();
    }
    return QStringLiteral("image://puzzle/%1/%2?rev=%3")
        .arg(m_selectedPuzzleId)
        .arg(slotIndex)
        .arg(m_puzzleImageRevision);
}

void AdminViewModel::setSelectedPresetId(int presetId)
{
    if (m_selectedPresetId == presetId) {
        return;
    }
    m_selectedPresetId = presetId;
    m_editPresetName = selectedPresetName();
    emit editPresetNameChanged();
    emit selectedPresetIdChanged(presetId);
    refreshPresetRounds();
    clearPuzzleEditor();
}

void AdminViewModel::setSelectedRoundId(int roundId)
{
    if (m_selectedRoundId == roundId) {
        return;
    }
    m_selectedRoundId = roundId;
    m_selectedRoundLayoutType.clear();
    if (m_database && roundId > 0) {
        const RoundInfo round = m_database->roundById(roundId);
        m_selectedRoundLayoutType = round.layoutType;
    }
    emit selectedRoundIdChanged(roundId);
    emit imageSlotsChanged();
    clearPuzzleEditor();
    refreshPuzzles();
}

void AdminViewModel::setEditPresetName(const QString &name)
{
    if (m_editPresetName == name) {
        return;
    }
    m_editPresetName = name;
    emit editPresetNameChanged();
}

void AdminViewModel::setEditAnswer(const QString &answer)
{
    if (m_editAnswer == answer) {
        return;
    }
    m_editAnswer = answer;
    emit editAnswerChanged();
}

void AdminViewModel::setEditHint(const QString &hint)
{
    if (m_editHint == hint) {
        return;
    }
    m_editHint = hint;
    emit editHintChanged();
}

void AdminViewModel::setEditQuotes(const QString &quotes)
{
    if (m_editQuotes == quotes) {
        return;
    }
    m_editQuotes = quotes;
    emit editQuotesChanged();
}

void AdminViewModel::setHybridAnimationStyle(const QString &style)
{
    const QString normalized = normalizeHybridAnimStyleValue(style);
    if (m_hybridAnimationStyle == normalized) {
        return;
    }
    m_hybridAnimationStyle = normalized;
    emit hybridAnimationStyleChanged();
}

void AdminViewModel::refreshPresets()
{
    rebuildPresetList();
    emit presetsChanged();

    if (m_selectedPresetId <= 0 && !m_presets.isEmpty()) {
        setSelectedPresetId(m_presets.first().toMap().value(QStringLiteral("id")).toInt());
    } else if (m_selectedPresetId > 0) {
        m_editPresetName = selectedPresetName();
        emit editPresetNameChanged();
        refreshPresetRounds();
    }
}

void AdminViewModel::refreshCatalogRounds()
{
    rebuildCatalogRounds();
    emit catalogRoundsChanged();
}

void AdminViewModel::refreshPresetRounds()
{
    rebuildPresetRoundIds();
    emit presetRoundIdsChanged();
}

void AdminViewModel::refreshPuzzles()
{
    rebuildPuzzleList();
    emit puzzlesChanged();
}

void AdminViewModel::createPreset()
{
    if (!m_database) {
        return;
    }

    QString name = QStringLiteral("Новая игра");
    int suffix = 2;
    for (;;) {
        bool exists = false;
        for (const QVariant &item : m_presets) {
            if (item.toMap().value(QStringLiteral("name")).toString() == name) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            break;
        }
        name = QStringLiteral("Новая игра %1").arg(suffix++);
    }

    const int id = m_database->createPreset(name);
    if (id <= 0) {
        setStatusMessage(QStringLiteral("Не удалось создать пресет"));
        return;
    }

    refreshPresets();
    setSelectedPresetId(id);
    setEditPresetName(name);
    setStatusMessage(QStringLiteral("Пресет создан"));
}

void AdminViewModel::savePresetMeta()
{
    if (!m_database || m_selectedPresetId <= 0) {
        return;
    }

    if (!m_database->renamePreset(m_selectedPresetId, m_editPresetName)) {
        setStatusMessage(QStringLiteral("Не удалось сохранить название"));
        return;
    }

    refreshPresets();
    setStatusMessage(QStringLiteral("Название сохранено"));
}

void AdminViewModel::deleteSelectedPreset()
{
    deletePreset(m_selectedPresetId);
}

void AdminViewModel::deletePreset(int presetId)
{
    if (!m_database || presetId <= 0) {
        return;
    }

    if (!m_database->deletePreset(presetId)) {
        setStatusMessage(QStringLiteral("Не удалось удалить пресет"));
        return;
    }

    if (m_selectedPresetId == presetId) {
        m_selectedPresetId = 0;
        m_editPresetName.clear();
        emit editPresetNameChanged();
        emit selectedPresetIdChanged(0);
        refreshPresetRounds();
        clearPuzzleEditor();
    }

    refreshPresets();
    setStatusMessage(QStringLiteral("Пресет удалён"));
}

int AdminViewModel::roundTemplateStatus(int roundId) const
{
    return evaluateRoundTemplateStatus(roundId);
}

int AdminViewModel::evaluateRoundTemplateStatus(int roundId) const
{
    if (!m_database || roundId <= 0) {
        return static_cast<int>(DataEmpty);
    }

    const RoundInfo round = m_database->roundById(roundId);
    if (round.id <= 0) {
        return static_cast<int>(DataEmpty);
    }

    const QVector<PuzzleInfo> puzzles = m_database->listPuzzlesForRound(roundId);
    if (puzzles.isEmpty()) {
        return static_cast<int>(DataEmpty);
    }

    const PuzzleInfo &puzzle = puzzles.first();
    const QString layout = round.layoutType;
    const int imageSlots = imageSlotCountForLayout(layout);
    const int textSlots = textSlotCountForLayout(layout);

    const bool hasAnswer = !puzzle.correctAnswer.trimmed().isEmpty();

    int filledQuotes = 0;
    if (!puzzle.quoteSlotsJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(puzzle.quoteSlotsJson.toUtf8());
        if (doc.isArray()) {
            for (const QJsonValue &value : doc.array()) {
                if (!value.toString().trimmed().isEmpty()) {
                    ++filledQuotes;
                }
            }
        }
    }

    int filledOptions = 0;
    if (!puzzle.correctOrder.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(puzzle.correctOrder.toUtf8());
        if (doc.isArray()) {
            for (const QJsonValue &value : doc.array()) {
                if (!value.toString().trimmed().isEmpty()) {
                    ++filledOptions;
                }
            }
        }
    }

    int filledImages = 0;
    for (int slot = 0; slot < imageSlots; ++slot) {
        if (!m_database->puzzleImageData(puzzle.id, slot).isEmpty()) {
            ++filledImages;
        }
    }

    bool hasMask = false;
    if (layout == QStringLiteral("FULL_MASK")) {
        const QVector<PuzzleMaskInfo> masks = m_database->listPuzzleMasks(puzzle.id);
        if (!masks.isEmpty()) {
            hasMask = true;
            for (const PuzzleMaskInfo &mask : masks) {
                if (mask.contourPoints.trimmed().isEmpty() || mask.answerText.trimmed().isEmpty()) {
                    hasMask = false;
                    break;
                }
            }
        } else if (puzzle.templateId > 0) {
            hasMask = !m_database->maskTemplateContour(puzzle.templateId).isEmpty();
        }
    }

    const bool hasHint = !m_database->localizedString(puzzle.hintTextKey).trimmed().isEmpty();
    const bool hasAny = hasAnswer || hasHint || filledQuotes > 0 || filledOptions > 0
                        || filledImages > 0 || hasMask;
    if (!hasAny) {
        return static_cast<int>(DataEmpty);
    }

    bool complete = false;
    if (layout == QStringLiteral("FULL_MASK")) {
        complete = hasAnswer && filledImages >= 1 && hasMask;
    } else if (layout == QStringLiteral("TEXT_ONLY")) {
        complete = hasAnswer && filledQuotes >= textSlots;
    } else if (layout == QStringLiteral("SINGLE_HYBRID")) {
        complete = hasAnswer && filledImages >= 1 && filledQuotes >= textSlots;
    } else if (imageSlots > 0) {
        complete = hasAnswer && filledImages >= imageSlots;
    } else {
        complete = hasAnswer;
    }

    return complete ? static_cast<int>(DataComplete) : static_cast<int>(DataPartial);
}

void AdminViewModel::togglePresetRound(int roundId, bool enabled)
{
    QVector<int> roundIds;
    for (const QVariant &item : m_presetRoundIdsList) {
        roundIds.append(item.toInt());
    }

    if (enabled) {
        if (!roundIds.contains(roundId)) {
            roundIds.append(roundId);
        }
    } else {
        roundIds.removeAll(roundId);
    }

    m_presetRoundIdsList.clear();
    for (int id : roundIds) {
        m_presetRoundIdsList.append(id);
    }
    emit presetRoundIdsChanged();
}

void AdminViewModel::savePresetRounds()
{
    if (!m_database || m_selectedPresetId <= 0) {
        return;
    }

    QVector<int> roundIds;
    roundIds.reserve(m_presetRoundIdsList.size());
    for (const QVariant &item : m_presetRoundIdsList) {
        roundIds.append(item.toInt());
    }

    if (!m_database->setPresetRounds(m_selectedPresetId, roundIds)) {
        setStatusMessage(QStringLiteral("Не удалось сохранить раунды"));
        return;
    }

    refreshPresets();
    refreshPresetRounds();
    setStatusMessage(QStringLiteral("Раунды сохранены"));
}

bool AdminViewModel::isRoundEnabled(int roundId) const
{
    for (const QVariant &item : m_presetRoundIdsList) {
        if (item.toInt() == roundId) {
            return true;
        }
    }
    return false;
}

void AdminViewModel::setRoundEnabled(int roundId, bool enabled)
{
    if (isRoundEnabled(roundId) == enabled) {
        return;
    }
    togglePresetRound(roundId, enabled);
    savePresetRounds();
}

void AdminViewModel::openRoundConfig(int roundId)
{
    if (!m_database || m_selectedPresetId <= 0) {
        setStatusMessage(QStringLiteral("Сначала выберите или создайте пресет"));
        return;
    }

    if (!isRoundEnabled(roundId)) {
        setRoundEnabled(roundId, true);
    }

    if (m_selectedRoundId != roundId) {
        setSelectedRoundId(roundId);
    } else if (m_database && roundId > 0) {
        const RoundInfo round = m_database->roundById(roundId);
        m_selectedRoundLayoutType = round.layoutType;
        emit selectedRoundIdChanged(roundId);
        emit imageSlotsChanged();
        refreshPuzzles();
    }

    const RoundInfo round = m_database->roundById(roundId);
    m_configRoundTitle = m_database->localizedString(round.titleKey);
    m_configRoundRule = m_database->localizedString(round.ruleTextKey);

    if (!ensureRoundPuzzleTemplates(roundId, GameConstants::MAX_PUZZLES_PER_ROUND)) {
        setStatusMessage(QStringLiteral("Не удалось создать шаблоны вопросов для раунда"));
        return;
    }
    refreshPuzzles();
    if (!m_puzzles.isEmpty()) {
        const QVariantMap first = m_puzzles.first().toMap();
        selectPuzzle(first.value(QStringLiteral("id")).toInt());
    }

    if (m_roundConfigOpen) {
        emit roundConfigOpenChanged();
        return;
    }

    m_roundConfigOpen = true;
    emit roundConfigOpenChanged();
}

void AdminViewModel::closeRoundConfig()
{
    if (!m_roundConfigOpen) {
        return;
    }

    ++m_editorGuardDepth;
    if (m_selectedPuzzleId > 0) {
        queuePuzzleSave(capturePuzzleSaveSnapshot(m_selectedPuzzleId), false);
    } else {
        cacheCurrentSlotState();
    }
    --m_editorGuardDepth;

    m_roundConfigOpen = false;
    emit roundConfigOpenChanged();
}

void AdminViewModel::saveRoundConfig()
{
    savePuzzle();
}

void AdminViewModel::autosaveEditorDraft()
{
    if (!m_database || m_editorGuardDepth > 0) {
        return;
    }

    if (m_roundConfigOpen) {
        if (m_selectedPuzzleId > 0) {
            queuePuzzleSave(capturePuzzleSaveSnapshot(m_selectedPuzzleId), false);
        }
        return;
    }

    if (m_selectedPresetId <= 0 || m_editPresetName.trimmed().isEmpty()) {
        return;
    }

    const QString savedName = selectedPresetName();
    if (m_editPresetName == savedName) {
        return;
    }

    if (!m_database->renamePreset(m_selectedPresetId, m_editPresetName)) {
        return;
    }
    refreshPresets();
}

void AdminViewModel::addAnswerOption()
{
    if (m_answerOptions.size() >= maxAnswerOptions()) {
        setStatusMessage(QStringLiteral("Не более %1 вариантов ответа").arg(maxAnswerOptions()));
        return;
    }
    m_answerOptions.append(QString());
    emit answerOptionsChanged();
}

void AdminViewModel::removeAnswerOptionAt(int index)
{
    if (index < 0 || index >= m_answerOptions.size()) {
        return;
    }
    m_answerOptions.removeAt(index);
    emit answerOptionsChanged();
}

QString AdminViewModel::cardTextAt(int index) const
{
    if (index < 0 || index >= m_configCardTexts.size()) {
        return {};
    }
    return m_configCardTexts.at(index);
}

void AdminViewModel::setCardTextAt(int index, const QString &text)
{
    if (index < 0 || index >= m_configCardTexts.size()) {
        return;
    }
    if (m_configCardTexts.at(index) == text) {
        return;
    }
    m_configCardTexts[index] = text;
    emit cardTextsChanged();
}

QString AdminViewModel::answerOptionAt(int index) const
{
    if (index < 0 || index >= m_answerOptions.size()) {
        return {};
    }
    return m_answerOptions.at(index);
}

void AdminViewModel::setAnswerOptionAt(int index, const QString &text)
{
    if (index < 0 || index >= m_answerOptions.size()) {
        return;
    }
    if (m_answerOptions.at(index) == text) {
        return;
    }
    m_answerOptions[index] = text;
    emit answerOptionsChanged();
}

QString AdminViewModel::cardTextPlaceholder(int index) const
{
    if (m_selectedRoundLayoutType == QStringLiteral("TEXT_ONLY")) {
        if (m_selectedRoundId == 2) {
            return label(QStringLiteral("ui.editor.text_slot_association_format")).arg(index + 1);
        }
        return label(QStringLiteral("ui.editor.text_slot_quote_format")).arg(index + 1);
    }
    return label(QStringLiteral("ui.editor.text_slot_default_format")).arg(index + 1);
}

QString AdminViewModel::hostHintAt(int index) const
{
    if (index < 0) {
        return {};
    }
    const QStringList parts = m_editHint.split(QStringLiteral("||"));
    return index < parts.size() ? parts.at(index).trimmed() : QString();
}

void AdminViewModel::setHostHintAt(int index, const QString &text)
{
    if (index < 0) {
        return;
    }

    QStringList parts = m_editHint.split(QStringLiteral("||"));
    while (parts.size() <= index) {
        parts.append(QString());
    }
    parts[index] = text;

    while (parts.size() > 1 && parts.last().trimmed().isEmpty()) {
        parts.removeLast();
    }

    QString packed;
    for (int i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            packed += QStringLiteral("||");
        }
        packed += parts.at(i);
    }
    setEditHint(packed);
}

void AdminViewModel::removeHostHintAt(int index)
{
    if (index < 0) {
        return;
    }

    QStringList parts = m_editHint.split(QStringLiteral("||"), Qt::KeepEmptyParts);
    if (parts.size() == 1 && parts.first().trimmed().isEmpty()) {
        parts.clear();
    }
    if (index >= parts.size()) {
        return;
    }
    parts.removeAt(index);

    QString packed;
    for (int i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            packed += QStringLiteral("||");
        }
        packed += parts.at(i);
    }
    setEditHint(packed);
}

QString AdminViewModel::imageSlotLabel(int index) const
{
    if (m_selectedRoundLayoutType == QStringLiteral("SINGLE_HYBRID")) {
        switch (index) {
        case 0:
            return label(QStringLiteral("ui.editor.image_slot_merged"));
        case 1:
            return label(QStringLiteral("ui.editor.image_slot_left"));
        case 2:
            return label(QStringLiteral("ui.editor.image_slot_right"));
        default:
            break;
        }
    }
    if (m_selectedRoundLayoutType == QStringLiteral("EQUATION")) {
        if (index < 2) {
            return label(QStringLiteral("ui.editor.image_slot_operand_format")).arg(index + 1);
        }
        return label(QStringLiteral("ui.editor.image_slot_answer"));
    }
    return label(QStringLiteral("ui.editor.image_slot_default_format")).arg(index + 1);
}

QString AdminViewModel::answerOptionPlaceholder() const
{
    return QStringLiteral("Введите вариант ответа");
}

void AdminViewModel::ensureConfigCardTextSize()
{
    const int needed = textSlotCountForLayout(m_selectedRoundLayoutType);
    if (m_configCardTexts.size() == needed) {
        return;
    }
    while (m_configCardTexts.size() < needed) {
        m_configCardTexts.append(QString());
    }
    while (m_configCardTexts.size() > needed) {
        m_configCardTexts.removeLast();
    }
    emit cardTextsChanged();
}

void AdminViewModel::selectPuzzle(int puzzleId)
{
    if (puzzleId <= 0 || puzzleId == m_selectedPuzzleId) {
        return;
    }

    ++m_editorGuardDepth;
    if (m_selectedPuzzleId > 0) {
        queuePuzzleSave(capturePuzzleSaveSnapshot(m_selectedPuzzleId), false);
    } else {
        cacheCurrentSlotState();
    }

    m_selectedPuzzleId = puzzleId;
    m_selectedImageSlot = 0;
    emit selectedPuzzleIdChanged();
    emit selectedImageSlotChanged();
    loadPuzzleEditor(puzzleId);
    --m_editorGuardDepth;
}

void AdminViewModel::focusPhotoMaskRound()
{
    for (const QVariant &item : m_catalogRounds) {
        const QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("layoutType")).toString() != QStringLiteral("FULL_MASK")) {
            continue;
        }
        const int roundId = map.value(QStringLiteral("id")).toInt();
        if (m_selectedPresetId <= 0) {
            setStatusMessage(QStringLiteral("Сначала создайте или выберите пресет"));
            return;
        }
        if (!m_presetRoundIdsList.contains(roundId)) {
            m_presetRoundIdsList.append(roundId);
            emit presetRoundIdsChanged();
            savePresetRounds();
        }
        setSelectedRoundId(roundId);
        setStatusMessage(QStringLiteral("Раунд «Чего-то не хватает» — добавьте фото-загадку"));
        return;
    }
    setStatusMessage(QStringLiteral("Раунд FULL_MASK не найден в каталоге"));
}

void AdminViewModel::startPhotoPuzzle()
{
    for (const QVariant &item : m_catalogRounds) {
        const QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("layoutType")).toString() != QStringLiteral("FULL_MASK")) {
            continue;
        }
        const int roundId = map.value(QStringLiteral("id")).toInt();
        if (m_selectedPresetId <= 0) {
            setStatusMessage(QStringLiteral("Сначала создайте или выберите пресет"));
            return;
        }
        if (!isRoundEnabled(roundId)) {
            setRoundEnabled(roundId, true);
        }
        openRoundConfig(roundId);
        return;
    }
    setStatusMessage(QStringLiteral("Раунд FULL_MASK не найден в каталоге"));
}

void AdminViewModel::createPuzzle()
{
    if (m_puzzles.size() >= GameConstants::MAX_PUZZLES_PER_ROUND) {
        setStatusMessage(QStringLiteral("Максимум %1 вопросов на раунд")
                             .arg(GameConstants::MAX_PUZZLES_PER_ROUND));
        return;
    }

    if (!m_database || m_selectedRoundId <= 0) {
        setStatusMessage(QStringLiteral("Сначала выберите раунд"));
        return;
    }

    const int puzzleId = m_database->createPuzzle(m_selectedRoundId, QString(), QString());
    if (puzzleId <= 0) {
        setStatusMessage(QStringLiteral("Не удалось создать загадку"));
        return;
    }

    refreshPuzzles();
    selectPuzzle(puzzleId);
    setStatusMessage(QStringLiteral("Загадка создана"));
}

bool AdminViewModel::ensureRoundPuzzleTemplates(int roundId, int targetCount)
{
    if (!m_database || roundId <= 0) {
        return false;
    }

    const int limit = std::max(1, targetCount);
    if (!m_database->ensurePuzzleTemplates(roundId, limit)) {
        return false;
    }
    return m_database->normalizePuzzleSortOrder(roundId);
}

QHash<int, QByteArray> AdminViewModel::encodedSlotImages(const PuzzleSaveSnapshot &snapshot)
{
    QHash<int, QByteArray> encoded;
    const auto encodeImage = [](const QImage &image) -> QByteArray {
        if (image.isNull()) {
            return {};
        }
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        return bytes;
    };

    QHash<int, QImage> images = snapshot.slotImageCache;
    for (auto it = snapshot.slotPendingBytes.constBegin(); it != snapshot.slotPendingBytes.constEnd(); ++it) {
        if (!it.value().isEmpty()) {
            encoded.insert(it.key(), it.value());
            images.remove(it.key());
        }
    }
    for (auto it = images.constBegin(); it != images.constEnd(); ++it) {
        const QByteArray bytes = encodeImage(it.value());
        if (!bytes.isEmpty()) {
            encoded.insert(it.key(), bytes);
        }
    }
    return encoded;
}

bool AdminViewModel::persistPuzzleSnapshot(DatabaseManager *database,
                                           const PuzzleSaveSnapshot &snapshot,
                                           QString *errorMessage)
{
    if (!database || snapshot.puzzleId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Database unavailable");
        }
        return false;
    }

    if (!database->updatePuzzle(snapshot.puzzleId,
                                snapshot.answer,
                                snapshot.hint,
                                snapshot.quoteSlotsJson,
                                snapshot.answerOptionsJson)) {
        return false;
    }

    const QHash<int, QByteArray> slotImages = encodedSlotImages(snapshot);
    for (auto it = slotImages.constBegin(); it != slotImages.constEnd(); ++it) {
        if (!database->upsertPuzzleImage(snapshot.puzzleId, it.key(), it.value())) {
            return false;
        }
    }

    if (snapshot.replaceMasks) {
        QVector<PuzzleMaskInfo> masks;
        masks.reserve(snapshot.editorMasks.size());
        for (const EditorMaskEntry &entry : snapshot.editorMasks) {
            if (entry.contour.trimmed().isEmpty()) {
                continue;
            }
            PuzzleMaskInfo mask;
            mask.sortOrder = entry.number;
            mask.answerText = entry.answer.trimmed();
            mask.contourPoints = entry.contour;
            masks.append(mask);
        }
        if (!masks.isEmpty() && !database->replacePuzzleMasks(snapshot.puzzleId, masks)) {
            return false;
        }
    }

    return true;
}

void AdminViewModel::savePuzzle()
{
    if (!validatePuzzleEditor()) {
        return;
    }

    setStatusMessage(QStringLiteral("Сохранение..."));
    queuePuzzleSave(capturePuzzleSaveSnapshot(m_selectedPuzzleId), true);
}

bool AdminViewModel::validatePuzzleEditor()
{
    if (!m_database || m_selectedPuzzleId <= 0) {
        return false;
    }

    if (isPhotoMaskRound() && m_editorMasks.isEmpty()) {
        setStatusMessage(QStringLiteral("Добавьте хотя бы одну маску на фото"));
        return false;
    }

    if (isPhotoMaskRound()) {
        for (const EditorMaskEntry &mask : m_editorMasks) {
            if (mask.answer.trimmed().isEmpty()) {
                setStatusMessage(QStringLiteral("Укажите ответ для каждой маски"));
                return false;
            }
        }
    }

    if (isPhotoMaskRound() && !slotHasImage(0)) {
        setStatusMessage(QStringLiteral("Загрузите фото в первую ячейку"));
        return false;
    }

    return true;
}

AdminViewModel::PuzzleSaveSnapshot AdminViewModel::capturePuzzleSaveSnapshot(int puzzleId)
{
    cacheCurrentSlotState();

    PuzzleSaveSnapshot snapshot;
    snapshot.puzzleId = puzzleId;
    snapshot.layoutType = m_selectedRoundLayoutType;
    snapshot.answer = m_editAnswer;
    snapshot.hint = m_editHint;
    snapshot.hybridAnimationStyle = m_hybridAnimationStyle;
    snapshot.editorMasks = m_editorMasks;
    snapshot.slotImageCache = m_slotImageCache;
    snapshot.slotPendingBytes = m_slotPendingBytes;
    snapshot.selectedImageSlot = m_selectedImageSlot;
    snapshot.replaceMasks = isPhotoMaskRound() && !m_editorMasks.isEmpty();

    if (!m_sourceImage.isNull()) {
        snapshot.slotImageCache.insert(m_selectedImageSlot, m_sourceImage);
    }
    if (!m_pendingImageBytes.isEmpty()) {
        snapshot.slotPendingBytes.insert(m_selectedImageSlot, m_pendingImageBytes);
    }

    const bool photoMask = isPhotoMaskRound();
    QString answer = snapshot.answer;
    if (!photoMask && answer.trimmed().isEmpty()) {
        for (const QString &option : m_answerOptions) {
            if (!option.trimmed().isEmpty()) {
                answer = option.trimmed();
                break;
            }
        }
    }
    if (photoMask && answer.trimmed().isEmpty()) {
        for (const EditorMaskEntry &mask : snapshot.editorMasks) {
            if (!mask.answer.trimmed().isEmpty()) {
                answer = mask.answer.trimmed();
                break;
            }
        }
    }
    if (photoMask && !snapshot.editorMasks.isEmpty()) {
        QStringList uniqueAnswers;
        for (const EditorMaskEntry &mask : snapshot.editorMasks) {
            const QString key = normalizeAnswerKey(mask.answer);
            bool exists = false;
            for (const QString &existing : uniqueAnswers) {
                if (normalizeAnswerKey(existing) == key) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                uniqueAnswers.append(mask.answer.trimmed());
            }
        }
        answer = uniqueAnswers.join(QStringLiteral(", "));
    }
    snapshot.answer = answer;
    snapshot.quoteSlotsJson = quoteSlotsJsonFromEditor();
    snapshot.answerOptionsJson = answerOptionsJsonFromEditor();
    return snapshot;
}

void AdminViewModel::queuePuzzleSave(PuzzleSaveSnapshot snapshot, bool userInitiated)
{
    if (!m_database || snapshot.puzzleId <= 0) {
        return;
    }

    setPuzzleSavingBusy(true);
    DatabaseManager *database = m_database;

    auto *watcher = new QFutureWatcher<PuzzleSaveOutcome>(this);
    connect(watcher, &QFutureWatcher<PuzzleSaveOutcome>::finished, this, [this, watcher, userInitiated]() {
        const PuzzleSaveOutcome outcome = watcher->result();
        watcher->deleteLater();
        setPuzzleSavingBusy(false);
        onPuzzleSaveFinished(outcome.puzzleId,
                             outcome.success,
                             userInitiated,
                             outcome.errorMessage);
    });
    watcher->setFuture(QtConcurrent::run([database, snapshot]() -> PuzzleSaveOutcome {
        PuzzleSaveOutcome outcome;
        outcome.puzzleId = snapshot.puzzleId;
        outcome.success = AdminViewModel::persistPuzzleSnapshot(database, snapshot, &outcome.errorMessage);
        return outcome;
    }));
}

void AdminViewModel::onPuzzleSaveFinished(int puzzleId,
                                          bool success,
                                          bool userInitiated,
                                          const QString &errorMessage)
{
    if (!success) {
        if (userInitiated) {
            setStatusMessage(errorMessage.isEmpty()
                                 ? QStringLiteral("Не удалось сохранить загадку")
                                 : errorMessage);
        }
        return;
    }

    if (m_imageProvider) {
        m_imageProvider->invalidatePuzzle(puzzleId);
    }

    if (puzzleId == m_selectedPuzzleId) {
        ++m_puzzleImageRevision;
        emit puzzleImageChanged();
        notifySlotImagesChanged();
    }

    if (!userInitiated || puzzleId != m_selectedPuzzleId) {
        return;
    }

    refreshPuzzles();
    loadPuzzleEditor(m_selectedPuzzleId);
    rebuildCatalogRounds();
    emit catalogRoundsChanged();
    setStatusMessage(QStringLiteral("Загадка сохранена"));
}

void AdminViewModel::setPuzzleSavingBusy(bool starting)
{
    const bool wasSaving = m_pendingPuzzleSaves > 0;
    if (starting) {
        ++m_pendingPuzzleSaves;
    } else {
        m_pendingPuzzleSaves = qMax(0, m_pendingPuzzleSaves - 1);
    }
    if (wasSaving != (m_pendingPuzzleSaves > 0)) {
        emit puzzleSavingChanged();
    }
}

void AdminViewModel::deleteSelectedPuzzle()
{
    if (!m_database || m_selectedPuzzleId <= 0) {
        return;
    }

    if (!m_database->deletePuzzle(m_selectedPuzzleId)) {
        setStatusMessage(QStringLiteral("Не удалось удалить загадку"));
        return;
    }

    clearPuzzleEditor();
    refreshPuzzles();
    setStatusMessage(QStringLiteral("Загадка удалена"));
}

bool AdminViewModel::importPuzzleImage(const QUrl &fileUrl)
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatusMessage(QStringLiteral("Не удалось открыть файл"));
        return false;
    }

    m_pendingImageBytes = file.readAll();
    if (!m_sourceImage.loadFromData(m_pendingImageBytes)) {
        m_pendingImageBytes.clear();
        setStatusMessage(QStringLiteral("Формат изображения не поддерживается"));
        return false;
    }
    m_slotPendingBytes.insert(m_selectedImageSlot, m_pendingImageBytes);
    m_slotImageCache.insert(m_selectedImageSlot, m_sourceImage);

    if (m_selectedImageSlot == 0) {
        m_editorMasks.clear();
        emit masksChanged();
    }

    refreshEditorPreview();

    if (m_selectedPuzzleId > 0 && m_database) {
        m_database->upsertPuzzleImage(m_selectedPuzzleId, m_selectedImageSlot, m_pendingImageBytes);
        m_slotPendingBytes.remove(m_selectedImageSlot);
        m_pendingImageBytes.clear();
        ++m_puzzleImageRevision;
        emit puzzleImageChanged();
    }

    notifySlotImagesChanged();
    setStatusMessage(QStringLiteral("Фото загружено в ячейку %1 — нажмите на объект")
                         .arg(m_selectedImageSlot + 1));
    return true;
}

bool AdminViewModel::markMissingArea(double relX, double relY)
{
    return markMissingRegion(relX, relY, 0.0, 0.0);
}

bool AdminViewModel::markMissingRegion(double relX, double relY, double relW, double relH)
{
    if (m_selectedImageSlot != 0) {
        setStatusMessage(QStringLiteral("Маска доступна только для первой ячейки"));
        return false;
    }

    if (m_sourceImage.isNull()) {
        setStatusMessage(QStringLiteral("Сначала загрузите фото"));
        return false;
    }

    if (!m_imageProcessor) {
        setStatusMessage(QStringLiteral("Обработка изображений недоступна (соберите с OpenCV)"));
        return false;
    }

    const int jobId = ++m_regionJobId;
    const QImage source = m_sourceImage;
    ImageProcessor *processor = m_imageProcessor;
    if (!m_maskProcessing) {
        m_maskProcessing = true;
        emit maskProcessingChanged();
    }
    setStatusMessage(QStringLiteral("Обработка изображения…"));

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, jobId]() {
        if (jobId == m_regionJobId) {
            const QString contour = watcher->result();
            if (contour.isEmpty()) {
                setStatusMessage(QStringLiteral("Не удалось выделить объект — обведите его рамкой"));
            } else {
                EditorMaskEntry entry;
                entry.number = m_editorMasks.size() + 1;
                entry.contour = contour;
                entry.answer.clear();
                m_editorMasks.append(entry);
                emit masksChanged();
                regenerateGroupedAnswerOptions();
                if (m_imageProvider && m_selectedPuzzleId > 0) {
                    m_imageProvider->invalidatePuzzle(m_selectedPuzzleId);
                }
                refreshEditorPreview();
                setStatusMessage(QStringLiteral("Маска %1 добавлена — укажите ответ").arg(entry.number));
            }
        }
        if (m_maskProcessing) {
            m_maskProcessing = false;
            emit maskProcessingChanged();
        }
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([processor, source, relX, relY, relW, relH]() -> QString {
        if (!processor) {
            return {};
        }
        if (relW < 0.015 && relH < 0.015) {
            return processor->extractContour(source, relX, relY, 24);
        }
        return processor->extractContourInRect(source, relX, relY, relW, relH);
    }));

    return true;
}

void AdminViewModel::clearMask()
{
    if (m_selectedImageSlot != 0) {
        return;
    }

    m_editorMasks.clear();
    m_answerOptions.clear();
    emit answerOptionsChanged();
    emit masksChanged();
    refreshEditorPreview();
    setStatusMessage(QStringLiteral("Все маски сброшены"));
}

QString AdminViewModel::label(const QString &key) const
{
    if (!m_database) {
        return key;
    }
    return m_database->localizedString(key);
}

bool AdminViewModel::selectPresetByName(const QString &name)
{
    for (const QVariant &item : m_presets) {
        const QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("name")).toString() == name) {
            setSelectedPresetId(map.value(QStringLiteral("id")).toInt());
            return true;
        }
    }
    return false;
}

void AdminViewModel::rebuildPresetList()
{
    m_presets.clear();
    if (!m_database) {
        return;
    }

    const QVector<GamePresetInfo> presets = m_database->listPresets();
    for (const GamePresetInfo &preset : presets) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), preset.id);
        map.insert(QStringLiteral("name"), preset.name);
        map.insert(QStringLiteral("roundCount"), preset.roundCount);
        m_presets.append(map);
    }
}

void AdminViewModel::rebuildCatalogRounds()
{
    m_catalogRounds.clear();
    if (!m_database) {
        return;
    }

    const QVector<RoundInfo> rounds = m_database->listAllRounds();
    for (const RoundInfo &round : rounds) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), round.id);
        map.insert(QStringLiteral("title"), m_database->localizedString(round.titleKey));
        map.insert(QStringLiteral("rule"), m_database->localizedString(round.ruleTextKey));
        map.insert(QStringLiteral("layoutType"), round.layoutType);
        map.insert(QStringLiteral("dataStatus"), evaluateRoundTemplateStatus(round.id));
        m_catalogRounds.append(map);
    }
}

void AdminViewModel::rebuildPresetRoundIds()
{
    m_presetRoundIdsList.clear();
    if (!m_database || m_selectedPresetId <= 0) {
        return;
    }

    const QVector<int> roundIds = m_database->presetRoundIds(m_selectedPresetId);
    for (int id : roundIds) {
        m_presetRoundIdsList.append(id);
    }
}

void AdminViewModel::rebuildPuzzleList()
{
    m_puzzles.clear();
    if (!m_database || m_selectedRoundId <= 0) {
        return;
    }

    const QVector<PuzzleInfo> items = m_database->listPuzzlesForRound(m_selectedRoundId);
    int displayNumber = 0;
    for (const PuzzleInfo &puzzle : items) {
        if (displayNumber >= GameConstants::MAX_PUZZLES_PER_ROUND) {
            break;
        }
        ++displayNumber;
        QVariantMap map;
        map.insert(QStringLiteral("id"), puzzle.id);
        map.insert(QStringLiteral("answer"), puzzle.correctAnswer);
        map.insert(QStringLiteral("sortOrder"), displayNumber);
        map.insert(QStringLiteral("title"),
                   label(QStringLiteral("ui.editor.question_item_format")).arg(displayNumber));
        m_puzzles.append(map);
    }
}

void AdminViewModel::loadPuzzleEditor(int puzzleId)
{
    if (!m_database || puzzleId <= 0) {
        clearPuzzleEditor();
        return;
    }

    const PuzzleInfo puzzle = m_database->puzzleById(puzzleId);
    if (puzzle.id <= 0) {
        clearPuzzleEditor();
        return;
    }

    m_slotImageCache.clear();
    m_slotPendingBytes.clear();
    preloadPuzzleSlotImages(puzzleId);
    m_editAnswer = puzzle.correctAnswer;
    m_editHint = m_database->localizedString(puzzle.hintTextKey);
    m_selectedTemplateId = puzzle.templateId;

    QStringList quotes;
    if (!puzzle.quoteSlotsJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(puzzle.quoteSlotsJson.toUtf8());
        for (const QJsonValue &value : doc.array()) {
            quotes.append(value.toString());
        }
    }

    ensureConfigCardTextSize();
    for (int i = 0; i < m_configCardTexts.size(); ++i) {
        m_configCardTexts[i] = i < quotes.size() ? quotes.at(i) : QString();
    }
    m_editQuotes = quotes.join(QStringLiteral("\n"));

    m_answerOptions.clear();
    if (!puzzle.correctOrder.isEmpty()) {
        const QJsonDocument optionsDoc = QJsonDocument::fromJson(puzzle.correctOrder.toUtf8());
        if (optionsDoc.isArray()) {
            for (const QJsonValue &value : optionsDoc.array()) {
                const QString option = value.toString().trimmed();
                if (option.isEmpty() || option.startsWith(kHybridAnimPrefix)) {
                    continue;
                }
                m_answerOptions.append(option);
            }
        }
    }
    setHybridAnimationStyle(parseHybridAnimStyleFromCorrectOrder(puzzle.correctOrder));
    emit answerOptionsChanged();
    emit cardTextsChanged();

    m_selectedImageSlot = 0;
    emit selectedImageSlotChanged();
    notifySlotImagesChanged();

    loadImageSlot(0);
    loadEditorMasks(puzzleId);
    regenerateGroupedAnswerOptions();

    emit editAnswerChanged();
    emit editHintChanged();
    emit editQuotesChanged();

    if (isPhotoMaskRound()) {
        setStatusMessage(hasMaskContour()
                             ? QStringLiteral("Загадка #%1 — превью как в игре").arg(puzzle.sortOrder)
                             : QStringLiteral("Загадка #%1 — загрузите фото и отметьте объект").arg(puzzle.sortOrder));
    } else {
        setStatusMessage(QStringLiteral("Загадка #%1").arg(puzzle.sortOrder));
    }
}

void AdminViewModel::preloadPuzzleSlotImages(int puzzleId)
{
    if (!m_database || puzzleId <= 0) {
        return;
    }

    const int slotCount = imageSlotCount();
    for (int slot = 0; slot < slotCount; ++slot) {
        const QByteArray imageData = m_database->puzzleImageData(puzzleId, slot);
        if (imageData.isEmpty()) {
            continue;
        }
        QImage image;
        if (image.loadFromData(imageData)) {
            m_slotImageCache.insert(slot, image);
        }
    }
}

void AdminViewModel::clearPuzzleEditor()
{
    m_selectedPuzzleId = 0;
    m_selectedTemplateId = 0;
    m_selectedImageSlot = 0;
    m_editAnswer.clear();
    m_editHint.clear();
    m_editQuotes.clear();
    m_configCardTexts.clear();
    m_answerOptions.clear();
    setHybridAnimationStyle(QStringLiteral("soft"));
    m_editorMasks.clear();
    m_sourceImage = QImage();
    m_previewImage = QImage();
    m_pendingImageBytes.clear();
    m_slotImageCache.clear();
    m_slotPendingBytes.clear();

    if (m_imageProvider) {
        m_imageProvider->clearPreviewImage();
    }

    emit selectedPuzzleIdChanged();
    emit selectedImageSlotChanged();
    notifySlotImagesChanged();
    emit editAnswerChanged();
    emit editHintChanged();
    emit editQuotesChanged();
    emit answerOptionsChanged();
    emit cardTextsChanged();
    emit masksChanged();
    emit previewImageChanged();
    emit puzzleImageChanged();
}

void AdminViewModel::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    emit statusMessageChanged();
}

void AdminViewModel::updatePreviewProvider()
{
    if (m_imageProvider) {
        if (m_previewImage.isNull()) {
            m_imageProvider->clearPreviewImage();
        } else {
            m_imageProvider->setPreviewImage(m_previewImage);
        }
    }
    bumpPreviewRevision();
}

void AdminViewModel::bumpPreviewRevision()
{
    ++m_previewRevision;
    emit previewImageChanged();
}

void AdminViewModel::notifySlotImagesChanged()
{
    ++m_slotImageRevision;
    emit slotImagesChanged();
}

QString AdminViewModel::quoteSlotsJsonFromEditor() const
{
    if (!m_configCardTexts.isEmpty()) {
        QJsonArray array;
        for (const QString &text : m_configCardTexts) {
            array.append(text.trimmed());
        }
        return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    }

    const QStringList lines = m_editQuotes.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                                 Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        return {};
    }

    QJsonArray array;
    for (const QString &line : lines) {
        array.append(line.trimmed());
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QString AdminViewModel::answerOptionsJsonFromEditor() const
{
    if (m_selectedRoundLayoutType == QStringLiteral("SINGLE_HYBRID")) {
        QJsonArray array;
        array.append(QStringLiteral("%1%2")
                         .arg(QString(kHybridAnimPrefix), normalizeHybridAnimStyleValue(m_hybridAnimationStyle)));
        return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    }

    if (m_answerOptions.isEmpty()) {
        return {};
    }

    QJsonArray array;
    for (const QString &option : m_answerOptions) {
        const QString trimmed = option.trimmed();
        if (!trimmed.isEmpty()) {
            array.append(trimmed);
        }
    }
    if (array.isEmpty()) {
        return {};
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}
