#pragma once

#include <pcl/PolygonMesh.h>
#include <fbxsdk.h>
#include <string>

/**
 * @brief 3D modeli FBX formatında dışa aktarma sınıfı
 */
class FBXExporter {
public:
    /**
     * @brief Yapıcı fonksiyon
     */
    FBXExporter();
    
    /**
     * @brief Yıkıcı fonksiyon
     */
    ~FBXExporter();
    
    /**
     * @brief Mesh'i FBX formatında dışa aktarır
     * 
     * @param mesh PCL PolygonMesh
     * @param outputPath Çıktı dosya yolu
     * @param modelName Model adı
     * @return bool Başarı durumu
     */
    bool exportMesh(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName = "3DModel");
    
    /**
     * @brief FBX sürümünü ayarlar
     * 
     * @param major Major sürüm
     * @param minor Minor sürüm
     * @param revision Revizyon
     */
    void setFBXVersion(int major = 7, int minor = 5, int revision = 0) {
        fbxMajorVersion = major;
        fbxMinorVersion = minor;
        fbxRevision = revision;
    }
    
    /**
     * @brief Vertex renklerini kullanma durumunu ayarlar
     * 
     * @param useColor Vertex renkleri kullanılsın mı?
     */
    void setUseVertexColors(bool useColor) {
        useVertexColors = useColor;
    }

private:
    FbxManager* fbxManager;
    FbxScene* fbxScene;
    int fbxMajorVersion;
    int fbxMinorVersion;
    int fbxRevision;
    bool useVertexColors;
    
    /**
     * @brief FBX mesh oluşturur
     * 
     * @param mesh PCL PolygonMesh
     * @param modelName Model adı
     * @return FbxNode* Oluşturulan FBX nodu
     */
    FbxNode* createFBXMesh(const pcl::PolygonMesh& mesh, const std::string& modelName);
    
    /**
     * @brief FBX materyal oluşturur
     * 
     * @param materialName Materyal adı
     * @return FbxSurfacePhong* Oluşturulan FBX materyal
     */
    FbxSurfacePhong* createMaterial(const std::string& materialName);
    
    /**
     * @brief FBX dokusu oluşturur
     * 
     * @param textureName Doku adı
     * @param mesh PCL PolygonMesh
     * @return FbxFileTexture* Oluşturulan FBX dokusu
     */
    FbxFileTexture* createTexture(const std::string& textureName, const pcl::PolygonMesh& mesh);
};