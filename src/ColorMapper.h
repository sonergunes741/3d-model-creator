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
     * @brief Görüntü dosya yolundan renk bilgisi ekler (batch processing için)
     * 
     * @param imagePath Renk bilgisi içeren görüntü dosyasının yolu
     * @param angle Görüntünün çekildiği açı (derece)
     */
    void addColorData(
        const std::string& imagePath, 
        float angle
    );

    /**
     * @brief Görüntülerden renk bilgisi ekler (eski versiyon - geriye uyumluluk)
     * 
     * @param colorImage Renk bilgisi içeren görüntü (tüm görüntü)
     * @param angle Görüntünün çekildiği açı (derece)
     */
    void addColorDataLegacy(
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
        colorImagePaths.clear();
        angles.clear();
    }

private:
    std::vector<std::string> colorImagePaths;  // Store file paths instead of loaded images
    std::vector<float> angles;
    
    // Batch processing parameters
    static const size_t BATCH_SIZE = 200;  // Process 200 images at a time
    
    /**
     * @brief Belirtilen indeksteki görüntüyü yükler
     * 
     * @param index Görüntü indeksi
     * @return cv::Mat Yüklenen görüntü
     */
    cv::Mat loadImageAtIndex(size_t index) const;

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