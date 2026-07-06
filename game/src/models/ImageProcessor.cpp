#include "ImageProcessor.h"

#ifdef HAS_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#endif

#include <QBuffer>

#include <algorithm>

namespace {

#ifdef HAS_OPENCV
constexpr int kMaxWorkEdge = 1024;

QImage downscaleWorkImage(const QImage &image)
{
    const int maxDim = std::max(image.width(), image.height());
    if (maxDim <= kMaxWorkEdge) {
        return image;
    }

    const double scale = static_cast<double>(kMaxWorkEdge) / maxDim;
    return image.scaled(static_cast<int>(image.width() * scale),
                        static_cast<int>(image.height() * scale),
                        Qt::IgnoreAspectRatio,
                        Qt::FastTransformation);
}

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

std::vector<cv::Point> contourStringToPolygon(const QString &contourPoints, int cols, int rows)
{
    std::vector<cv::Point> polygon;
    const QStringList pairs = contourPoints.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    polygon.reserve(static_cast<size_t>(pairs.size()));
    for (const QString &pair : pairs) {
        const QStringList coords = pair.split(QLatin1Char(','), Qt::SkipEmptyParts);
        if (coords.size() != 2) {
            continue;
        }
        const int x = static_cast<int>(coords.at(0).toDouble() * cols);
        const int y = static_cast<int>(coords.at(1).toDouble() * rows);
        polygon.emplace_back(x, y);
    }
    return polygon;
}

cv::Mat buildFilledMask(const QString &contourPoints, int cols, int rows)
{
    cv::Mat mask = cv::Mat::zeros(rows, cols, CV_8UC1);
    const std::vector<cv::Point> polygon = contourStringToPolygon(contourPoints, cols, rows);
    if (polygon.empty()) {
        return mask;
    }
    const std::vector<std::vector<cv::Point>> contours{polygon};
    cv::fillPoly(mask, contours, cv::Scalar(255));
    return mask;
}

QString contourPointsFromPolygon(const std::vector<cv::Point> &polygon, int cols, int rows)
{
    if (polygon.size() < 3) {
        return {};
    }

    QStringList points;
    points.reserve(static_cast<int>(polygon.size()));
    for (const cv::Point &point : polygon) {
        const double rx = static_cast<double>(point.x) / cols;
        const double ry = static_cast<double>(point.y) / rows;
        points.append(QStringLiteral("%1,%2").arg(rx, 0, 'f', 4).arg(ry, 0, 'f', 4));
    }
    return points.join(QLatin1Char(';'));
}

QString largestContourString(const cv::Mat &binaryMask, int cols, int rows)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binaryMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) {
        return {};
    }

    size_t bestIdx = 0;
    double largest = 0.0;
    for (size_t i = 0; i < contours.size(); ++i) {
        const double area = cv::contourArea(contours[i]);
        if (area > largest) {
            largest = area;
            bestIdx = i;
        }
    }

    std::vector<cv::Point> simplified;
    cv::approxPolyDP(contours[bestIdx], simplified, 2.0, true);
    if (simplified.size() < 3) {
        simplified = contours[bestIdx];
    }

    return contourPointsFromPolygon(simplified, cols, rows);
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
    const QImage workImage = downscaleWorkImage(image);
    cv::Mat mat = qImageToMat(workImage);
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGBA2BGR);
    cv::Mat lab;
    cv::cvtColor(bgr, lab, cv::COLOR_BGR2Lab);

    const int pixelX = std::clamp(static_cast<int>(relX * bgr.cols), 0, bgr.cols - 1);
    const int pixelY = std::clamp(static_cast<int>(relY * bgr.rows), 0, bgr.rows - 1);
    Q_UNUSED(tolerance);
    const cv::Point seed(pixelX, pixelY);

    const int patchRadius = 12;
    const int x0 = std::max(0, pixelX - patchRadius);
    const int y0 = std::max(0, pixelY - patchRadius);
    const int x1 = std::min(bgr.cols, pixelX + patchRadius + 1);
    const int y1 = std::min(bgr.rows, pixelY + patchRadius + 1);
    const cv::Mat patch = lab(cv::Rect(x0, y0, x1 - x0, y1 - y0));

    cv::Scalar mean;
    cv::Scalar stddev;
    cv::meanStdDev(patch, mean, stddev);
    const int tolL = std::clamp(static_cast<int>(stddev[0] * 2.8) + 8, 12, 48);
    const int tolAB = std::clamp(static_cast<int>(std::max(stddev[1], stddev[2]) * 2.2) + 6, 10, 36);
    const cv::Scalar loDiff(tolL, tolAB, tolAB);
    const cv::Scalar upDiff(tolL, tolAB, tolAB);

    cv::Mat bestMask;
    int bestArea = 0;

    const int tolerances[] = {0, 6, 12};
    for (int attempt = 0; attempt < 3; ++attempt) {
        const int extra = tolerances[attempt];
        cv::Mat floodMask = cv::Mat::zeros(bgr.rows + 2, bgr.cols + 2, CV_8UC1);
        cv::Mat labCopy = lab.clone();
        const cv::Scalar attemptLo(loDiff[0] + extra, loDiff[1] + extra, loDiff[2] + extra);
        const cv::Scalar attemptUp(upDiff[0] + extra, upDiff[1] + extra, upDiff[2] + extra);
        cv::floodFill(labCopy,
                      floodMask,
                      seed,
                      cv::Scalar(0, 0, 0),
                      nullptr,
                      attemptLo,
                      attemptUp,
                      4 | cv::FLOODFILL_MASK_ONLY | (255 << 8));

        cv::Mat regionMask = floodMask(cv::Rect(1, 1, bgr.cols, bgr.rows));
        const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::morphologyEx(regionMask, regionMask, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(regionMask, regionMask, cv::MORPH_OPEN, kernel);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(regionMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        for (const std::vector<cv::Point> &contour : contours) {
            if (contour.size() < 3) {
                continue;
            }
            if (cv::pointPolygonTest(contour, seed, false) < 0) {
                continue;
            }
            const int area = static_cast<int>(cv::contourArea(contour));
            if (area > bestArea) {
                bestArea = area;
                bestMask = regionMask.clone();
            }
        }

        if (bestArea > 0) {
            break;
        }
    }

    if (bestMask.empty() || bestArea <= 0) {
        emit processingFailed(QStringLiteral("No contour found"));
        return {};
    }

    std::vector<std::vector<cv::Point>> finalContours;
    cv::findContours(bestMask, finalContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (finalContours.empty()) {
        emit processingFailed(QStringLiteral("No contour found"));
        return {};
    }

    size_t bestIdx = 0;
    double largest = 0.0;
    for (size_t i = 0; i < finalContours.size(); ++i) {
        const double area = cv::contourArea(finalContours[i]);
        if (area > largest) {
            largest = area;
            bestIdx = i;
        }
    }

    std::vector<cv::Point> simplified;
    cv::approxPolyDP(finalContours[bestIdx], simplified, 2.0, true);
    if (simplified.size() < 3) {
        simplified = finalContours[bestIdx];
    }

    const QString contour = contourPointsFromPolygon(simplified, bgr.cols, bgr.rows);
    if (contour.isEmpty()) {
        emit processingFailed(QStringLiteral("No contour found"));
        return {};
    }

    emit contourExtracted(contour);
    return contour;
#endif
}

QString ImageProcessor::extractContourInRect(const QImage &image,
                                             double relX,
                                             double relY,
                                             double relW,
                                             double relH)
{
    if (image.isNull()) {
        emit processingFailed(QStringLiteral("Image is empty"));
        return {};
    }

#ifndef HAS_OPENCV
    Q_UNUSED(relX)
    Q_UNUSED(relY)
    Q_UNUSED(relW)
    Q_UNUSED(relH)
    emit processingFailed(QStringLiteral("OpenCV is not available"));
    return {};
#else
    relW = std::max(relW, 0.0);
    relH = std::max(relH, 0.0);

    if (relW < 0.015 && relH < 0.015) {
        return extractContour(image, relX + relW * 0.5, relY + relH * 0.5, 24);
    }

    const QImage workImage = downscaleWorkImage(image);
    cv::Mat mat = qImageToMat(workImage);
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGBA2BGR);

    const int cols = bgr.cols;
    const int rows = bgr.rows;

    int x = std::clamp(static_cast<int>(relX * cols), 0, cols - 1);
    int y = std::clamp(static_cast<int>(relY * rows), 0, rows - 1);
    int w = std::clamp(static_cast<int>(relW * cols), 1, cols - x);
    int h = std::clamp(static_cast<int>(relH * rows), 1, rows - y);

    if (w < 6 || h < 6) {
        return extractContour(image, relX + relW * 0.5, relY + relH * 0.5, 24);
    }

    cv::Rect roi(x, y, w, h);
    cv::Mat grabcutMask(rows, cols, CV_8UC1, cv::Scalar(cv::GC_BGD));
    cv::Mat bgdModel;
    cv::Mat fgdModel;
    cv::grabCut(bgr, grabcutMask, roi, bgdModel, fgdModel, 2, cv::GC_INIT_WITH_RECT);

    cv::Mat fgMask = (grabcutMask == cv::GC_FGD) | (grabcutMask == cv::GC_PR_FGD);
    fgMask.convertTo(fgMask, CV_8UC1, 255);

    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_CLOSE, kernel);
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_OPEN, kernel);

    QString contour = largestContourString(fgMask, cols, rows);

    if (contour.isEmpty()) {
        const double cx = relX + relW * 0.5;
        const double cy = relY + relH * 0.5;
        contour = extractContour(image, cx, cy, 24);
        return contour;
    }

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
    cv::Mat mask = buildFilledMask(contourPoints, mat.cols, mat.rows);
    if (cv::countNonZero(mask) == 0) {
        return image;
    }

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
    const QSize originalSize = image.size();
    const QImage workImage = downscaleWorkImage(image);
    cv::Mat mat = qImageToMat(workImage);
    cv::Mat mask = buildFilledMask(contourPoints, mat.cols, mat.rows);
    if (cv::countNonZero(mask) == 0) {
        return image;
    }

    const cv::Mat dilateKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::dilate(mask, mask, dilateKernel);

    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGBA2BGR);

    cv::Mat blurred;
    cv::GaussianBlur(bgr, blurred, cv::Size(0, 0), 16.0);
    blurred.copyTo(bgr, mask);

    cv::Mat rgba;
    cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);
    QImage result = matToQImage(rgba);
    if (workImage.size() != originalSize) {
        result = result.scaled(originalSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return result;
#endif
}
