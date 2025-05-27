#include "LaserLineDetector.h"
#include <iostream>

LaserLineDetector::LaserLineDetector(const cv::Scalar& lowerThresh, const cv::Scalar& upperThresh)
    : lowerThreshold(lowerThresh), 
      upperThreshold(upperThresh), 
      debugMode(false),
      enhanceContrast(true),
      contrastAlpha(1.5),
      contrastBeta(0),
      roiEnabled(false),
      roiX(0), roiY(0), roiWidth(0), roiHeight(0),
      binaryThreshold(128),
      useGaussianBlur(true),
      gaussianKernelSize(5),
      gaussianSigma(1.5),
      useMedianBlur(true),
      medianKernelSize(5),
      useErosion(true),
      erosionIterations(1),
      erosionKernelSize(3),
      useDilation(true),
      dilationIterations(1),
      dilationKernelSize(3) {
}

// Helper function to interpolate between points to fill gaps
std::vector<cv::Point> interpolatePoints(const std::vector<cv::Point>& points, int maxGapSize = 50) {
    if (points.size() < 2) return points;
    
    std::vector<cv::Point> interpolated;
    interpolated.reserve(points.size() * 2); // Reserve space for interpolated points
    
    // Add first point
    interpolated.push_back(points[0]);
    
    // Interpolate between consecutive points
    for (size_t i = 1; i < points.size(); i++) {
        const cv::Point& prev = points[i-1];
        const cv::Point& curr = points[i];
        
        // If gap is too large, interpolate
        int yGap = curr.y - prev.y;
        if (yGap > 1 && yGap <= maxGapSize) {
            // Linear interpolation
            for (int y = prev.y + 1; y < curr.y; y++) {
                float t = static_cast<float>(y - prev.y) / yGap;
                int x = static_cast<int>(prev.x + t * (curr.x - prev.x));
                interpolated.push_back(cv::Point(x, y));
            }
        }
        
        interpolated.push_back(curr);
    }
    
    return interpolated;
}

// Helper function to fit a quadratic curve to points
std::vector<float> fitQuadraticCurve(const std::vector<cv::Point>& points) {
    if (points.size() < 3) return {0, 0, 0}; // Not enough points for quadratic fit
    
    // Normalize y coordinates to avoid numerical issues
    float yMin = points.front().y;
    float yMax = points.back().y;
    float yRange = yMax - yMin;
    
    // Build the system of equations
    cv::Mat A(points.size(), 3, CV_32F);
    cv::Mat b(points.size(), 1, CV_32F);
    
    for (size_t i = 0; i < points.size(); i++) {
        float y = (points[i].y - yMin) / yRange; // Normalized y
        A.at<float>(i, 0) = y * y;
        A.at<float>(i, 1) = y;
        A.at<float>(i, 2) = 1;
        b.at<float>(i, 0) = points[i].x;
    }
    
    // Solve the system using least squares
    cv::Mat x;
    cv::solve(A, b, x, cv::DECOMP_SVD);
    
    // Convert coefficients back to original scale
    std::vector<float> coeffs(3);
    coeffs[0] = x.at<float>(0, 0) / (yRange * yRange);
    coeffs[1] = x.at<float>(1, 0) / yRange - 2 * yMin * coeffs[0];
    coeffs[2] = x.at<float>(2, 0) - yMin * yMin * coeffs[0] - yMin * coeffs[1];
    
    return coeffs;
}

