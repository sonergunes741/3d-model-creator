#pragma once

#include <pcl/PolygonMesh.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

/**
 * @brief OBJ format 3D model exporter
 * 
 * Exports 3D models in OBJ format with MTL and PNG texture files
 */
class MeshExporter {
public:
    enum class ExportFormat {
        OBJ    // OBJ + MTL + PNG
    };

    /**
     * @brief Constructor
     */
    MeshExporter();
    
    /**
     * @brief Export mesh to OBJ format
     * 
     * @param mesh PCL PolygonMesh
     * @param outputPath Output file path (without extension)
     * @param format Export format (only OBJ supported)
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
    
    // Export methods for OBJ format
    bool exportOBJ(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName);
    
    // Helper methods
    std::vector<std::pair<float, float>> generateUVCoordinates(const pcl::PolygonMesh& mesh);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr extractColors(const pcl::PolygonMesh& mesh);
    cv::Mat createTextureImage(const pcl::PolygonMesh& mesh, const std::vector<std::pair<float, float>>& uvCoordinates);
    
    // Format-specific helpers
    bool writeOBJFiles(const pcl::PolygonMesh& mesh, const std::string& basePath, const std::string& modelName);
}; 