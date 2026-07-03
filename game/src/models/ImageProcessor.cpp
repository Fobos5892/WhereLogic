#include "ImageProcessor.h"

#ifdef HAS_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#endif

#include <QBuffer>

namespace {

#ifdef HAS_OPENCV
cv::Mat qImageToMat(const QImage &image)
{
    QImage converted = image.convertToFormat(QImage::Format_RGBA8888);
    return cv::Mat(converted.height(),
                   converted.width(),
                   CV_8UC4,
                   const_cast<uchar *>(converted.bits()),
                   static_cast<size_t>(converted.bytesPerLine()))
        .clone();
}

QImage matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC4) {
        return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGBA8888).copy();
    }
    if (mat.type() == CV_8UC3) {
        return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888).copy();
    }
    return {};
}
#endif

} // namespace

ImageProcessor::ImageProcessor(QObject *parent)
    : QObject(parent)
{
}

QString ImageProcessor::extractContour(const QImage &image, double relX, double relY, int tolerance)
{
    if (image.isNull()) {
        emit processingFailed(QStringLiteral("Image is empty"));
        return {};
    }

#ifndef HAS_OPENCV
    Q_UNUSED(relX)
    Q_UNUSED(relY)
    Q_UNUSED(tolerance)
    emit processingFailed(QStringLiteral("OpenCV is not available"));
    return {};
#else
    cv::Mat mat = qImageToMat(image);
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGBA2BGR);

    const int pixelX = static_cast<int>(relX * bgr.cols);
    const int pixelY = static_cast<int>(relY * bgr.rows);
    cv::Point seed(pixelX, pixelY);

    cv::Mat mask = cv::Mat::zeros(bgr.rows + 2, bgr.cols + 2, CV_8UC1);
    cv::floodFill(bgr, mask, seed, cv::Scalar(0, 255, 0), nullptr, cv::Scalar(tolerance, tolerance, tolerance),
                  cv::Scalar(tolerance, tolerance, tolerance), 4 | cv::FLOODFILL_MASK_ONLY | (255 << 8));

    cv::Mat contourMask = mask(cv::Rect(1, 1, bgr.cols, bgr.rows));
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(contourMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) {
        emit processingFailed(QStringLiteral("No contour found"));
        return {};
    }

    QStringList points;
    for (const cv::Point &point : contours.front()) {
        const double rx = static_cast<double>(point.x) / bgr.cols;
        const double ry = static_cast<double>(point.y) / bgr.rows;
        points.append(QStringLiteral("%1,%2").arg(rx, 0, 'f', 4).arg(ry, 0, 'f', 4));
    }

    const QString contour = points.join(QLatin1Char(';'));
    emit contourExtracted(contour);
    return contour;
#endif
}

QImage ImageProcessor::applyMask(const QImage &image, const QString &contourPoints)
{
    if (image.isNull()) {
        return image;
    }

#ifndef HAS_OPENCV
    Q_UNUSED(contourPoints)
    return image;
#else
    if (contourPoints.trimmed().isEmpty()) {
        return image;
    }

    cv::Mat mat = qImageToMat(image);
    cv::Mat mask = cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);

    const QStringList pairs = contourPoints.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    std::vector<cv::Point> polygon;
    polygon.reserve(pairs.size());
    for (const QString &pair : pairs) {
        const QStringList coords = pair.split(QLatin1Char(','), Qt::SkipEmptyParts);
        if (coords.size() != 2) {
            continue;
        }
        const int x = static_cast<int>(coords.at(0).toDouble() * mat.cols);
        const int y = static_cast<int>(coords.at(1).toDouble() * mat.rows);
        polygon.emplace_back(x, y);
    }

    if (polygon.empty()) {
        return image;
    }

    const std::vector<std::vector<cv::Point>> contours{polygon};
    cv::fillPoly(mask, contours, cv::Scalar(255));
    cv::Mat masked;
    mat.copyTo(masked, mask);
    return matToQImage(masked);
#endif
}

QImage ImageProcessor::applyHideMask(const QImage &image, const QString &contourPoints)
{
    if (image.isNull() || contourPoints.trimmed().isEmpty()) {
        return image;
    }

#ifndef HAS_OPENCV
    Q_UNUSED(contourPoints)
    return image;
#else
    cv::Mat mat = qImageToMat(image);
    cv::Mat mask = cv::Mat::zeros(mat.rows, mat.cols, CV_8UC1);

    const QStringList pairs = contourPoints.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    std::vector<cv::Point> polygon;
    polygon.reserve(pairs.size());
    for (const QString &pair : pairs) {
        const QStringList coords = pair.split(QLatin1Char(','), Qt::SkipEmptyParts);
        if (coords.size() != 2) {
            continue;
        }
        const int x = static_cast<int>(coords.at(0).toDouble() * mat.cols);
        const int y = static_cast<int>(coords.at(1).toDouble() * mat.rows);
        polygon.emplace_back(x, y);
    }

    if (polygon.empty()) {
        return image;
    }

    const std::vector<std::vector<cv::Point>> contours{polygon};
    cv::fillPoly(mask, contours, cv::Scalar(255));

    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGBA2BGR);
    cv::Mat blurred;
    cv::GaussianBlur(bgr, blurred, cv::Size(31, 31), 0);
    blurred.copyTo(bgr, mask);

    cv::Mat rgba;
    cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);
    return matToQImage(rgba);
#endif
}
