#include "ImageProcessor.h"

#ifdef HAS_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#endif

#include <QBuffer>

#include <algorithm>
#include <cmath>

#include <QSet>

namespace {

#ifdef HAS_OPENCV
constexpr int kMaxWorkEdge = 1024;

struct MaskPrecisionProfile {
    int borderDivisor = 6;
    int coreDivisor = 4;
    int grabCutIterations = 10;
    int refineGrabCutIterations = 4;
    int colorPeelPasses = 4;
    int colorPeelThreshold = 20;
    double shrinkScale = 0.02;
    int openKernel = 5;
    int erosionPasses = 1;
    double maskOverlapMin = 0.5;
    double insideSelectionMin = 0.4;
    bool secondGrabCut = true;
    bool looseCapture = false;
};

MaskPrecisionProfile maskPrecisionProfile(int level)
{
    level = std::clamp(level, 1, 10);
    const double t = (level - 1) / 9.0;

    const MaskPrecisionProfile loose = {
        14, 8, 4, 0, 0, 38, 0.0, 3, 0, 0.24, 0.14, false};
    const MaskPrecisionProfile tight = {
        4, 3, 12, 6, 7, 10, 0.05, 5, 2, 0.72, 0.62, true};

    const auto lerpInt = [](int a, int b, double ratio) {
        return static_cast<int>(std::lround(a + (b - a) * ratio));
    };
    const auto lerpD = [](double a, double b, double ratio) {
        return a + (b - a) * ratio;
    };

    MaskPrecisionProfile profile;
    profile.borderDivisor = std::max(3, lerpInt(loose.borderDivisor, tight.borderDivisor, t));
    profile.coreDivisor = std::max(3, lerpInt(loose.coreDivisor, tight.coreDivisor, t));
    profile.grabCutIterations = std::max(3, lerpInt(loose.grabCutIterations, tight.grabCutIterations, t));
    profile.refineGrabCutIterations = level >= 6 ? std::max(1, lerpInt(2, tight.refineGrabCutIterations, (t - 0.55) / 0.45)) : 0;
    profile.colorPeelPasses = level >= 5 ? std::max(0, lerpInt(1, tight.colorPeelPasses, (t - 0.44) / 0.56)) : 0;
    profile.colorPeelThreshold = lerpInt(loose.colorPeelThreshold, tight.colorPeelThreshold, t);
    profile.shrinkScale = lerpD(loose.shrinkScale, tight.shrinkScale, t);
    profile.openKernel = std::clamp(lerpInt(loose.openKernel, tight.openKernel, t), 3, 7);
    profile.erosionPasses = level >= 7 ? std::max(0, lerpInt(0, tight.erosionPasses, (t - 0.66) / 0.34)) : 0;
    profile.maskOverlapMin = lerpD(loose.maskOverlapMin, tight.maskOverlapMin, t);
    profile.insideSelectionMin = lerpD(loose.insideSelectionMin, tight.insideSelectionMin, t);
    profile.secondGrabCut = level >= 6;
    profile.looseCapture = level <= 3;
    return profile;
}

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

cv::Rect relRectToPixelRect(double relX,
                            double relY,
                            double relW,
                            double relH,
                            int imgW,
                            int imgH)
{
    relX = std::clamp(relX, 0.0, 1.0);
    relY = std::clamp(relY, 0.0, 1.0);
    relW = std::clamp(relW, 0.0, 1.0 - relX);
    relH = std::clamp(relH, 0.0, 1.0 - relY);

    const int x = std::clamp(static_cast<int>(relX * imgW), 0, std::max(0, imgW - 1));
    const int y = std::clamp(static_cast<int>(relY * imgH), 0, std::max(0, imgH - 1));
    const int w = std::clamp(static_cast<int>(std::lround(relW * imgW)), 1, imgW - x);
    const int h = std::clamp(static_cast<int>(std::lround(relH * imgH)), 1, imgH - y);
    return {x, y, w, h};
}

cv::Rect clampRectForGrabCut(const cv::Rect &rect, int imgW, int imgH)
{
    cv::Rect bounded = rect & cv::Rect(0, 0, imgW, imgH);
    if (bounded.width < 4 || bounded.height < 4) {
        return {};
    }

    if (bounded.x < 1) {
        bounded.width -= 1 - bounded.x;
        bounded.x = 1;
    }
    if (bounded.y < 1) {
        bounded.height -= 1 - bounded.y;
        bounded.y = 1;
    }
    if (bounded.x + bounded.width >= imgW) {
        bounded.width = imgW - bounded.x - 1;
    }
    if (bounded.y + bounded.height >= imgH) {
        bounded.height = imgH - bounded.y - 1;
    }

    if (bounded.width < 4 || bounded.height < 4) {
        return {};
    }
    return bounded;
}

bool contourTouchesImageBorder(const cv::Rect &box, int imgW, int imgH)
{
    return box.x <= 0 || box.y <= 0 || (box.x + box.width) >= imgW || (box.y + box.height) >= imgH;
}

QString contourStringFromPolygon(const std::vector<cv::Point> &polygon, int cols, int rows)
{
    if (polygon.size() < 3) {
        return {};
    }

    std::vector<cv::Point> simplified;
    cv::approxPolyDP(polygon, simplified, 1.5, true);
    if (simplified.size() < 3) {
        simplified = polygon;
    }

    return contourPointsFromPolygon(simplified, cols, rows);
}

cv::Mat maskForSelection(const cv::Rect &selection, int imgW, int imgH)
{
    cv::Mat mask = cv::Mat::zeros(imgH, imgW, CV_8UC1);
    cv::rectangle(mask, selection, cv::Scalar(255), cv::FILLED);
    return mask;
}

double contourOverlapWithMask(const std::vector<cv::Point> &contour, const cv::Mat &mask)
{
    cv::Mat contourMask = cv::Mat::zeros(mask.rows, mask.cols, CV_8UC1);
    const std::vector<std::vector<cv::Point>> contours{contour};
    cv::fillPoly(contourMask, contours, cv::Scalar(255));

    cv::Mat overlap;
    cv::bitwise_and(contourMask, mask, overlap);
    const int overlapArea = cv::countNonZero(overlap);
    const int contourArea = std::max(1, cv::countNonZero(contourMask));
    return static_cast<double>(overlapArea) / contourArea;
}

cv::Mat shrinkMaskByDistance(const cv::Mat &mask, double minDistance)
{
    if (mask.empty() || minDistance <= 0.0) {
        return mask;
    }

    cv::Mat dist;
    cv::distanceTransform(mask, dist, cv::DIST_L2, 5);
    cv::Mat shrunk;
    cv::threshold(dist, shrunk, minDistance, 255.0, cv::THRESH_BINARY);
    shrunk.convertTo(shrunk, CV_8UC1);
    return shrunk;
}

cv::Mat peelColorMatchedEdges(const cv::Mat &lab, const cv::Mat &mask, int passes, int colorThreshold)
{
    if (lab.empty() || mask.empty() || passes <= 0) {
        return mask;
    }

    cv::Mat current = mask;
    for (int pass = 0; pass < passes; ++pass) {
        cv::Mat next = current.clone();
        bool changed = false;
        for (int y = 1; y < current.rows - 1; ++y) {
            for (int x = 1; x < current.cols - 1; ++x) {
                if (current.at<uchar>(y, x) == 0) {
                    continue;
                }

                int exteriorNeighbors = 0;
                double exteriorDistance = 0.0;
                int interiorNeighbors = 0;
                double interiorDistance = 0.0;
                const cv::Vec3b center = lab.at<cv::Vec3b>(y, x);

                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) {
                            continue;
                        }
                        const int nx = x + dx;
                        const int ny = y + dy;
                        const cv::Vec3b neighbor = lab.at<cv::Vec3b>(ny, nx);
                        const double dist = std::abs(center[0] - neighbor[0])
                                            + std::abs(center[1] - neighbor[1])
                                            + std::abs(center[2] - neighbor[2]);
                        if (current.at<uchar>(ny, nx) == 0) {
                            ++exteriorNeighbors;
                            exteriorDistance += dist;
                        } else {
                            ++interiorNeighbors;
                            interiorDistance += dist;
                        }
                    }
                }

                if (exteriorNeighbors == 0) {
                    continue;
                }

                const double avgExterior = exteriorDistance / exteriorNeighbors;
                const double avgInterior = interiorNeighbors > 0
                                               ? interiorDistance / interiorNeighbors
                                               : 1e9;
                if (avgExterior < colorThreshold && avgExterior + 5.0 < avgInterior) {
                    next.at<uchar>(y, x) = 0;
                    changed = true;
                }
            }
        }
        current = next;
        if (!changed) {
            break;
        }
    }
    return current;
}

