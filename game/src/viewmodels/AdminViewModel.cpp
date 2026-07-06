#include "AdminViewModel.h"

#include "../models/DatabaseManager.h"
#include "../models/ImageProcessor.h"
#include "../models/PuzzleImageProvider.h"

#include <QBuffer>
#include <QFile>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QVariantMap>
#include <QtConcurrent>

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
    if (m_selectedRoundLayoutType == QStringLiteral("FULL_MASK")) {
        return 1;
    }
    if (m_selectedRoundLayoutType == QStringLiteral("TEXT_ONLY")) {
        return 0;
    }
    if (m_selectedRoundLayoutType == QStringLiteral("SINGLE_HYBRID")) {
        return 2;
    }
    return 4;
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
    m_maskContour.clear();

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

    if (slotIndex == 0 && m_selectedTemplateId > 0 && m_database) {
        m_maskContour = m_database->maskTemplateContour(m_selectedTemplateId);
    }

    if (m_sourceImage.isNull() && slotIndex == 0 && m_selectedTemplateId > 0 && m_database) {
        const QByteArray templateData = m_database->maskTemplateImageData(m_selectedTemplateId);
        if (!templateData.isEmpty()) {
            m_sourceImage.loadFromData(templateData);
        }
    }

    emit maskContourChanged();
    refreshEditorPreview();
    ++m_puzzleImageRevision;
    emit puzzleImageChanged();
}

