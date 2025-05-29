#include "MeshExporter.h"
#include <pcl/io/pcd_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/common/common.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

MeshExporter::MeshExporter() 
    : useVertexColors(true),
      textureWidth(1024),
      textureHeight(1024) {
}

bool MeshExporter::exportMesh(
    const pcl::PolygonMesh& mesh, 
    const std::string& outputPath, 
    ExportFormat format,
    const std::string& modelName) {
    
    if (format != ExportFormat::OBJ) {
        std::cerr << "Only OBJ format is supported!" << std::endl;
        return false;
    }
    
    return exportOBJ(mesh, outputPath, modelName);
}

bool MeshExporter::exportOBJ(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName) {
    return writeOBJFiles(mesh, outputPath, modelName);
}

bool MeshExporter::writeOBJFiles(const pcl::PolygonMesh& mesh, const std::string& basePath, const std::string& modelName) {
    // Use the existing OBJ export logic
    std::string objPath = basePath + ".obj";
    std::string mtlPath = basePath + ".mtl";
    std::string texturePath = basePath + ".png";
    
    // Extract point cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(mesh.cloud, *cloud);
    
    // Generate UV coordinates
    auto uvCoordinates = generateUVCoordinates(mesh);
    
    // Write OBJ file
    std::ofstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "Cannot create OBJ file: " << objPath << std::endl;
        return false;
    }
    
    objFile << "# OBJ file created by 3D Model Creator" << std::endl;
    objFile << "# Optimized for Unreal Engine import" << std::endl;
    objFile << "# Contains: vertices, texture coordinates, normals, and faces" << std::endl;
    objFile << "# Only OBJ file is generated (no MTL or PNG)" << std::endl;
    objFile << std::endl;
    
    // Write vertices
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const auto& p = cloud->points[i];
        objFile << "v " << p.x << " " << p.y << " " << p.z;
        if (useVertexColors) {
            objFile << " " << p.r / 255.0f << " " << p.g / 255.0f << " " << p.b / 255.0f;
        }
        objFile << std::endl;
    }
    
    // Write normals
    for (size_t i = 0; i < cloud->points.size(); i++) {
        objFile << "vn 0.0 1.0 0.0" << std::endl;
    }
    
    // Write faces
    for (const auto& polygon : mesh.polygons) {
        if (polygon.vertices.size() >= 3) {
            objFile << "f";
            for (size_t j = 0; j < polygon.vertices.size(); j++) {
                size_t vertexIndex = polygon.vertices[j] + 1;
                objFile << " " << vertexIndex << "//" << vertexIndex;
            }
            objFile << std::endl;
        }
    }
    
    objFile.close();
    
    std::cout << "OBJ file created: " << objPath << std::endl;
    return true;
}

std::vector<std::pair<float, float>> MeshExporter::generateUVCoordinates(const pcl::PolygonMesh& mesh) {
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh.cloud, *cloud);
    
    pcl::PointXYZ min_pt, max_pt;
    pcl::getMinMax3D(*cloud, min_pt, max_pt);
    
    std::vector<std::pair<float, float>> uvCoordinates;
    
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const pcl::PointXYZ& point = cloud->points[i];
        
        float angle = std::atan2(point.z, point.x);
        float u = (angle + M_PI) / (2.0f * M_PI);
        
        float v = 0.5f;
        float height_range = max_pt.y - min_pt.y;
        if (height_range > 0.0001f) {
            v = (point.y - min_pt.y) / height_range;
        }
        
        u = std::min(1.0f, std::max(0.0f, u));
        v = std::min(1.0f, std::max(0.0f, v));
        
        uvCoordinates.push_back(std::make_pair(u, v));
    }
    
    return uvCoordinates;
}

pcl::PointCloud<pcl::PointXYZRGB>::Ptr MeshExporter::extractColors(const pcl::PolygonMesh& mesh) {
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr colorCloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(mesh.cloud, *colorCloud);
    
    if (colorCloud->empty()) {
        std::cerr << "WARNING: No color information found in mesh." << std::endl;
        colorCloud->points.resize(1);
        colorCloud->points[0].r = 255;
        colorCloud->points[0].g = 255;
        colorCloud->points[0].b = 255;
    }
    
    return colorCloud;
}

cv::Mat MeshExporter::createTextureImage(const pcl::PolygonMesh& mesh, const std::vector<std::pair<float, float>>& uvCoordinates) {
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr colorCloud = extractColors(mesh);
    cv::Mat texture = cv::Mat::zeros(textureHeight, textureWidth, CV_8UC3);
    
    if (colorCloud->empty() || uvCoordinates.empty()) {
        texture = cv::Mat(textureHeight, textureWidth, CV_8UC3, cv::Scalar(255, 255, 255));
        return texture;
    }
    
    for (size_t i = 0; i < colorCloud->points.size() && i < uvCoordinates.size(); i++) {
        const pcl::PointXYZRGB& point = colorCloud->points[i];
        const std::pair<float, float>& uv = uvCoordinates[i];
        
        int x = static_cast<int>(uv.first * (textureWidth - 1));
        int y = static_cast<int>((1.0f - uv.second) * (textureHeight - 1));
        
        if (x >= 0 && x < textureWidth && y >= 0 && y < textureHeight) {
            texture.at<cv::Vec3b>(y, x) = cv::Vec3b(point.b, point.g, point.r);
        }
    }
    
    return texture;
} 