cv::Mat refineForegroundMask(const cv::Mat &fgMask,
                             const cv::Rect &selection,
                             const MaskPrecisionProfile &profile,
                             const cv::Mat &lab = cv::Mat())
{
    if (fgMask.empty() || selection.empty()) {
        return fgMask;
    }

    cv::Point center(selection.x + selection.width / 2,
                     selection.y + selection.height / 2);
    center.x = std::clamp(center.x, 0, fgMask.cols - 1);
    center.y = std::clamp(center.y, 0, fgMask.rows - 1);

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int componentCount = cv::connectedComponentsWithStats(fgMask, labels, stats, centroids, 8, CV_32S);

    int keepLabel = labels.at<int>(center.y, center.x);
    if (keepLabel <= 0) {
        double bestOverlap = 0.0;
        const cv::Mat selectionMask = maskForSelection(selection, fgMask.cols, fgMask.rows);
        for (int label = 1; label < componentCount; ++label) {
            cv::Mat componentMask = (labels == label);
            componentMask.convertTo(componentMask, CV_8UC1, 255);
            cv::Mat overlap;
            cv::bitwise_and(componentMask, selectionMask, overlap);
            const double overlapRatio = static_cast<double>(cv::countNonZero(overlap))
                                        / std::max(1, cv::countNonZero(componentMask));
            if (overlapRatio > bestOverlap) {
                bestOverlap = overlapRatio;
                keepLabel = label;
            }
        }
    }

    if (keepLabel <= 0) {
        return fgMask;
    }

    cv::Mat refined = cv::Mat::zeros(fgMask.size(), CV_8UC1);
    refined.setTo(255, labels == keepLabel);

    const cv::Mat kernelSmall = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    if (!profile.looseCapture) {
        const int openSize = std::clamp(profile.openKernel, 3, 7);
        const cv::Mat kernelOpen = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(openSize, openSize));
        if (openSize > 0) {
            cv::morphologyEx(refined, refined, cv::MORPH_OPEN, kernelOpen);
        }

        if (!lab.empty() && profile.colorPeelPasses > 0) {
            refined = peelColorMatchedEdges(lab, refined, profile.colorPeelPasses, profile.colorPeelThreshold);
        }

        const double minDist = std::clamp(std::min(selection.width, selection.height) * profile.shrinkScale,
                                          0.0,
                                          8.0);
        if (minDist > 0.0) {
            refined = shrinkMaskByDistance(refined, minDist);
        }

        cv::morphologyEx(refined, refined, cv::MORPH_OPEN, kernelSmall);
        if (profile.erosionPasses > 0) {
            cv::erode(refined, refined, kernelSmall, cv::Point(-1, -1), profile.erosionPasses);
        }
    }

    if (refined.at<uchar>(center.y, center.x) == 0) {
        cv::dilate(refined, refined, kernelSmall);
    }

    return refined;
}

