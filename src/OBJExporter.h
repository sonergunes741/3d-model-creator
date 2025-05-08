#pragma once

#include <pcl/PolygonMesh.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>  // PointXYZRGB için
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

/**
 * @brief 3D modeli OBJ formatında dışa aktarma sınıfı
 * 
 * Bu sınıf PCL PolygonMesh'i OBJ, MTL ve PNG dosyalarına dönüştürür.
 */
class OBJExporter {
public:
    /**
     * @brief Yapıcı fonksiyon
     */
    OBJExporter();
    
    /**
     * @brief Mesh'i OBJ formatında dışa aktarır
     * 
     * @param mesh PCL PolygonMesh
     * @param outputPath Çıktı dosya yolu (uzantı olmadan)
     * @param modelName Model adı
     * @return bool Başarı durumu
     */
    bool exportMesh(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName = "3DModel");
    
    /**
     * @brief Vertex renklerini kullanma durumunu ayarlar
     * 
     * @param useColor Vertex renkleri kullanılsın mı?
     */
    void setUseVertexColors(bool useColor) {
        useVertexColors = useColor;
    }

    /**
     * @brief Texture çözünürlüğünü ayarlar
     * 
     * @param width Texture genişliği
     * @param height Texture yüksekliği
     */
    void setTextureResolution(int width, int height) {
        textureWidth = width;
        textureHeight = height;
    }

private:
    bool useVertexColors;
    int textureWidth;
    int textureHeight;
    
    /**
     * @brief OBJ dosyasını oluşturur
     * 
     * @param mesh PCL PolygonMesh
     * @param objFilePath OBJ dosya yolu
     * @param mtlFileName MTL dosya adı (uzantı olmadan)
     * @param textureFileName Texture dosya adı (uzantı olmadan)
     * @return bool Başarı durumu
     */
    bool writeOBJFile(
        const pcl::PolygonMesh& mesh, 
        const std::string& objFilePath,
        const std::string& mtlFileName,
        const std::string& textureFileName
    );
    
    /**
     * @brief MTL dosyasını oluşturur
     * 
     * @param mtlFilePath MTL dosya yolu
     * @param materialName Materyal adı
     * @param textureFileName Texture dosya adı (uzantı ile)
     * @return bool Başarı durumu
     */
    bool writeMTLFile(
        const std::string& mtlFilePath,
        const std::string& materialName,
        const std::string& textureFileName
    );
    
    /**
     * @brief Texture dosyasını oluşturur
     * 
     * @param mesh PCL PolygonMesh
     * @param textureFilePath Texture dosya yolu
     * @return bool Başarı durumu
     */
    bool writeTextureFile(
        const pcl::PolygonMesh& mesh,
        const std::string& textureFilePath
    );

    /**
     * @brief UV koordinatlarını oluşturur
     * 
     * @param mesh PCL PolygonMesh
     * @return std::vector<std::pair<float, float>> UV koordinatları
     */
    std::vector<std::pair<float, float>> generateUVCoordinates(const pcl::PolygonMesh& mesh);

    /**
     * @brief Mesh'ten renk verilerini çıkarır
     * 
     * @param mesh PCL PolygonMesh
     * @return pcl::PointCloud<pcl::PointXYZRGB>::Ptr Renkli nokta bulutu
     */
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr extractColors(const pcl::PolygonMesh& mesh);

    /**
     * @brief Texture görüntüsünü oluşturur
     * 
     * @param mesh PCL PolygonMesh
     * @param uvCoordinates UV koordinatları
     * @return cv::Mat Oluşturulan texture görüntüsü
     */
    cv::Mat createTextureImage(
        const pcl::PolygonMesh& mesh,
        const std::vector<std::pair<float, float>>& uvCoordinates
    );
};