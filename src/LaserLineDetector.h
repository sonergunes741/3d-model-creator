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
        const cv::Scalar& lowerThresh = cv::Scalar(160, 100, 100),
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

private:
    cv::Scalar lowerThreshold;
    cv::Scalar upperThreshold;
    bool debugMode;

    /**
     * @brief Konturlardan lazer çizgisini seçer
     * 
     * @param contours Tespit edilen tüm konturlar
     * @return std::vector<cv::Point> Seçilen lazer çizgisi noktaları
     */
    std::vector<cv::Point> selectLaserLine(const std::vector<std::vector<cv::Point>>& contours);
};