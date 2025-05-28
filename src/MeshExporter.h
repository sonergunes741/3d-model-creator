#pragma once

#include <pcl/PolygonMesh.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/**
 * @brief Multi-format 3D model exporter
 * 
 * Supports OBJ, FBX, and GLTF/GLB export formats
 */
class MeshExporter {
public:
    enum class ExportFormat {
        OBJ,    // OBJ + MTL + PNG
        PLY,    // PLY format
        FBX     // FBX format
    };

    /**
     * @brief Constructor
     */
    MeshExporter();
    
    /**
     * @brief Export mesh to specified format
     * 
     * @param mesh PCL PolygonMesh
     * @param outputPath Output file path (without extension)
     * @param format Export format
     * @param modelName Model name
     * @return bool Success status
     */
    bool exportMesh(
        const pcl::PolygonMesh& mesh, 
        const std::string& outputPath, 
        ExportFormat format,
        const std::string& modelName = "3DModel"
    );
    
    /**
     * @brief Export to all supported formats (OBJ, PLY, FBX)
     * 
     * @param mesh PCL PolygonMesh
     * @param basePath Base output path (without extension)
     * @param modelName Model name
     * @return bool Success status
     */
    bool exportAllFormats(
        const pcl::PolygonMesh& mesh,
        const std::string& basePath,
        const std::string& modelName = "3DModel"
    );

    /**
     * @brief Set texture resolution
     * 
     * @param width Texture width
     * @param height Texture height
     */
    void setTextureResolution(int width, int height) {
        textureWidth = width;
        textureHeight = height;
    }

    /**
     * @brief Set vertex colors usage
     * 
     * @param useColor Use vertex colors?
     */
    void setUseVertexColors(bool useColor) {
        useVertexColors = useColor;
    }

private:
    bool useVertexColors;
    int textureWidth;
    int textureHeight;
    
    // Export methods for different formats
    bool exportOBJ(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName);
    bool exportPLY(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName);
    bool exportFBX(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName);
    
    // Helper methods
    std::vector<std::pair<float, float>> generateUVCoordinates(const pcl::PolygonMesh& mesh);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr extractColors(const pcl::PolygonMesh& mesh);
    cv::Mat createTextureImage(const pcl::PolygonMesh& mesh, const std::vector<std::pair<float, float>>& uvCoordinates);
    
    // Format-specific helpers
    bool writeOBJFiles(const pcl::PolygonMesh& mesh, const std::string& basePath, const std::string& modelName);
    bool writePLYFile(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName);
    bool writeFBXFile(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName);
}; 