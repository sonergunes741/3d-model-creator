#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

/**
 * @brief Lazer çizgilerinden 3D nokta bulutu oluşturan sınıf
 */
class PointCloudBuilder {
public:
    struct PointWithImageInfo {
        pcl::PointXYZRGB point;
        float angle;
        int imageX;
        int imageY;
        int imageIndex; // index of the scan angle/color image
    };
    std::vector<PointWithImageInfo> pointImageInfo;

    /**
     * @brief Varsayılan yapıcı fonksiyon
     */
    PointCloudBuilder();
    
    /**
     * @brief Kamera parametreleriyle yapıcı fonksiyon
     * 
     * @param cameraMatrix Kamera kalibrasyon matrisi
     * @param distCoeffs Bozulma katsayıları
     */
    PointCloudBuilder(
        const cv::Mat& cameraMatrix,
        const cv::Mat& distCoeffs
    );

    /**
     * @brief Lazer çizgisinden 3D noktalar oluşturur
     * 
     * @param laserLine Görüntüdeki lazer çizgisi noktaları
     * @param angle Tarama açısı (derece)
     * @param imageWidth Görüntü genişliği
     * @param imageHeight Görüntü yüksekliği
     */
    void addLineToCloud(
        const std::vector<cv::Point>& laserLine, 
        float angle, 
        int imageWidth, 
        int imageHeight
    );

    /**
     * @brief Nokta bulutunu filtreler ve hazırlar
     */
    void processPointCloud();

    /**
     * @brief Oluşturulan nokta bulutunu alır
     * 
     * @return pcl::PointCloud<pcl::PointXYZRGB>::Ptr Nokta bulutu
     */
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr getPointCloud() const {
        return cloud;
    }
    
    /**
     * @brief Oluşturulan nokta bulutunu alır (alternatif metod)
     * 
     * @return pcl::PointCloud<pcl::PointXYZRGB>::Ptr Nokta bulutu
     */
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr getCloud();

    /**
     * @brief Kamera parametrelerini ayarlar
     * 
     * @param cameraMatrix Kamera kalibrasyon matrisi
     * @param distCoeffs Bozulma katsayıları
     */
    void setCameraParameters(const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs);

    /**
     * @brief Tarama parametrelerini ayarlar
     * 
     * @param radius Tarama yarıçapı
     * @param centerX Merkez X koordinatı
     * @param centerY Merkez Y koordinatı
     * @param centerZ Merkez Z koordinatı
     */
    void setScanParameters(float radius, float centerX, float centerY, float centerZ);
    
    /**
     * @brief Rotasyon merkezi X koordinatını ayarlar (scanner yaklaşımı)
     * 
     * @param centerX Rotasyon merkezi X koordinatı (piksel cinsinden)
     */
    void setRotationCenterX(int centerX);
    
    /**
     * @brief İlk görüntüden rotasyon merkezini otomatik belirler (scanner yaklaşımı)
     * 
     * @param laserLine İlk görüntüdeki lazer çizgisi noktaları
     * @param imageWidth Görüntü genişliği
     */
    void determineRotationCenterFromFirstImage(const std::vector<cv::Point>& laserLine, int imageWidth);

    /**
     * @brief Scanner yaklaşımı ile nokta filtreleme (dikey hassasiyet kontrolü)
     * 
     * @param verticalPrecision Dikey hassasiyet yüzdesi (0-100)
     */
    void applyScannerStyleFiltering(int verticalPrecision = 100);

private:
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud;
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
    float scanRadius;
    float scanCenterX, scanCenterY, scanCenterZ;
    int rotationCenterX;
    bool useCustomRotationCenter;

    /**
     * @brief Görüntü noktasını 3D dünya koordinatlarına dönüştürür
     * 
     * @param point Görüntü noktası
     * @param angle Tarama açısı (derece)
     * @param imageWidth Görüntü genişliği
     * @param imageHeight Görüntü yüksekliği
     * @return pcl::PointXYZRGB 3D nokta
     */
    pcl::PointXYZRGB projectPointTo3D(
        const cv::Point& point, 
        float angle, 
        int imageWidth, 
        int imageHeight
    );
};