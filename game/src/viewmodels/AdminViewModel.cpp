#include "AdminViewModel.h"

#include "../models/DatabaseManager.h"
#include "../models/ImageProcessor.h"
#include "../models/PuzzleImageProvider.h"

#include <QBuffer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QVariantMap>

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
    return QStringLiteral("image://puzzle/%1/0?rev=%2").arg(m_selectedPuzzleId).arg(m_puzzleImageRevision);
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
    if (m_selectedPuzzleId == puzzleId) {
        return;
    }
    m_selectedPuzzleId = puzzleId;
    emit selectedPuzzleIdChanged();
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

    if (m_editAnswer.trimmed().isEmpty()) {
        setStatusMessage(QStringLiteral("Введите правильный ответ"));
        return;
    }

    if (isPhotoMaskRound() && m_maskContour.isEmpty()) {
        setStatusMessage(QStringLiteral("Нажмите на объект на фото, чтобы скрыть его"));
        return;
    }

    if (isPhotoMaskRound() && m_sourceImage.isNull()) {
        setStatusMessage(QStringLiteral("Загрузите фото"));
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
        m_database->upsertPuzzleImage(m_selectedPuzzleId, 0, imageBytes);
    }

    if (!m_pendingImageBytes.isEmpty()) {
        m_database->upsertPuzzleImage(m_selectedPuzzleId, 0, m_pendingImageBytes);
        m_pendingImageBytes.clear();
    }

    if (!m_maskContour.isEmpty() && !m_sourceImage.isNull()) {
        QByteArray imageBytes;
        QBuffer buffer(&imageBytes);
        buffer.open(QIODevice::WriteOnly);
        m_sourceImage.save(&buffer, "PNG");

        const int templateId = m_database->upsertMaskTemplate(m_selectedTemplateId,
                                                              QStringLiteral("mask_%1").arg(m_selectedPuzzleId),
                                                              imageBytes,
                                                              m_maskContour);
        if (templateId > 0) {
            m_selectedTemplateId = templateId;
            m_database->setPuzzleTemplateId(m_selectedPuzzleId, templateId);
        }
    }

    ++m_puzzleImageRevision;
    emit puzzleImageChanged();
    refreshPuzzles();
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
    m_previewImage = m_sourceImage;
    m_maskContour.clear();
    emit maskContourChanged();

    if (m_selectedPuzzleId > 0 && m_database) {
        m_database->upsertPuzzleImage(m_selectedPuzzleId, 0, m_pendingImageBytes);
        m_pendingImageBytes.clear();
        ++m_puzzleImageRevision;
        emit puzzleImageChanged();
    }

    updatePreviewProvider();
    setStatusMessage(QStringLiteral("Фото загружено — нажмите на объект, которого не хватает"));
    return true;
}

bool AdminViewModel::markMissingArea(double relX, double relY)
{
    if (m_sourceImage.isNull()) {
        setStatusMessage(QStringLiteral("Сначала загрузите фото"));
        return false;
    }

    if (!m_imageProcessor) {
        setStatusMessage(QStringLiteral("Обработка изображений недоступна (соберите с OpenCV)"));
        return false;
    }

    const QString contour = m_imageProcessor->extractContour(m_sourceImage, relX, relY, 24);
    if (contour.isEmpty()) {
        setStatusMessage(QStringLiteral("Не удалось выделить область — попробуйте другое место"));
        return false;
    }

    m_maskContour = contour;
    emit maskContourChanged();

    const QImage hidden = m_imageProcessor->applyHideMask(m_sourceImage, contour);
    if (!hidden.isNull()) {
        m_previewImage = hidden;
        updatePreviewProvider();
    }

    setStatusMessage(QStringLiteral("Область выделена — нажмите «Сохранить загадку»"));
    return true;
}

void AdminViewModel::clearMask()
{
    m_maskContour.clear();
    emit maskContourChanged();

    if (m_selectedPuzzleId > 0 && m_database) {
        const QByteArray data = m_database->puzzleImageData(m_selectedPuzzleId, 0);
        if (!data.isEmpty()) {
            m_sourceImage.loadFromData(data);
            m_previewImage = m_sourceImage;
        }
    } else {
        m_previewImage = m_sourceImage;
    }

    updatePreviewProvider();
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

    m_maskContour.clear();
    if (puzzle.templateId > 0) {
        m_maskContour = m_database->maskTemplateContour(puzzle.templateId);
    }
    emit maskContourChanged();

    m_sourceImage = QImage();
    m_previewImage = QImage();
    const QByteArray imageData = m_database->puzzleImageData(puzzleId, 0);
    if (!imageData.isEmpty()) {
        m_sourceImage.loadFromData(imageData);
        m_previewImage = m_sourceImage;
    } else if (puzzle.templateId > 0) {
        const QByteArray templateData = m_database->maskTemplateImageData(puzzle.templateId);
        m_sourceImage.loadFromData(templateData);
        m_previewImage = m_sourceImage;
    }

    if (!m_maskContour.isEmpty() && m_imageProcessor && !m_sourceImage.isNull()) {
        const QImage hidden = m_imageProcessor->applyHideMask(m_sourceImage, m_maskContour);
        if (!hidden.isNull()) {
            m_previewImage = hidden;
        }
    }

    m_pendingImageBytes.clear();
    updatePreviewProvider();
    ++m_puzzleImageRevision;

    emit editAnswerChanged();
    emit editHintChanged();
    emit editQuotesChanged();
    emit puzzleImageChanged();
}

void AdminViewModel::clearPuzzleEditor()
{
    m_selectedPuzzleId = 0;
    m_selectedTemplateId = 0;
    m_editAnswer.clear();
    m_editHint.clear();
    m_editQuotes.clear();
    m_maskContour.clear();
    m_sourceImage = QImage();
    m_previewImage = QImage();
    m_pendingImageBytes.clear();

    if (m_imageProvider) {
        m_imageProvider->clearPreviewImage();
    }

    emit selectedPuzzleIdChanged();
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
