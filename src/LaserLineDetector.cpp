#include "LaserLineDetector.h"
#include <iostream>

LaserLineDetector::LaserLineDetector(const cv::Scalar& lowerThresh, const cv::Scalar& upperThresh)
    : lowerThreshold(lowerThresh), upperThreshold(upperThresh), debugMode(false) {
}

std::vector<cv::Point> LaserLineDetector::detectLaserLine(const cv::Mat& image) {
    // HSV formatına dönüştür
    cv::Mat hsvImage;
    cv::cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);

    // Kırmızı renk aralığında eşikleme yap
    cv::Mat mask;
    cv::inRange(hsvImage, lowerThreshold, upperThreshold, mask);

    // Morfolojik işlemlerle gürültü temizle
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    // Konturları bul
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Debug modunda görselleştir
    if (debugMode) {
        cv::Mat debug = image.clone();
        cv::drawContours(debug, contours, -1, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Laser Contours", debug);
        cv::imshow("Laser Mask", mask);
        cv::waitKey(1);
    }

    // Lazer çizgisini seç
    return selectLaserLine(contours);
}

std::vector<cv::Point> LaserLineDetector::selectLaserLine(const std::vector<std::vector<cv::Point>>& contours) {
    if (contours.empty()) {
        return std::vector<cv::Point>();
    }

    // En uzun yatay kontur olması muhtemel lazer çizgisidir
    int bestIndex = -1;
    double maxWidth = 0;

    for (size_t i = 0; i < contours.size(); i++) {
        cv::Rect boundingRect = cv::boundingRect(contours[i]);
        double width = boundingRect.width;
        
        // Minimum alan kontrolü - çok küçük konturları filtrele
        if (boundingRect.area() < 100) continue;

        if (width > maxWidth) {
            maxWidth = width;
            bestIndex = i;
        }
    }

    if (bestIndex == -1) {
        return std::vector<cv::Point>();
    }

    // Kontur noktalarını düzenle - x koordinatlarına göre sırala
    std::vector<cv::Point> laserLine = contours[bestIndex];
    std::sort(laserLine.begin(), laserLine.end(), 
        [](const cv::Point& a, const cv::Point& b) {
            return a.x < b.x;
        }
    );

    // Düzgün nokta dağılımı oluştur
    std::vector<cv::Point> smoothedLine;
    const int numPoints = 100; // İstenilen nokta sayısı
    
    if (laserLine.size() > 1) {
        double minX = laserLine.front().x;
        double maxX = laserLine.back().x;
        double step = (maxX - minX) / (numPoints - 1);

        for (int i = 0; i < numPoints; i++) {
            double currentX = minX + i * step;
            
            // En yakın y değerini bul
            int nearestIdx = 0;
            double minDist = std::abs(laserLine[0].x - currentX);
            
            for (size_t j = 1; j < laserLine.size(); j++) {
                double dist = std::abs(laserLine[j].x - currentX);
                if (dist < minDist) {
                    minDist = dist;
                    nearestIdx = j;
                }
            }
            
            smoothedLine.push_back(cv::Point(currentX, laserLine[nearestIdx].y));
        }
    }

    return smoothedLine.empty() ? laserLine : smoothedLine;
}

void LaserLineDetector::setThresholds(const cv::Scalar& lowerThresh, const cv::Scalar& upperThresh) {
    lowerThreshold = lowerThresh;
    upperThreshold = upperThresh;
}