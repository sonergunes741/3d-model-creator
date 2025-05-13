#include "LaserLineDetector.h"
#include <iostream>

LaserLineDetector::LaserLineDetector(const cv::Scalar& lowerThresh, const cv::Scalar& upperThresh)
    : lowerThreshold(lowerThresh), 
      upperThreshold(upperThresh), 
      debugMode(false),
      enhanceContrast(true),  // Varsayılan olarak kontrast artırma aktif
      contrastAlpha(1.5),     // Kontrast artırma değeri
      contrastBeta(0),
      roiEnabled(false),      // İlgi bölgesi (ROI) varsayılan olarak devre dışı
      roiX(0), roiY(0), roiWidth(0), roiHeight(0) {
}

std::vector<cv::Point> LaserLineDetector::detectLaserLine(const cv::Mat& image) {
    // İlgi bölgesi (ROI) kullanılıyorsa görüntüyü kırp
    cv::Mat workingImage;
    if (roiEnabled) {
        // ROI parametrelerini görüntü boyutlarına göre ayarla
        int x = roiX < 0 ? 0 : roiX;
        int y = roiY < 0 ? 0 : roiY;
        int width = (roiWidth <= 0 || x + roiWidth > image.cols) ? (image.cols - x) : roiWidth;
        int height = (roiHeight <= 0 || y + roiHeight > image.rows) ? (image.rows - y) : roiHeight;
        
        // ROI'yi kırp
        cv::Rect roi(x, y, width, height);
        workingImage = image(roi).clone();
        
        if (debugMode) {
            cv::Mat debugROI = image.clone();
            cv::rectangle(debugROI, roi, cv::Scalar(0, 255, 0), 2);
            cv::imshow("ROI", debugROI);
            cv::waitKey(1);
        }
    } else {
        workingImage = image.clone();
    }
    
    // Görüntü ön işleme - kontrast artırma
    cv::Mat processedImage;
    if (enhanceContrast) {
        workingImage.convertTo(processedImage, -1, contrastAlpha, contrastBeta);
        
        if (debugMode) {
            cv::imshow("Enhanced Image", processedImage);
            cv::waitKey(1);
        }
    } else {
        processedImage = workingImage.clone();
    }
    
    // HSV formatına dönüştür
    cv::Mat hsvImage;
    cv::cvtColor(processedImage, hsvImage, cv::COLOR_BGR2HSV);

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
        cv::Mat debug = processedImage.clone();
        cv::drawContours(debug, contours, -1, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Laser Contours", debug);
        cv::imshow("Laser Mask", mask);
        cv::waitKey(1);
    }

    // Lazer çizgisini seç
    std::vector<cv::Point> selectedLine = selectLaserLine(contours);
    
    // Eğer ROI kullanıldıysa, koordinatları orijinal görüntüye göre düzelt
    if (roiEnabled && !selectedLine.empty()) {
        for (auto& point : selectedLine) {
            point.x += roiX;
            point.y += roiY;
        }
    }
    
    return selectedLine;
}

std::vector<cv::Point> LaserLineDetector::selectLaserLine(const std::vector<std::vector<cv::Point>>& contours) {
    if (contours.empty()) {
        return std::vector<cv::Point>();
    }

    // YÖNTEM 1: En parlak/en büyük lazer çizgisini bul
    std::vector<std::pair<int, double>> contoursInfo; // index, skor
    
    for (size_t i = 0; i < contours.size(); i++) {
        cv::Rect boundingRect = cv::boundingRect(contours[i]);
        
        // Çok küçük konturları atla
        if (boundingRect.area() < 100) continue;
        
        // Skor hesapla: Genişlik * Yükseklik (alan büyüklüğü)
        double score = boundingRect.width * boundingRect.height;
        
        // Yatay çizgilere daha yüksek skor ver (genişlik > yükseklik)
        if (boundingRect.width > boundingRect.height) {
            score *= 2.0;
        }
        
        contoursInfo.push_back(std::make_pair(i, score));
    }
    
    if (contoursInfo.empty()) {
        return std::vector<cv::Point>();
    }
    
    // Skora göre sırala (büyükten küçüğe)
    std::sort(contoursInfo.begin(), contoursInfo.end(), 
        [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
            return a.second > b.second;
        }
    );
    
    // En yüksek skorlu konturu seç
    int bestIndex = contoursInfo[0].first;
    
    // Debug modunda en iyi konturu göster
    if (debugMode) {
        std::cout << "Seçilen lazer çizgisi, skor: " << contoursInfo[0].second << std::endl;
        if (contoursInfo.size() > 1) {
            std::cout << "İkinci en iyi çizgi skoru: " << contoursInfo[1].second << std::endl;
        }
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

void LaserLineDetector::setROI(int x, int y, int width, int height) {
    roiX = x;
    roiY = y;
    roiWidth = width;
    roiHeight = height;
    roiEnabled = true;
}

void LaserLineDetector::disableROI() {
    roiEnabled = false;
}