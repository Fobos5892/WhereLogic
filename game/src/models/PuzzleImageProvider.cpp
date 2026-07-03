#include "PuzzleImageProvider.h"

#include "DatabaseManager.h"
#include "ImageProcessor.h"

PuzzleImageProvider::PuzzleImageProvider(DatabaseManager *database)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_database(database)
{
}

void PuzzleImageProvider::setImageProcessor(ImageProcessor *processor)
{
    m_imageProcessor = processor;
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
        image = loadHiddenPuzzleImage(puzzleId);
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
    const QByteArray data = m_database->puzzleImageData(puzzleId, slotIndex);
    if (data.isEmpty()) {
        return {};
    }

    QImage image;
    if (!image.loadFromData(data)) {
        return {};
    }
    return image;
}

QImage PuzzleImageProvider::loadHiddenPuzzleImage(int puzzleId) const
{
    QImage image = loadPuzzleImage(puzzleId, 0);
    if (image.isNull() || !m_imageProcessor) {
        return image;
    }

    const PuzzleInfo puzzle = m_database->puzzleById(puzzleId);
    if (puzzle.templateId <= 0) {
        return image;
    }

    const QString contour = m_database->maskTemplateContour(puzzle.templateId);
    if (contour.isEmpty()) {
        return image;
    }

    return m_imageProcessor->applyHideMask(image, contour);
}

void PuzzleImageProvider::setPreviewImage(const QImage &image)
{
    m_previewImage = image;
}

void PuzzleImageProvider::clearPreviewImage()
{
    m_previewImage = QImage();
}
