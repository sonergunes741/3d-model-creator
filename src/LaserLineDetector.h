#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

/**
 * @brief Lazer çizgisi tespiti için sınıf
 * 
 * Bu sınıf, görüntülerdeki kırmızı lazer çizgilerini tespit eder.
 */
class LaserLineDetector {
public:
    /**
     * @brief Yapıcı fonksiyon
     * 
     * @param lowerThresh Lazer rengi için alt HSV eşiği
     * @param upperThresh Lazer rengi için üst HSV eşiği
     */
    LaserLineDetector(
        const cv::Scalar& lowerThresh = cv::Scalar(140, 30, 30),  // Daha düşük değerler
        const cv::Scalar& upperThresh = cv::Scalar(179, 255, 255)
    );

    /**
     * @brief Verilen görüntüdeki lazer çizgisini tespit eder
     * 
     * @param image Lazer çizgisi içeren görüntü
     * @return std::vector<cv::Point> Tespit edilen lazer çizgisi noktaları
     */
    std::vector<cv::Point> detectLaserLine(const cv::Mat& image);

    /**
     * @brief Lazerle ilgili eşik değerlerini ayarlar
     * 
     * @param lowerThresh Lazer rengi için alt HSV eşiği
     * @param upperThresh Lazer rengi için üst HSV eşiği
     */
    void setThresholds(const cv::Scalar& lowerThresh, const cv::Scalar& upperThresh);

    /**
     * @brief Debug modunu açar/kapatır
     * 
     * @param enable Debug modu aktif/pasif
     */
    void setDebugMode(bool enable) { debugMode = enable; }

    /**
     * @brief Kontrast artırma seviyesini ayarlar
     * 
     * @param alpha Kontrast çarpanı (1.0 = değişiklik yok, >1.0 kontrast artırma)
     * @param beta Parlaklık değeri (0 = değişiklik yok)
     */
    void setContrastEnhancement(double alpha = 1.5, double beta = 0) {
        contrastAlpha = alpha;
        contrastBeta = beta;
        enhanceContrast = true;
    }
    
    /**
     * @brief İlgi bölgesini (ROI) ayarlar
     * 
     * @param x ROI'nin sol üst köşesinin x koordinatı
     * @param y ROI'nin sol üst köşesinin y koordinatı
     * @param width ROI'nin genişliği
     * @param height ROI'nin yüksekliği
     */
    void setROI(int x, int y, int width, int height);
    
    /**
     * @brief ROI kullanımını devre dışı bırakır
     */
    void disableROI();

private:
    cv::Scalar lowerThreshold;
    cv::Scalar upperThreshold;
    bool debugMode;
    bool enhanceContrast;
    double contrastAlpha;
    double contrastBeta;
    bool roiEnabled;
    int roiX, roiY, roiWidth, roiHeight;

    /**
     * @brief Konturlardan lazer çizgisini seçer
     * 
     * @param contours Tespit edilen tüm konturlar
     * @return std::vector<cv::Point> Seçilen lazer çizgisi noktaları
     */
    std::vector<cv::Point> selectLaserLine(const std::vector<std::vector<cv::Point>>& contours);
};