bool pickBestForegroundContour(const std::vector<std::vector<cv::Point>> &contours,
                               const cv::Rect &selection,
                               int imgW,
                               int imgH,
                               const MaskPrecisionProfile &profile,
                               std::vector<cv::Point> *bestContour)
{
    if (!bestContour) {
        return false;
    }

    const cv::Point selectionCenter(selection.x + selection.width / 2,
                                    selection.y + selection.height / 2);
    const double imgArea = static_cast<double>(imgW * imgH);
    const double selectionArea = static_cast<double>(std::max(1, selection.area()));
    const cv::Mat selectionMask = maskForSelection(selection, imgW, imgH);

    double bestScore = -1e9;
    bool found = false;

    for (const std::vector<cv::Point> &contour : contours) {
        if (contour.size() < 3) {
            continue;
        }

        const double area = cv::contourArea(contour);
        if (area < 40.0 || area > imgArea * 0.7) {
            continue;
        }

        if (cv::pointPolygonTest(contour, selectionCenter, false) < 0) {
            continue;
        }

        const cv::Rect box = cv::boundingRect(contour);
        const cv::Rect inter = box & selection;
        if (inter.area() <= 0) {
            continue;
        }

        const double insideSelection = static_cast<double>(inter.area())
                                       / std::max(1.0, static_cast<double>(box.area()));
        const double maskOverlap = contourOverlapWithMask(contour, selectionMask);
        const double selectionFill = area / selectionArea;
        const double spillOutside = std::max(0.0, 1.0 - maskOverlap);
        const bool touchesBorder = contourTouchesImageBorder(box, imgW, imgH);
        if (touchesBorder && area > imgArea * 0.25) {
            continue;
        }
        if (maskOverlap < profile.maskOverlapMin || insideSelection < profile.insideSelectionMin) {
            continue;
        }

        const double score = maskOverlap * 0.5 + insideSelection * 0.32 + selectionFill * 0.18
                             - spillOutside * 0.65 - (area / imgArea) * 0.35
                             - (touchesBorder ? 0.12 : 0.0);
        if (score > bestScore) {
            bestScore = score;
            *bestContour = contour;
            found = true;
        }
    }

    return found;
}

