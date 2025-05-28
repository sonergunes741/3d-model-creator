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
        const cv::Scalar& lowerThresh = cv::Scalar(140, 30, 30),
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
    void setDebugMode(bool enable);

    /**
     * @brief Kontrast artırma seviyesini ayarlar
     * 
     * @param alpha Kontrast çarpanı (1.0 = değişiklik yok, >1.0 kontrast artırma)
     * @param beta Parlaklık değeri (0 = değişiklik yok)
     */
    void setContrastEnhancement(double alpha = 1.5, double beta = 0);
    
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
    
    /**
     * @brief ROI üzerinde görüntü işleme uygulanıp lazer çizgisi tespit edilir
     *
     * @param image Lazer görüntüsü
     * @return bool İşlem başarılı mı
     */
    bool optimizeROIAndDetectLaser(const cv::Mat& image);

    /**
     * @brief Gaussian blur parametrelerini ayarlar
     *
     * @param kernelSize Gaussian blur çekirdeği boyutu (tek sayı olmalı)
     * @param sigma Gaussian blur sigma değeri
     */
    void setGaussianBlur(int kernelSize, double sigma);

    /**
     * @brief Median blur parametrelerini ayarlar
     *
     * @param kernelSize Median blur çekirdeği boyutu (tek sayı olmalı)
     */
    void setMedianBlur(int kernelSize);
    
    /**
     * @brief Erosion parametrelerini ayarlar
     *
     * @param iterations Erosion iterasyon sayısı
     * @param kernelSize Erosion çekirdeği boyutu
     */
    void setErosion(int iterations, int kernelSize);
    
    /**
     * @brief Dilation parametrelerini ayarlar
     *
     * @param iterations Dilation iterasyon sayısı
     * @param kernelSize Dilation çekirdeği boyutu
     */
    void setDilation(int iterations, int kernelSize);

    /**
     * @brief Renkli görüntüden otomatik ROI tespiti yapar
     * 
     * @param colorImage Renkli görüntü
     * @param padding ROI etrafına eklenecek boşluk (piksel)
     * @return bool ROI başarıyla tespit edildi mi
     */
    bool detectROIFromColorImage(const cv::Mat& colorImage, int padding = 20);

private:
    cv::Scalar lowerThreshold;
    cv::Scalar upperThreshold;
    bool debugMode;
    bool enhanceContrast;
    double contrastAlpha;
    double contrastBeta;
    bool roiEnabled;
    int roiX, roiY, roiWidth, roiHeight;
    int binaryThreshold;
    
    // Blurring parametreleri
    bool useGaussianBlur;
    int gaussianKernelSize;
    double gaussianSigma;
    bool useMedianBlur;
    int medianKernelSize;
    
    // Morfolojik işlem parametreleri
    bool useErosion;
    int erosionIterations;
    int erosionKernelSize;
    bool useDilation;
    int dilationIterations;
    int dilationKernelSize;

    /**
     * @brief Konturlardan lazer çizgisini seçer
     * 
     * @param contours Tespit edilen tüm konturlar
     * @return std::vector<cv::Point> Seçilen lazer çizgisi noktaları
     */
    std::vector<cv::Point> selectLaserLine(const std::vector<std::vector<cv::Point>>& contours);
};