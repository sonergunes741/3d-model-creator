#pragma once

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/PolygonMesh.h>
#include <pcl/common/common.h>

/**
 * @brief Nokta bulutundan 3D mesh oluşturan sınıf
 */
class MeshCreator {
public:
    /**
     * @brief Yapıcı fonksiyon
     * 
     * @param depth Poisson rekonstrüksiyon derinliği
     */
    MeshCreator(int depth = 9);

    /**
     * @brief Nokta bulutundan mesh oluşturur
     * 
     * @param cloud Girdi nokta bulutu
     * @return Oluşturulan mesh
     */
    pcl::PolygonMesh createMesh(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud);

    /**
     * @brief Mesh'i düzgünleştirir ve optimize eder
     * 
     * @param mesh Düzgünleştirilecek mesh
     */
    void refineMesh(pcl::PolygonMesh& mesh);

    /**
     * @brief Poisson rekonstrüksiyon derinliğini ayarlar
     * 
     * @param depth Yeni derinlik değeri
     */
    void setDepth(int depth) { poissonDepth = depth; }

    /**
     * @brief Düzgünleştirme parametrelerini ayarlar
     * 
     * @param iterations İterasyon sayısı
     * @param factor Düzgünleştirme faktörü
     */
    void setSmoothingParameters(int iterations, float factor) {
        smoothingIterations = iterations;
        smoothingFactor = factor;
    }

private:
    int poissonDepth;
    int smoothingIterations;
    float smoothingFactor;

    /**
     * @brief Nokta bulutu normalleri hesaplar
     * 
     * @param cloud Girdi nokta bulutu
     * @param cloudWithNormals Normalleri hesaplanmış nokta bulutu
     */
    void computeNormals(
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
        pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudWithNormals
    );
};