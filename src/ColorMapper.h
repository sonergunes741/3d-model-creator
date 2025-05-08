#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>  // PointXYZRGB için
#include <pcl/PolygonMesh.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

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
     * @param colorImage Renk bilgisi içeren görüntü
     * @param angle Görüntünün çekildiği açı (derece)
     * @param imageWidth Görüntü genişliği
     * @param imageHeight Görüntü yüksekliği
     */
    void addColorData(
        const cv::Mat& colorImage, 
        float angle, 
        int imageWidth, 
        int imageHeight
    );

    /**
     * @brief Renk bilgisini mesh'e uygular
     * 
     * @param mesh Renklendirilecek mesh
     * @param cloud Orijinal nokta bulutu (referans için)
     */
    void applyColorToMesh(
        pcl::PolygonMesh& mesh, 
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud
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
     * @brief Görüntünün merkez çizgisinden renk verisini alır
     * 
     * @param colorImage Renk görüntüsü
     * @param imageWidth Görüntü genişliği
     * @return std::vector<cv::Vec3b> Merkez çizgideki renk değerleri
     */
    std::vector<cv::Vec3b> extractCenterLineColors(
        const cv::Mat& colorImage, 
        int imageWidth
    );

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