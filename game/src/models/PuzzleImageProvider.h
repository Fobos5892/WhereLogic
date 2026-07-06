#pragma once

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

class DatabaseManager;
class ImageProcessor;

class PuzzleImageProvider : public QQuickImageProvider
{
public:
    explicit PuzzleImageProvider(DatabaseManager *database);

    void setImageProcessor(ImageProcessor *processor);
    void invalidatePuzzle(int puzzleId);
    void clearHiddenCache();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    void setPreviewImage(const QImage &image);
    void clearPreviewImage();

private:
    QImage loadPuzzleImage(int puzzleId, int slotIndex) const;
    QImage loadHiddenPuzzleImage(int puzzleId) const;

    DatabaseManager *m_database = nullptr;
    ImageProcessor *m_imageProcessor = nullptr;
    QImage m_previewImage;
    mutable QMutex m_cacheMutex;
    mutable QHash<QString, QImage> m_hiddenCache;
};