QString extractContourWithGrabCut(const cv::Mat &bgr,
                                  const cv::Rect &selection,
                                  const MaskPrecisionProfile &profile)
{
    const int imgW = bgr.cols;
    const int imgH = bgr.rows;
    const cv::Rect grabRect = clampRectForGrabCut(selection, imgW, imgH);
    if (grabRect.empty()) {
        return {};
    }

    cv::Mat lab;
    cv::cvtColor(bgr, lab, cv::COLOR_BGR2Lab);

    cv::Mat gcMask(imgH, imgW, CV_8UC1, cv::Scalar(cv::GC_BGD));
    cv::Mat bgdModel;
    cv::Mat fgdModel;

    if (profile.looseCapture) {
        cv::grabCut(bgr, gcMask, grabRect, bgdModel, fgdModel, profile.grabCutIterations, cv::GC_INIT_WITH_RECT);
    } else {
        cv::rectangle(gcMask, grabRect, cv::Scalar(cv::GC_PR_FGD), cv::FILLED);

        const int borderX = std::max(3, grabRect.width / profile.borderDivisor);
        const int borderY = std::max(3, grabRect.height / profile.borderDivisor);
        cv::Rect inner(grabRect.x + borderX,
                       grabRect.y + borderY,
                       grabRect.width - borderX * 2,
                       grabRect.height - borderY * 2);
        if (inner.width > 6 && inner.height > 6) {
            cv::Mat borderBand = cv::Mat::zeros(imgH, imgW, CV_8UC1);
            cv::rectangle(borderBand, grabRect, cv::Scalar(255), cv::FILLED);
            cv::rectangle(borderBand, inner, cv::Scalar(0), cv::FILLED);
            gcMask.setTo(cv::GC_BGD, borderBand);

            const int coreW = std::max(4, inner.width / profile.coreDivisor);
            const int coreH = std::max(4, inner.height / profile.coreDivisor);
            const cv::Rect core(inner.x + (inner.width - coreW) / 2,
                                inner.y + (inner.height - coreH) / 2,
                                coreW,
                                coreH);
            cv::rectangle(gcMask, core, cv::Scalar(cv::GC_FGD), cv::FILLED);
        }

        cv::grabCut(bgr, gcMask, grabRect, bgdModel, fgdModel, profile.grabCutIterations, cv::GC_INIT_WITH_MASK);
    }

    cv::Mat fgMask = (gcMask == cv::GC_FGD) | (gcMask == cv::GC_PR_FGD);
    fgMask.convertTo(fgMask, CV_8UC1, 255);

    const cv::Mat selectionMask = maskForSelection(selection, imgW, imgH);
    cv::bitwise_and(fgMask, selectionMask, fgMask);
    fgMask = refineForegroundMask(fgMask, selection, profile, lab);

    if (profile.secondGrabCut && profile.refineGrabCutIterations > 0) {
        const cv::Mat kernelSmall = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::Mat gcRefined(imgH, imgW, CV_8UC1, cv::Scalar(cv::GC_BGD));
        gcRefined.setTo(cv::GC_PR_FGD, fgMask > 0);
        cv::Mat definiteFg;
        cv::erode(fgMask, definiteFg, kernelSmall, cv::Point(-1, -1), 2);
        gcRefined.setTo(cv::GC_FGD, definiteFg > 0);
        cv::grabCut(bgr, gcRefined, grabRect, bgdModel, fgdModel, profile.refineGrabCutIterations, cv::GC_EVAL);

        fgMask = (gcRefined == cv::GC_FGD) | (gcRefined == cv::GC_PR_FGD);
        fgMask.convertTo(fgMask, CV_8UC1, 255);
        cv::bitwise_and(fgMask, selectionMask, fgMask);
        fgMask = refineForegroundMask(fgMask, selection, profile, lab);
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Point> bestContour;
    if (!pickBestForegroundContour(contours, selection, imgW, imgH, profile, &bestContour)) {
        return {};
    }

    return contourStringFromPolygon(bestContour, imgW, imgH);
}

std::vector<cv::Point> contourFromMask(const cv::Mat &mask);
void drawDashedPolygon(cv::Mat &bgr,
                       const std::vector<cv::Point> &polygon,
                       const cv::Scalar &color,
                       int thickness);

void drawHideRegion(cv::Mat &bgr,
                    const cv::Mat &mask,
                    int labelNumber,
                    int cols,
                    int rows)
{
    const cv::Scalar light(232, 237, 242);
    const cv::Scalar dark(26, 34, 48);
    constexpr int tile = 10;

    for (int y = 0; y < bgr.rows; ++y) {
        for (int x = 0; x < bgr.cols; ++x) {
            if (mask.at<uchar>(y, x) == 0) {
                continue;
            }
            const cv::Scalar fill = ((x / tile) + (y / tile)) % 2 == 0 ? light : dark;
            bgr.at<cv::Vec3b>(y, x) = cv::Vec3b(
                static_cast<uchar>(fill[0]),
                static_cast<uchar>(fill[1]),
                static_cast<uchar>(fill[2]));
        }
    }

    const std::vector<cv::Point> polygon = contourFromMask(mask);
    if (!polygon.empty()) {
        drawDashedPolygon(bgr, polygon, cv::Scalar(255, 255, 255), 2);
        drawDashedPolygon(bgr, polygon, cv::Scalar(0, 0, 0), 1);

        const cv::Moments moments = cv::moments(mask, true);
        if (moments.m00 > 0.0) {
            const int cx = static_cast<int>(moments.m10 / moments.m00);
            const int cy = static_cast<int>(moments.m01 / moments.m00);
            const std::string mark = labelNumber > 0 ? std::to_string(labelNumber) : "?";
            const double fontScale = std::clamp(std::min(cols, rows) / 180.0, 0.8, 2.4);
            const cv::Size textSize = cv::getTextSize(mark, cv::FONT_HERSHEY_DUPLEX, fontScale, 2, nullptr);
            const cv::Point origin(cx - textSize.width / 2, cy + textSize.height / 2);
            cv::putText(bgr, mark, origin, cv::FONT_HERSHEY_DUPLEX, fontScale, cv::Scalar(0, 0, 0), 4, cv::LINE_AA);
            cv::putText(bgr, mark, origin, cv::FONT_HERSHEY_DUPLEX, fontScale, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        }
    }
}

std::vector<cv::Point> contourFromMask(const cv::Mat &mask)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
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
    return contours[bestIdx];
}

void drawDashedPolygon(cv::Mat &bgr, const std::vector<cv::Point> &polygon, const cv::Scalar &color, int thickness)
{
    if (polygon.size() < 2) {
        return;
    }

    constexpr int dashLen = 7;
    constexpr int gapLen = 5;

    const size_t count = polygon.size();
    for (size_t i = 0; i < count; ++i) {
        const cv::Point p0 = polygon[i];
        const cv::Point p1 = polygon[(i + 1) % count];
        const double dx = static_cast<double>(p1.x - p0.x);
        const double dy = static_cast<double>(p1.y - p0.y);
        const double length = std::hypot(dx, dy);
        if (length < 1.0) {
            continue;
        }

        const double ux = dx / length;
        const double uy = dy / length;
        double traveled = 0.0;
        bool draw = true;
        while (traveled < length) {
            const int segment = draw ? dashLen : gapLen;
            const double next = std::min(length, traveled + segment);
            if (draw) {
                const cv::Point a(static_cast<int>(p0.x + ux * traveled), static_cast<int>(p0.y + uy * traveled));
                const cv::Point b(static_cast<int>(p0.x + ux * next), static_cast<int>(p0.y + uy * next));
                cv::line(bgr, a, b, color, thickness, cv::LINE_AA);
            }
            traveled = next;
            draw = !draw;
        }
    }
}
#endif

} // namespace

