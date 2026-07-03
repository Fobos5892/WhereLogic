#pragma once

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QString>

class ImageProcessor : public QObject
{
    Q_OBJECT

public:
    explicit ImageProcessor(QObject *parent = nullptr);

    Q_INVOKABLE QString extractContour(const QImage &image, double relX, double relY, int tolerance = 20);
    Q_INVOKABLE QImage applyMask(const QImage &image, const QString &contourPoints);
    Q_INVOKABLE QImage applyHideMask(const QImage &image, const QString &contourPoints);

signals:
    void contourExtracted(const QString &contourPoints);
    void processingFailed(const QString &reason);
};