// Helper function to evaluate quadratic curve
float evaluateQuadratic(const std::vector<float>& coeffs, float y) {
    return coeffs[0] * y * y + coeffs[1] * y + coeffs[2];
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
    } else {
        workingImage = image.clone();
    }
    
    // Debug modunda görselleştirme için sonuç görüntüsünü hazırla
    cv::Mat debugResult;
    if (debugMode) {
        debugResult = image.clone();
        if (roiEnabled) {
            cv::rectangle(debugResult, cv::Rect(roiX, roiY, roiWidth, roiHeight), cv::Scalar(0, 255, 0), 2);
        }
    }
    
    // Görüntü ön işleme - kontrast artırma
    cv::Mat processedImage;
    if (enhanceContrast) {
        workingImage.convertTo(processedImage, -1, contrastAlpha, contrastBeta);
    } else {
        processedImage = workingImage.clone();
    }
    
    // Scanner approach: Find maximum red value per row (more robust than HSV thresholding)
    std::vector<cv::Point> laserPoints;
    int h = processedImage.rows;
    int w = processedImage.cols;
    
    // Create a binary mask for detected laser points
    cv::Mat laserMask = cv::Mat::zeros(h, w, CV_8U);
    
    // For each row, find the pixel with maximum red value
    for(int i = 0; i < h; i++) {
        int maxRed = -1;
        int maxRedIndex = -1;
        
        for(int j = 0; j < w; j++) {
            // Get red channel value (BGR format, so red is index 2)
            int redValue = processedImage.at<cv::Vec3b>(i, j)[2];
            if(redValue > maxRed) {
                maxRedIndex = j;
                maxRed = redValue;
            }
        }
        
        // If the maximum red value is above threshold, mark it as laser point
        if(maxRed > 25 && maxRedIndex >= 0) {  // Threshold of 25 (adjustable)
            laserMask.at<uchar>(i, maxRedIndex) = 255;
            laserPoints.push_back(cv::Point(maxRedIndex, i));
        }
    }
    
    // Apply additional filtering if needed
    if (useMedianBlur && medianKernelSize > 0) {
        cv::medianBlur(laserMask, laserMask, medianKernelSize);
    }
    
    if (useGaussianBlur && gaussianKernelSize > 0) {
        cv::GaussianBlur(laserMask, laserMask, 
                        cv::Size(gaussianKernelSize, gaussianKernelSize), 
                        gaussianSigma);
    }
    
    // Morfolojik işlemler - Erosion
    if (useErosion) {
        cv::Mat erosionKernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE, 
            cv::Size(erosionKernelSize, erosionKernelSize)
        );
        cv::erode(laserMask, laserMask, erosionKernel, cv::Point(-1, -1), erosionIterations);
    }
    
    // Morfolojik işlemler - Dilation
    if (useDilation) {
        cv::Mat dilationKernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE, 
            cv::Size(dilationKernelSize, dilationKernelSize)
        );
        cv::dilate(laserMask, laserMask, dilationKernel, cv::Point(-1, -1), dilationIterations);
    }
    
    // Re-extract points from processed mask
    laserPoints.clear();
    for(int y = 0; y < laserMask.rows; y++) {
        for(int x = 0; x < laserMask.cols; x++) {
            if(laserMask.at<uchar>(y, x) > 0) {
                laserPoints.push_back(cv::Point(x, y));
            }
        }
    }
    
    // Sort points by y-coordinate for better interpolation
    std::sort(laserPoints.begin(), laserPoints.end(), 
              [](const cv::Point& a, const cv::Point& b) { return a.y < b.y; });
    
    // Fill gaps using interpolation (from original code)
    laserPoints = interpolatePoints(laserPoints, 20);
    
    // If very few points found, create artificial line (fallback)
    if (laserPoints.size() < h * 0.1) {  // Less than 10% of rows
        std::cout << "UYARI: Çok az lazer noktası tespit edildi! Yapay nokta oluşturuluyor." << std::endl;
        
        // Calculate average x position from found points
        int avgX = w / 2;  // Default to center
        if (!laserPoints.empty()) {
            int sumX = 0;
            for (const auto& p : laserPoints) {
                sumX += p.x;
            }
            avgX = sumX / laserPoints.size();
        }
        
        // Create points for every row
        laserPoints.clear();
        for (int y = 0; y < h; y++) {
            laserPoints.push_back(cv::Point(avgX, y));
        }
    }
    
    // Debug visualization
    if (debugMode) {
        // Draw detected laser line
        for (const auto& point : laserPoints) {
            cv::Point adjustedPoint = point;
            if (roiEnabled) {
                adjustedPoint.x += roiX;
                adjustedPoint.y += roiY;
            }
            cv::circle(debugResult, adjustedPoint, 2, cv::Scalar(0, 255, 0), -1);
        }
        
        // Show result
        std::string windowName = "Scanner-Style Laser Detection";
        cv::namedWindow(windowName, cv::WINDOW_NORMAL);
        cv::resizeWindow(windowName, 800, 600);
        cv::imshow(windowName, debugResult);
        cv::waitKey(1);
    }
    
    // Adjust coordinates back to original image if ROI was used
    if (roiEnabled && !laserPoints.empty()) {
        for (auto& point : laserPoints) {
            point.x += roiX;
            point.y += roiY;
        }
    }
    
    return laserPoints;
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

