#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>  // PointXYZRGB için
#include <pcl/PolygonMesh.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "PointCloudBuilder.h"

/**
 * @brief Renk bilgisini 3D modele uygulayan sınıf
 */
class ColorMapper {
public:
    /**
     * @brief Yapıcı fonksiyon
     */
    ColorMapper();

    /**
     * @brief Görüntülerden renk bilgisi ekler
     * 
     * @param colorImage Renk bilgisi içeren görüntü (tüm görüntü)
     * @param angle Görüntünün çekildiği açı (derece)
     */
    void addColorData(
        const cv::Mat& colorImage, 
        float angle
    );

    /**
     * @brief Renk bilgisini mesh'e uygular
     * 
     * @param mesh Renklendirilecek mesh
     * @param cloud Orijinal nokta bulutu (referans için)
     */
    void applyColorToMesh(
        pcl::PolygonMesh& mesh, 
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
        const std::vector<PointCloudBuilder::PointWithImageInfo>& pointImageInfo
    );

    /**
     * @brief Renk verisini sıfırlar
     */
    void resetColorData() {
        colorData.clear();
        angles.clear();
    }

private:
    std::vector<cv::Mat> colorData;
    std::vector<float> angles;

    /**
     * @brief Mesh noktasının açısını hesaplar
     * 
     * @param point 3D nokta
     * @return float Hesaplanan açı (derece)
     */
    float calculateAngle(const pcl::PointXYZ& point);

    /**
     * @brief İki açı arasındaki farkı hesaplar
     * 
     * @param angle1 Birinci açı (derece)
     * @param angle2 İkinci açı (derece)
     * @return float Açı farkı (0-180 arası)
     */
    float angleDifference(float angle1, float angle2);
};