void AdminViewModel::refreshEditorPreview()
{
    if (m_sourceImage.isNull()) {
        m_previewImage = QImage();
        updatePreviewProvider();
        return;
    }

    const bool hide = isPhotoMaskRound() && m_selectedImageSlot == 0 && !m_maskContour.isEmpty()
                      && m_imageProcessor;
    if (!hide) {
        m_previewImage = m_sourceImage;
        updatePreviewProvider();
        return;
    }

    const int jobId = ++m_previewJobId;
    const QImage source = m_sourceImage;
    const QString contour = m_maskContour;
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
    watcher->setFuture(QtConcurrent::run([processor, source, contour]() -> QImage {
        if (!processor) {
            return source;
        }
        const QImage hidden = processor->applyHideMask(source, contour);
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

    const QString name = m_editPresetName.trimmed().isEmpty()
        ? QStringLiteral("Новая игра")
        : m_editPresetName.trimmed();
    const int id = m_database->createPreset(name);
    if (id <= 0) {
        setStatusMessage(QStringLiteral("Не удалось создать пресет"));
        return;
    }

    refreshPresets();
    setSelectedPresetId(id);
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
    if (!m_database || m_selectedPresetId <= 0) {
        return;
    }

    if (!m_database->deletePreset(m_selectedPresetId)) {
        setStatusMessage(QStringLiteral("Не удалось удалить пресет"));
        return;
    }

    m_selectedPresetId = 0;
    refreshPresets();
    setStatusMessage(QStringLiteral("Пресет удалён"));
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

void AdminViewModel::selectPuzzle(int puzzleId)
{
    cacheCurrentSlotState();
    m_selectedPuzzleId = puzzleId;
    m_selectedImageSlot = 0;
    emit selectedPuzzleIdChanged();
    emit selectedImageSlotChanged();
    loadPuzzleEditor(puzzleId);
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
    focusPhotoMaskRound();
    if (!isPhotoMaskRound()) {
        return;
    }
    createPuzzle();
}

void AdminViewModel::createPuzzle()
{
    if (!m_database || m_selectedRoundId <= 0) {
        setStatusMessage(QStringLiteral("Сначала выберите раунд"));
        return;
    }

    const int puzzleId = m_database->createPuzzle(m_selectedRoundId,
                                                  QStringLiteral("Ответ"),
                                                  QStringLiteral("Подсказка"));
    if (puzzleId <= 0) {
        setStatusMessage(QStringLiteral("Не удалось создать загадку"));
        return;
    }

    refreshPuzzles();
    selectPuzzle(puzzleId);
    setStatusMessage(QStringLiteral("Загадка создана"));
}

void AdminViewModel::savePuzzle()
{
    if (!m_database || m_selectedPuzzleId <= 0) {
        return;
    }

    cacheCurrentSlotState();

    if (m_editAnswer.trimmed().isEmpty()) {
        setStatusMessage(QStringLiteral("Введите правильный ответ"));
        return;
    }

    if (isPhotoMaskRound() && m_maskContour.isEmpty()) {
        setStatusMessage(QStringLiteral("Нажмите на объект на фото, чтобы скрыть его"));
        return;
    }

    if (isPhotoMaskRound() && !slotHasImage(0)) {
        setStatusMessage(QStringLiteral("Загрузите фото в первую ячейку"));
        return;
    }

    if (!m_database->updatePuzzle(m_selectedPuzzleId,
                                  m_editAnswer,
                                  m_editHint,
                                  quoteSlotsJsonFromEditor())) {
        setStatusMessage(QStringLiteral("Не удалось сохранить загадку"));
        return;
    }

    if (!m_sourceImage.isNull()) {
        QByteArray imageBytes;
        QBuffer buffer(&imageBytes);
        buffer.open(QIODevice::WriteOnly);
        m_sourceImage.save(&buffer, "PNG");
        m_database->upsertPuzzleImage(m_selectedPuzzleId, m_selectedImageSlot, imageBytes);
        m_slotImageCache.insert(m_selectedImageSlot, m_sourceImage);
    }

    if (!m_pendingImageBytes.isEmpty()) {
        m_database->upsertPuzzleImage(m_selectedPuzzleId, m_selectedImageSlot, m_pendingImageBytes);
        m_slotPendingBytes.remove(m_selectedImageSlot);
        m_pendingImageBytes.clear();
    }

    for (auto it = m_slotImageCache.constBegin(); it != m_slotImageCache.constEnd(); ++it) {
        if (it.key() == m_selectedImageSlot || it.value().isNull()) {
            continue;
        }
        QByteArray imageBytes;
        QBuffer buffer(&imageBytes);
        buffer.open(QIODevice::WriteOnly);
        it.value().save(&buffer, "PNG");
        m_database->upsertPuzzleImage(m_selectedPuzzleId, it.key(), imageBytes);
    }

    for (auto it = m_slotPendingBytes.constBegin(); it != m_slotPendingBytes.constEnd(); ++it) {
        if (it.key() == m_selectedImageSlot) {
            continue;
        }
        m_database->upsertPuzzleImage(m_selectedPuzzleId, it.key(), it.value());
    }
    m_slotPendingBytes.clear();

    if (!m_maskContour.isEmpty()) {
        QImage maskSource = m_slotImageCache.value(0);
        if (maskSource.isNull() && m_selectedImageSlot == 0) {
            maskSource = m_sourceImage;
        }
        if (maskSource.isNull() && m_database) {
            const QByteArray slot0Data = m_database->puzzleImageData(m_selectedPuzzleId, 0);
            if (!slot0Data.isEmpty()) {
                maskSource.loadFromData(slot0Data);
            }
        }

        if (!maskSource.isNull()) {
            QByteArray imageBytes;
            QBuffer buffer(&imageBytes);
            buffer.open(QIODevice::WriteOnly);
            maskSource.save(&buffer, "PNG");

            const int templateId = m_database->upsertMaskTemplate(m_selectedTemplateId,
                                                                  QStringLiteral("mask_%1").arg(m_selectedPuzzleId),
                                                                  imageBytes,
                                                                  m_maskContour);
            if (templateId > 0) {
                m_selectedTemplateId = templateId;
                m_database->setPuzzleTemplateId(m_selectedPuzzleId, templateId);
            }
        }
    }

    ++m_puzzleImageRevision;
    emit puzzleImageChanged();
    if (m_imageProvider && m_selectedPuzzleId > 0) {
        m_imageProvider->invalidatePuzzle(m_selectedPuzzleId);
    }
    refreshPuzzles();
    loadPuzzleEditor(m_selectedPuzzleId);
    setStatusMessage(QStringLiteral("Загадка сохранена"));
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
        m_maskContour.clear();
        emit maskContourChanged();
    }

    refreshEditorPreview();

    if (m_selectedPuzzleId > 0 && m_database) {
        m_database->upsertPuzzleImage(m_selectedPuzzleId, m_selectedImageSlot, m_pendingImageBytes);
        m_slotPendingBytes.remove(m_selectedImageSlot);
        m_pendingImageBytes.clear();
        ++m_puzzleImageRevision;
        emit puzzleImageChanged();
    }

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
    if (m_imageProcessing) {
        return false;
    }

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
    setImageProcessingBusy(true);
    setStatusMessage(QStringLiteral("Обработка изображения…"));

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, jobId]() {
        if (jobId == m_regionJobId) {
            const QString contour = watcher->result();
            if (contour.isEmpty()) {
                setStatusMessage(QStringLiteral("Не удалось выделить объект — обведите его рамкой"));
            } else {
                m_maskContour = contour;
                emit maskContourChanged();
                if (m_imageProvider && m_selectedPuzzleId > 0) {
                    m_imageProvider->invalidatePuzzle(m_selectedPuzzleId);
                }
                refreshEditorPreview();
                setStatusMessage(QStringLiteral("Объект скрыт — так увидит игрок. Сохраните загадку"));
            }
        }
        setImageProcessingBusy(false);
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

    m_maskContour.clear();
    emit maskContourChanged();
    refreshEditorPreview();
    setStatusMessage(QStringLiteral("Маска сброшена"));
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
        map.insert(QStringLiteral("layoutType"), round.layoutType);
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
    for (const PuzzleInfo &puzzle : items) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), puzzle.id);
        map.insert(QStringLiteral("answer"), puzzle.correctAnswer);
        map.insert(QStringLiteral("sortOrder"), puzzle.sortOrder);
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
    m_editQuotes = quotes.join(QStringLiteral("\n"));

    m_selectedImageSlot = 0;
    emit selectedImageSlotChanged();
    emit imageSlotsChanged();

    loadImageSlot(0);

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

void AdminViewModel::clearPuzzleEditor()
{
    m_selectedPuzzleId = 0;
    m_selectedTemplateId = 0;
    m_selectedImageSlot = 0;
    m_editAnswer.clear();
    m_editHint.clear();
    m_editQuotes.clear();
    m_maskContour.clear();
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
    emit imageSlotsChanged();
    emit editAnswerChanged();
    emit editHintChanged();
    emit editQuotesChanged();
    emit maskContourChanged();
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

QString AdminViewModel::quoteSlotsJsonFromEditor() const
{
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