ImageProcessor::ImageProcessor(QObject *parent)
    : QObject(parent)
{
}

QString ImageProcessor::extractContour(const QImage &image,
                                       double relX,
                                       double relY,
                                       int tolerance,
                                       int precision)
{
    if (image.isNull()) {
        emit processingFailed(QStringLiteral("Image is empty"));
        return {};
    }

#ifndef HAS_OPENCV
    Q_UNUSED(relX)
    Q_UNUSED(relY)
    Q_UNUSED(tolerance)
    Q_UNUSED(precision)
    emit processingFailed(QStringLiteral("OpenCV is not available"));
    return {};
#else
    const MaskPrecisionProfile profile = maskPrecisionProfile(precision);
    const double toleranceScale = 1.0 + (5.5 - std::clamp(precision, 1, 10)) * 0.14;
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
    const int tolL = std::clamp(static_cast<int>((stddev[0] * 2.4 + 6) * toleranceScale), 8, 48);
    const int tolAB = std::clamp(static_cast<int>((std::max(stddev[1], stddev[2]) * 1.9 + 5) * toleranceScale), 6, 36);
    const cv::Scalar loDiff(tolL, tolAB, tolAB);
    const cv::Scalar upDiff(tolL, tolAB, tolAB);

    cv::Mat bestMask;
    int bestArea = 0;

    const int tolerances[] = {0,
                              static_cast<int>(4 * toleranceScale),
                              static_cast<int>(8 * toleranceScale)};
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
        const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(regionMask, regionMask, cv::MORPH_OPEN, kernel);

        const cv::Rect seedBox(std::max(0, pixelX - patchRadius),
                               std::max(0, pixelY - patchRadius),
                               std::min(bgr.cols, patchRadius * 2 + 1),
                               std::min(bgr.rows, patchRadius * 2 + 1));
        regionMask = refineForegroundMask(regionMask, seedBox, profile, lab);

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
            const cv::Rect box = cv::boundingRect(contour);
            if (contourTouchesImageBorder(box, bgr.cols, bgr.rows) && area > bgr.cols * bgr.rows * 0.25) {
                continue;
            }
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
        const cv::Rect box = cv::boundingRect(finalContours[i]);
        if (contourTouchesImageBorder(box, bgr.cols, bgr.rows) && area > bgr.cols * bgr.rows * 0.25) {
            continue;
        }
        if (area > largest) {
            largest = area;
            bestIdx = i;
        }
    }

    if (largest <= 0.0) {
        emit processingFailed(QStringLiteral("No contour found"));
        return {};
    }

    std::vector<cv::Point> simplified;
    cv::approxPolyDP(finalContours[bestIdx], simplified, 1.5, true);
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
                                             double relH,
                                             int precision)
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
    Q_UNUSED(precision)
    emit processingFailed(QStringLiteral("OpenCV is not available"));
    return {};
