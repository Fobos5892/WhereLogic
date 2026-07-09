#include "PuzzleImageProvider.h"

#include "DatabaseManager.h"
#include "ImageProcessor.h"

#include <QMetaObject>
#include <QMutexLocker>
#include <QPainter>
#include <QThread>
#include <type_traits>

namespace {

QImage placeholderSlotImage(int slotIndex)
{
    QImage image(360, 480, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(28, 32, 40));

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(96, 108, 128), 4));
    painter.setBrush(QColor(40, 46, 58));
    painter.drawRoundedRect(image.rect().adjusted(24, 24, -24, -24), 16, 16);
    painter.setPen(QColor(196, 204, 220));
    QFont font = painter.font();
    font.setPixelSize(72);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(image.rect(), Qt::AlignCenter, QString::number(slotIndex + 1));
    return image;
}

QByteArray fetchPuzzleImageBytes(DatabaseManager *database, int puzzleId, int slotIndex)
{
    if (!database) {
        return {};
    }

    if (QThread::currentThread() == database->thread()) {
        return database->fetchPuzzleImageData(puzzleId, slotIndex);
    }

    QByteArray data;
    const bool invoked = QMetaObject::invokeMethod(
        database,
        "fetchPuzzleImageData",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QByteArray, data),
        Q_ARG(int, puzzleId),
        Q_ARG(int, slotIndex));
    if (!invoked) {
        return database->fetchPuzzleImageData(puzzleId, slotIndex);
    }
    return data;
}

template<typename Fn>
auto runOnDatabaseThread(DatabaseManager *database, Fn &&fn)
{
    using Result = std::invoke_result_t<Fn>;
    if (!database) {
        if constexpr (std::is_void_v<Result>) {
            return;
        }
        return Result{};
    }

    if (QThread::currentThread() == database->thread()) {
        return fn();
    }

    if constexpr (std::is_void_v<Result>) {
        QMetaObject::invokeMethod(database, std::forward<Fn>(fn), Qt::BlockingQueuedConnection);
    } else {
        Result result{};
        QMetaObject::invokeMethod(database,
                                  [&]() { result = fn(); },
                                  Qt::BlockingQueuedConnection);
        return result;
    }
}

} // namespace

PuzzleImageProvider::PuzzleImageProvider(DatabaseManager *database)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_database(database)
{
}

void PuzzleImageProvider::setImageProcessor(ImageProcessor *processor)
{
    m_imageProcessor = processor;
}

void PuzzleImageProvider::invalidatePuzzle(int puzzleId)
{
    QMutexLocker locker(&m_cacheMutex);
    const QString prefix = QString::number(puzzleId) + QLatin1Char(':');
    for (auto it = m_hiddenCache.begin(); it != m_hiddenCache.end();) {
        if (it.key().startsWith(prefix)) {
            it = m_hiddenCache.erase(it);
        } else {
            ++it;
        }
    }
}

void PuzzleImageProvider::clearHiddenCache()
{
    QMutexLocker locker(&m_cacheMutex);
    m_hiddenCache.clear();
}

QImage PuzzleImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)

    QString path = id;
    const int queryPos = path.indexOf(QLatin1Char('?'));
    if (queryPos >= 0) {
        path = path.left(queryPos);
    }

    if (path.startsWith(QStringLiteral("preview"))) {
        if (size) {
            *size = m_previewImage.size();
        }
        return m_previewImage;
    }

    const QStringList parts = path.split(QLatin1Char('/'));
    if (parts.isEmpty()) {
        return {};
    }

    const int puzzleId = parts.at(0).toInt();
    if (puzzleId <= 0 || !m_database) {
        return {};
    }

    QImage image;
    if (parts.size() >= 2 && parts.at(1) == QStringLiteral("hide")) {
        QVector<int> revealed;
        if (parts.size() >= 3) {
            const QStringList revealedParts = parts.at(2).split(QLatin1Char(','), Qt::SkipEmptyParts);
            for (const QString &part : revealedParts) {
                bool ok = false;
                const int number = part.toInt(&ok);
                if (ok && number > 0) {
                    revealed.append(number);
                }
            }
        }
        image = loadHiddenPuzzleImage(puzzleId, revealed);
    } else {
        const int slotIndex = parts.size() >= 2 ? parts.at(1).toInt() : 0;
        image = loadPuzzleImage(puzzleId, slotIndex);
    }

    if (size) {
        *size = image.size();
    }
    return image;
}

QImage PuzzleImageProvider::loadPuzzleImage(int puzzleId, int slotIndex) const
{
    const QByteArray data = fetchPuzzleImageBytes(m_database, puzzleId, slotIndex);
    if (data.isEmpty()) {
        return placeholderSlotImage(slotIndex);
    }

    QImage image;
    if (!image.loadFromData(data)) {
        return placeholderSlotImage(slotIndex);
    }
    return image;
}

QVector<MaskLayer> PuzzleImageProvider::maskLayersForPuzzle(int puzzleId) const
{
    return runOnDatabaseThread(m_database, [&]() {
        QVector<MaskLayer> layers;
        if (!m_database || puzzleId <= 0) {
            return layers;
        }

        const QVector<PuzzleMaskInfo> masks = m_database->listPuzzleMasks(puzzleId);
        layers.reserve(masks.size());
        for (const PuzzleMaskInfo &mask : masks) {
            MaskLayer layer;
            layer.number = mask.sortOrder;
            layer.contour = mask.contourPoints;
            layers.append(layer);
        }

        if (!layers.isEmpty()) {
            return layers;
        }

        const PuzzleInfo puzzle = m_database->puzzleById(puzzleId);
        if (puzzle.templateId > 0) {
            const QString contour = m_database->maskTemplateContour(puzzle.templateId);
            if (!contour.isEmpty()) {
                MaskLayer layer;
                layer.number = 1;
                layer.contour = contour;
                layers.append(layer);
            }
        }

        return layers;
    });
}

QImage PuzzleImageProvider::loadHiddenPuzzleImage(int puzzleId, const QVector<int> &revealedMaskNumbers) const
{
    const QVector<MaskLayer> layers = maskLayersForPuzzle(puzzleId);
    if (layers.isEmpty()) {
        return loadPuzzleImage(puzzleId, 0);
    }

    QString cacheKey = QStringLiteral("%1:").arg(puzzleId);
    for (const MaskLayer &layer : layers) {
        cacheKey += QString::number(layer.number) + QLatin1Char('|') + QString::number(qHash(layer.contour)) + QLatin1Char(';');
    }
    cacheKey += QStringLiteral("r:");
    for (int number : revealedMaskNumbers) {
        cacheKey += QString::number(number) + QLatin1Char(',');
    }

    {
        QMutexLocker locker(&m_cacheMutex);
        const auto it = m_hiddenCache.constFind(cacheKey);
        if (it != m_hiddenCache.constEnd()) {
            return it.value();
        }
    }

    QImage image = loadPuzzleImage(puzzleId, 0);
    if (image.isNull() || !m_imageProcessor) {
        return image;
    }

    const QImage hidden = m_imageProcessor->applyHideMasks(image, layers, revealedMaskNumbers);
    if (!hidden.isNull()) {
        QMutexLocker locker(&m_cacheMutex);
        m_hiddenCache.insert(cacheKey, hidden);
        return hidden;
    }

    return image;
}

void PuzzleImageProvider::setPreviewImage(const QImage &image)
{
    m_previewImage = image;
}

void PuzzleImageProvider::clearPreviewImage()
{
    m_previewImage = QImage();
}
