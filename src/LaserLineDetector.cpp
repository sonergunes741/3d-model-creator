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
    
    // HSV renk uzayına dönüştür
    cv::Mat hsvImage;
    cv::cvtColor(processedImage, hsvImage, cv::COLOR_BGR2HSV);
    
    // Renk eşikleme ile lazer çizgisini tespit et
    cv::Mat mask;
    cv::inRange(hsvImage, lowerThreshold, upperThreshold, mask);
    
    // Median blur uygula
    if (useMedianBlur && medianKernelSize > 0) {
        cv::medianBlur(mask, mask, medianKernelSize);
    }
    
    // Gaussian blur uygula
    if (useGaussianBlur && gaussianKernelSize > 0) {
        cv::GaussianBlur(mask, mask, 
                        cv::Size(gaussianKernelSize, gaussianKernelSize), 
                        gaussianSigma);
    }
    
    // Morfolojik işlemler - Erosion
    if (useErosion) {
        cv::Mat erosionKernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE, 
            cv::Size(erosionKernelSize, erosionKernelSize)
        );
        cv::erode(mask, mask, erosionKernel, cv::Point(-1, -1), erosionIterations);
    }
    
    // Morfolojik işlemler - Dilation
    if (useDilation) {
        cv::Mat dilationKernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE, 
            cv::Size(dilationKernelSize, dilationKernelSize)
        );
        cv::dilate(mask, mask, dilationKernel, cv::Point(-1, -1), dilationIterations);
    }
    
    // Lazer çizgisi noktalarını bul
    std::vector<cv::Point> laserPoints;
    
    // Her satır için en parlak noktayı bul
    for (int y = 0; y < mask.rows; y++) {
        std::vector<int> whitePixels;
        
        // Bu satırdaki beyaz pikselleri bul
        for (int x = 0; x < mask.cols; x++) {
            if (mask.at<uchar>(y, x) > 0) {
                whitePixels.push_back(x);
            }
        }
        
        // Eğer beyaz piksel varsa, ortasını al
        if (!whitePixels.empty()) {
            int centerX = whitePixels[whitePixels.size() / 2];
            laserPoints.push_back(cv::Point(centerX, y));
        }
    }
    
    // Boşlukları doldur
    laserPoints = interpolatePoints(laserPoints, 20);
    
    // Eğer çok az nokta bulunduysa, yapay çizgi oluştur
    if (laserPoints.size() < mask.rows * 0.1) {
        std::cout << "UYARI: Çok az lazer noktası tespit edildi! Yapay nokta oluşturuluyor." << std::endl;
        
        // Bulunan noktalardan ortalama x pozisyonu hesapla
        int avgX = mask.cols / 2;  // Varsayılan olarak merkez
        if (!laserPoints.empty()) {
            int sumX = 0;
            for (const auto& p : laserPoints) {
                sumX += p.x;
            }
            avgX = sumX / laserPoints.size();
        }
        
        // Her satır için nokta oluştur
        laserPoints.clear();
        for (int y = 0; y < mask.rows; y++) {
            laserPoints.push_back(cv::Point(avgX, y));
        }
    }
    
    // Debug görselleştirmesi
    if (debugMode) {
        // Tespit edilen lazer çizgisini çiz
        for (const auto& point : laserPoints) {
            cv::Point adjustedPoint = point;
            if (roiEnabled) {
                adjustedPoint.x += roiX;
                adjustedPoint.y += roiY;
            }
            cv::circle(debugResult, adjustedPoint, 2, cv::Scalar(0, 255, 0), -1);
        }
        
        // Sonucu göster
        std::string windowName = "Laser Line Detection";
        cv::namedWindow(windowName, cv::WINDOW_NORMAL);
        cv::resizeWindow(windowName, 800, 600);
        cv::imshow(windowName, debugResult);
        cv::waitKey(1);
    }
    
    // ROI kullanıldıysa koordinatları orijinal görüntüye göre ayarla
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
    
    // Automatically proceed with default values - no optimization interface
    std::cout << "ROI seçimi tamamlandı. Varsayılan ayarlarla devam ediliyor..." << std::endl;
    std::cout << "Kullanılan ayarlar:" << std::endl;
    std::cout << "  Kontrast: " << contrastAlpha << std::endl;
    std::cout << "  Binary Threshold: " << binaryThreshold << std::endl;
    std::cout << "  Gaussian Blur: " << (useGaussianBlur ? "Açık" : "Kapalı") 
             << " (Kernel: " << gaussianKernelSize << ")" << std::endl;
    std::cout << "  Median Blur: " << (useMedianBlur ? "Açık" : "Kapalı") 
             << " (Kernel: " << medianKernelSize << ")" << std::endl;
    std::cout << "  Erosion: " << (useErosion ? "Açık" : "Kapalı") 
             << " (İterasyon: " << erosionIterations << ", Kernel: " << erosionKernelSize << ")" << std::endl;
    std::cout << "  Dilation: " << (useDilation ? "Açık" : "Kapalı") 
             << " (İterasyon: " << dilationIterations << ", Kernel: " << dilationKernelSize << ")" << std::endl;
    
    // No windows created, no user interaction required
    // Simply return success with current default settings
    return true;
}

void LaserLineDetector::setDebugMode(bool enable) { 
    debugMode = enable; 
}