#else
    const MaskPrecisionProfile profile = maskPrecisionProfile(precision);
    const QImage workImage = downscaleWorkImage(image);
    cv::Mat mat = qImageToMat(workImage);
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGBA2BGR);

    const int imgW = bgr.cols;
    const int imgH = bgr.rows;
    const cv::Rect selection = relRectToPixelRect(relX, relY, relW, relH, imgW, imgH);

    QString contour;
    if (selection.width >= 4 && selection.height >= 4) {
        contour = extractContourWithGrabCut(bgr, selection, profile);
    }

    if (contour.isEmpty()) {
        const double cx = relX + relW * 0.5;
        const double cy = relY + relH * 0.5;
        contour = extractContour(image, cx, cy, 24, precision);
    }

    if (contour.isEmpty()) {
        emit processingFailed(QStringLiteral("No contour found"));
        return {};
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

    MaskLayer layer;
    layer.number = 1;
    layer.contour = contourPoints;
    return applyHideMasks(image, {layer});
}

QImage ImageProcessor::applyHideMasks(const QImage &image,
                                      const QVector<MaskLayer> &masks,
                                      const QVector<int> &revealedMaskNumbers)
{
    if (image.isNull() || masks.isEmpty()) {
        return image;
    }

#ifndef HAS_OPENCV
    Q_UNUSED(masks)
    Q_UNUSED(revealedMaskNumbers)
    return image;
#else
    QSet<int> revealed;
    for (int number : revealedMaskNumbers) {
        revealed.insert(number);
    }

    const QSize originalSize = image.size();
    const QImage workImage = downscaleWorkImage(image);
    cv::Mat mat = qImageToMat(workImage);
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGBA2BGR);

    for (const MaskLayer &layer : masks) {
        if (layer.contour.trimmed().isEmpty()) {
            continue;
        }
        if (revealed.contains(layer.number)) {
            continue;
        }

        cv::Mat mask = buildFilledMask(layer.contour, bgr.cols, bgr.rows);
        if (cv::countNonZero(mask) == 0) {
            continue;
        }
        drawHideRegion(bgr, mask, layer.number, bgr.cols, bgr.rows);
    }

    cv::Mat rgba;
    cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);
    QImage result = matToQImage(rgba);
    if (workImage.size() != originalSize) {
        result = result.scaled(originalSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return result;
#endif
}
