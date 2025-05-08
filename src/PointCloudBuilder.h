#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>  // PointXYZRGB için
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

/**
 * @brief Lazer çizgilerinden 3D nokta bulutu oluşturan sınıf
 */
class PointCloudBuilder {
public:
    /**
     * @brief Yapıcı fonksiyon
     * 
     * @param cameraMatrix Kamera kalibrasyon matrisi
     * @param distCoeffs Bozulma katsayıları
     */
    PointCloudBuilder(
        const cv::Mat& cameraMatrix = cv::Mat::eye(3, 3, CV_64F),
        const cv::Mat& distCoeffs = cv::Mat::zeros(5, 1, CV_64F)
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
     * @brief Kamera parametrelerini ayarlar
     * 
     * @param cameraMatrix Kamera kalibrasyon matrisi
     * @param distCoeffs Bozulma katsayıları
     */
    void setCameraParameters(const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs);

    /**
     * @brief Tarama parametrelerini ayarlar
     * 
     * @param radius Tarama mesafesi (mm)
     * @param centerX Tarama merkezi X (mm)
     * @param centerY Tarama merkezi Y (mm)
     * @param centerZ Tarama merkezi Z (mm)
     */
    void setScanParameters(
        float radius = 200.0f,
        float centerX = 0.0f,
        float centerY = 0.0f,
        float centerZ = 0.0f
    );

private:
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud;
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
    float scanRadius;
    float scanCenterX;
    float scanCenterY;
    float scanCenterZ;

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