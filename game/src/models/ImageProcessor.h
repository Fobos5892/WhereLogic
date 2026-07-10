#pragma once

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>

struct MaskLayer {
    int number = 0;
    QString contour;
};

class ImageProcessor : public QObject
{
    Q_OBJECT

public:
    explicit ImageProcessor(QObject *parent = nullptr);

    Q_INVOKABLE QString extractContour(const QImage &image, double relX, double relY, int tolerance = 20, int precision = 3);
    Q_INVOKABLE QString extractContourInRect(const QImage &image,
                                             double relX,
                                             double relY,
                                             double relW,
                                             double relH,
                                             int precision = 3);
    Q_INVOKABLE QImage applyMask(const QImage &image, const QString &contourPoints);
    Q_INVOKABLE QImage applyHideMask(const QImage &image, const QString &contourPoints);
    Q_INVOKABLE QImage applyHideMasks(const QImage &image,
                                      const QVector<MaskLayer> &masks,
                                      const QVector<int> &revealedMaskNumbers = {});

signals:
    void contourExtracted(const QString &contourPoints);
    void processingFailed(const QString &reason);
};