void LaserLineDetector::setGaussianBlur(int kernelSize, double sigma) {
    gaussianKernelSize = kernelSize;
    gaussianSigma = sigma;
    useGaussianBlur = (kernelSize > 0);
}

void LaserLineDetector::setMedianBlur(int kernelSize) {
    medianKernelSize = kernelSize;
    useMedianBlur = (kernelSize > 0);
}

void LaserLineDetector::setErosion(int iterations, int kernelSize) {
    erosionIterations = iterations;
    erosionKernelSize = kernelSize;
    useErosion = (iterations > 0);
}

void LaserLineDetector::setDilation(int iterations, int kernelSize) {
    dilationIterations = iterations;
    dilationKernelSize = kernelSize;
    useDilation = (iterations > 0);
}

void LaserLineDetector::setContrastEnhancement(double alpha, double beta) {
    contrastAlpha = alpha;
    contrastBeta = beta;
    enhanceContrast = true;
}

bool LaserLineDetector::optimizeROIAndDetectLaser(const cv::Mat& image) {
    
    
    // ROI seçimi yap
    if (!roiEnabled) {
        std::cout << "Lütfen lazer çizgisini içeren bölgeyi seçin (fareyle dikdörtgen çizin)." << std::endl;
        cv::namedWindow("Select ROI", cv::WINDOW_NORMAL);
        cv::resizeWindow("Select ROI", 640, 480);
        cv::Rect selectedROI = cv::selectROI("Select ROI", image, false, false);
        
        if (selectedROI.width == 0 || selectedROI.height == 0) {
            std::cout << "ROI seçimi iptal edildi!" << std::endl;
            cv::destroyWindow("Select ROI");
            return false;
        }
        
        setROI(selectedROI.x, selectedROI.y, selectedROI.width, selectedROI.height);
        std::cout << "ROI seçildi: x=" << selectedROI.x << ", y=" << selectedROI.y 
                 << ", genişlik=" << selectedROI.width << ", yükseklik=" << selectedROI.height << std::endl;
        
        cv::destroyWindow("Select ROI");
    }
    
    // ROI'yi çıkar
    cv::Mat roiImage;
    cv::Rect roi(roiX, roiY, roiWidth, roiHeight);
    roiImage = image(roi).clone();
    
    // Terminal tabanlı arayüz ile lazer tespitini optimize et
    bool optimizationDone = false;
    bool optimizationSuccess = false;
    
    // Başlangıç değerlerini ayarla
    int contrastValue = static_cast<int>(contrastAlpha * 10); // 0-50 arasında değer (1.0-5.0)
    int thresholdValue = binaryThreshold; // 0-255 arasında değer
    int gaussianValue = gaussianKernelSize; // 1-31 arasında tek sayı
    int medianValue = medianKernelSize; // 1-31 arasında tek sayı
    int erosionIter = erosionIterations; // Erosion iterasyon sayısı
    int erosionSize = erosionKernelSize; // Erosion kernel boyutu
    int dilationIter = dilationIterations; // Dilation iterasyon sayısı
    int dilationSize = dilationKernelSize; // Dilation kernel boyutu
    bool useGaussian = useGaussianBlur;
    bool useMedian = useMedianBlur;
    bool useErosionOp = useErosion;
    bool useDilationOp = useDilation;
    
    cv::Mat processedImage, grayImage, blurredImage, binaryImage, morphImage;
    std::vector<std::vector<cv::Point>> contours;
    
    // Sadece tek bir görselleştirme penceresi oluştur
    cv::namedWindow("Lazer Tespiti", cv::WINDOW_NORMAL);
    cv::resizeWindow("Lazer Tespiti", 640, 480);
    
    while (!optimizationDone) {
        // Mevcut değerleri göster
        std::cout << "\n===========================================" << std::endl;
        std::cout << "| Lazer Çizgisi Optimizasyonu             |" << std::endl;
        std::cout << "===========================================" << std::endl;
        std::cout << "Mevcut Ayarlar:" << std::endl;
        std::cout << "1. Kontrast: " << (contrastValue / 10.0) << std::endl;
        std::cout << "2. Binary Threshold: " << thresholdValue << std::endl;
        std::cout << "3. Gaussian Blur: " << (useGaussian ? "Açık" : "Kapalı") 
                 << " (Kernel: " << gaussianValue << ")" << std::endl;
        std::cout << "4. Median Blur: " << (useMedian ? "Açık" : "Kapalı") 
                 << " (Kernel: " << medianValue << ")" << std::endl;
        std::cout << "5. Erosion: " << (useErosionOp ? "Açık" : "Kapalı") 
                 << " (İterasyon: " << erosionIter << ", Kernel: " << erosionSize << ")" << std::endl;
        std::cout << "6. Dilation: " << (useDilationOp ? "Açık" : "Kapalı") 
                 << " (İterasyon: " << dilationIter << ", Kernel: " << dilationSize << ")" << std::endl;
        std::cout << "===========================================" << std::endl;
        std::cout << "Komutlar:" << std::endl;
        std::cout << "k [değer] - Kontrast değerini ayarla (10-50)" << std::endl;
        std::cout << "t [değer] - Threshold değerini ayarla (0-255)" << std::endl;
        std::cout << "g [değer] - Gaussian Blur kernel boyutunu ayarla (1-31 tek sayı, 0=kapalı)" << std::endl;
        std::cout << "m [değer] - Median Blur kernel boyutunu ayarla (1-31 tek sayı, 0=kapalı)" << std::endl;
        std::cout << "e [iter] [boyut] - Erosion parametrelerini ayarla (0=kapalı)" << std::endl;
        std::cout << "d [iter] [boyut] - Dilation parametrelerini ayarla (0=kapalı)" << std::endl;
        std::cout << "u - Güncel ayarları uygula ve görüntüle" << std::endl;
        std::cout << "o - Ayarları onayla ve çık" << std::endl;
        std::cout << "i - İptal et ve çık" << std::endl;
        std::cout << "===========================================" << std::endl;
        std::cout << "Komut: ";
        
        std::string command;
        std::cin >> command;
        
        if (command == "k") {
            int value;
            std::cin >> value;
            if (value >= 10 && value <= 50) {
                contrastValue = value;
                std::cout << "Kontrast değeri " << (contrastValue / 10.0) << " olarak ayarlandı." << std::endl;
            } else {
                std::cout << "Hata: Kontrast değeri 10-50 arasında olmalıdır." << std::endl;
            }
        } else if (command == "t") {
            int value;
            std::cin >> value;
            if (value >= 0 && value <= 255) {
                thresholdValue = value;
                std::cout << "Threshold değeri " << thresholdValue << " olarak ayarlandı." << std::endl;
            } else {
                std::cout << "Hata: Threshold değeri 0-255 arasında olmalıdır." << std::endl;
            }
        } else if (command == "g") {
            int value;
            std::cin >> value;
            if (value == 0) {
                useGaussian = false;
                std::cout << "Gaussian Blur kapatıldı." << std::endl;
            } else if ((value > 0 && value < 32) && (value % 2 == 1)) {
                useGaussian = true;
                gaussianValue = value;
                std::cout << "Gaussian Blur kernel boyutu " << gaussianValue << " olarak ayarlandı." << std::endl;
            } else {
                std::cout << "Hata: Gaussian Blur kernel boyutu 1-31 arasında tek sayı olmalıdır. Kapatmak için 0 girebilirsiniz." << std::endl;
            }
        } else if (command == "m") {
            int value;
            std::cin >> value;
            if (value == 0) {
                useMedian = false;
                std::cout << "Median Blur kapatıldı." << std::endl;
            } else if ((value > 0 && value < 32) && (value % 2 == 1)) {
                useMedian = true;
                medianValue = value;
                std::cout << "Median Blur kernel boyutu " << medianValue << " olarak ayarlandı." << std::endl;
            } else {
                std::cout << "Hata: Median Blur kernel boyutu 1-31 arasında tek sayı olmalıdır. Kapatmak için 0 girebilirsiniz." << std::endl;
            }
        } else if (command == "e") {
            int iter, size;
            std::cin >> iter >> size;
            
            if (iter == 0) {
                useErosionOp = false;
                std::cout << "Erosion kapatıldı." << std::endl;
            } else if (iter > 0 && iter <= 10 && size > 0 && size <= 15 && (size % 2 == 1)) {
                useErosionOp = true;
                erosionIter = iter;
                erosionSize = size;
                std::cout << "Erosion parametreleri ayarlandı: İterasyon=" << erosionIter << ", Kernel=" << erosionSize << std::endl;
            } else {
                std::cout << "Hata: İterasyon sayısı 1-10 arasında, kernel boyutu 1-15 arasında tek sayı olmalıdır." << std::endl;
            }
        } else if (command == "d") {
            int iter, size;
            std::cin >> iter >> size;
            
            if (iter == 0) {
                useDilationOp = false;
                std::cout << "Dilation kapatıldı." << std::endl;
            } else if (iter > 0 && iter <= 10 && size > 0 && size <= 15 && (size % 2 == 1)) {
                useDilationOp = true;
                dilationIter = iter;
                dilationSize = size;
                std::cout << "Dilation parametreleri ayarlandı: İterasyon=" << dilationIter << ", Kernel=" << dilationSize << std::endl;
            } else {
                std::cout << "Hata: İterasyon sayısı 1-10 arasında, kernel boyutu 1-15 arasında tek sayı olmalıdır." << std::endl;
            }
        } else if (command == "u") {
            // Görüntü işleme adımlarını uygula
            cv::Mat result = roiImage.clone();
            cv::Mat displayResult;
            
            // 1. Kontrast artır
            roiImage.convertTo(processedImage, -1, contrastValue / 10.0, 0);
            
            // 2. Görüntüyü gri tonlamaya dönüştür
            cv::cvtColor(processedImage, grayImage, cv::COLOR_BGR2GRAY);
            
            // 3. Median Blur uygula
            if (useMedian) {
                cv::medianBlur(grayImage, blurredImage, medianValue);
            } else {
                blurredImage = grayImage.clone();
            }
            
            // 4. Gaussian Blur uygula
            if (useGaussian) {
                cv::GaussianBlur(blurredImage, blurredImage, 
                                cv::Size(gaussianValue, gaussianValue), 0);
            }
            
            // 5. Keskinleştir
            cv::Mat sharpenedImage;
            cv::Laplacian(blurredImage, sharpenedImage, CV_8U, 3, 1, 0);
            cv::convertScaleAbs(sharpenedImage, sharpenedImage);
            cv::addWeighted(blurredImage, 1.5, sharpenedImage, -0.5, 0, sharpenedImage);
            
            // 6. Binary image oluştur
            cv::threshold(blurredImage, binaryImage, thresholdValue, 255, cv::THRESH_BINARY);
            
            // 7. Morfolojik işlemler - Erosion
            morphImage = binaryImage.clone();
            if (useErosionOp) {
                cv::Mat erosionKernel = cv::getStructuringElement(
                    cv::MORPH_ELLIPSE, 
                    cv::Size(erosionSize, erosionSize)
                );
                cv::erode(morphImage, morphImage, erosionKernel, cv::Point(-1, -1), erosionIter);
            }
            
            // 8. Morfolojik işlemler - Dilation
            if (useDilationOp) {
                cv::Mat dilationKernel = cv::getStructuringElement(
                    cv::MORPH_ELLIPSE, 
                    cv::Size(dilationSize, dilationSize)
                );
                cv::dilate(morphImage, morphImage, dilationKernel, cv::Point(-1, -1), dilationIter);
            }
            
            // Konturları bul
            contours.clear();
            cv::findContours(morphImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            
            // Sonuç görüntüsünü hazırla
            cv::cvtColor(morphImage, displayResult, cv::COLOR_GRAY2BGR);
            
            // Konturları ekle
            if (!contours.empty()) {
                // En büyük konturu bul
                int largestContourIdx = 0;
                double largestArea = 0;
                
                for (size_t i = 0; i < contours.size(); i++) {
                    double area = cv::contourArea(contours[i]);
                    if (area > largestArea) {
                        largestArea = area;
                        largestContourIdx = i;
                    }
                }
                
                // Konturları çiz
                for (size_t i = 0; i < contours.size(); i++) {
                    cv::Scalar color = (i == largestContourIdx) ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
                    cv::drawContours(displayResult, contours, i, color, 2);
                }
                
                // İstatistikleri göster
                std::cout << "Tespit edilen kontur sayısı: " << contours.size() << std::endl;
                std::cout << "En büyük kontur alanı: " << cv::contourArea(contours[largestContourIdx]) << std::endl;
            } else {
                std::cout << "Kontur bulunamadı!" << std::endl;
            }
            
            // Sonucu göster (sadece tek pencere)
            cv::imshow("Lazer Tespiti", displayResult);
            cv::waitKey(100);
        } else if (command == "o") {
            // Ayarları onayla
            contrastAlpha = contrastValue / 10.0;
            binaryThreshold = thresholdValue;
            useGaussianBlur = useGaussian;
            gaussianKernelSize = gaussianValue;
            useMedianBlur = useMedian;
            medianKernelSize = medianValue;
            useErosion = useErosionOp;
            erosionIterations = erosionIter;
            erosionKernelSize = erosionSize;
            useDilation = useDilationOp;
            dilationIterations = dilationIter;
            dilationKernelSize = dilationSize;
            
            std::cout << "Ayarlar onaylandı:" << std::endl;
            std::cout << "Kontrast: " << contrastAlpha << std::endl;
            std::cout << "Binary Threshold: " << binaryThreshold << std::endl;
            std::cout << "Gaussian Blur: " << (useGaussianBlur ? "Açık" : "Kapalı") 
                     << " (Kernel: " << gaussianKernelSize << ")" << std::endl;
            std::cout << "Median Blur: " << (useMedianBlur ? "Açık" : "Kapalı") 
                     << " (Kernel: " << medianKernelSize << ")" << std::endl;
            std::cout << "Erosion: " << (useErosion ? "Açık" : "Kapalı") 
                     << " (İterasyon: " << erosionIterations << ", Kernel: " << erosionKernelSize << ")" << std::endl;
            std::cout << "Dilation: " << (useDilation ? "Açık" : "Kapalı") 
                     << " (İterasyon: " << dilationIterations << ", Kernel: " << dilationKernelSize << ")" << std::endl;
            
            optimizationDone = true;
            optimizationSuccess = true;
        } else if (command == "i") {
            // İptal et
            std::cout << "İşlem iptal edildi." << std::endl;
            optimizationDone = true;
            optimizationSuccess = false;
        } else {
            std::cout << "Geçersiz komut! Lütfen tekrar deneyin." << std::endl;
        }
    }
    
    // Pencereleri kapat
    cv::destroyAllWindows();
    
    return optimizationSuccess;
}

void LaserLineDetector::setDebugMode(bool enable) { 
    debugMode = enable; 
}