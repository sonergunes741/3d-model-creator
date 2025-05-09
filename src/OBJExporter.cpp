#include "OBJExporter.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/common/common.h>  // getMinMax3D için
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

OBJExporter::OBJExporter() 
    : useVertexColors(true),
      textureWidth(1024),
      textureHeight(1024) {
}

bool OBJExporter::exportMesh(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName) {
    // Çıktı dizinini oluştur
    fs::path outputDir = fs::path(outputPath).parent_path();
    try {
        if (!fs::exists(outputDir)) {
            fs::create_directories(outputDir);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Klasör oluşturma hatası: " << e.what() << std::endl;
        return false;
    }

    // Dosya yollarını hazırla
    std::string basePath = outputPath;
    // .obj, .mtl, .png uzantılarını kaldır
    if (basePath.size() >= 4 && basePath.substr(basePath.size() - 4) == ".obj") {
        basePath = basePath.substr(0, basePath.size() - 4);
    }

    std::string objFilePath = basePath + ".obj";
    std::string mtlFileName = fs::path(basePath).filename().string();
    std::string mtlFilePath = basePath + ".mtl";
    std::string textureFileName = mtlFileName;
    std::string textureFilePath = basePath + ".png";

    // MTL dosyasını oluştur
    bool mtlSuccess = writeMTLFile(mtlFilePath, modelName + "_material", textureFileName + ".png");
    if (!mtlSuccess) {
        std::cerr << "MTL dosyası oluşturma hatası!" << std::endl;
        return false;
    }

    // Texture dosyasını oluştur
    bool textureSuccess = writeTextureFile(mesh, textureFilePath);
    if (!textureSuccess) {
        std::cerr << "Texture dosyası oluşturma hatası!" << std::endl;
        return false;
    }

    // OBJ dosyasını oluştur
    bool objSuccess = writeOBJFile(mesh, objFilePath, mtlFileName, textureFileName);
    if (!objSuccess) {
        std::cerr << "OBJ dosyası oluşturma hatası!" << std::endl;
        return false;
    }

    std::cout << "Model başarıyla OBJ+MTL+PNG formatında dışa aktarıldı: " << objFilePath << std::endl;
    return true;
}

bool OBJExporter::writeOBJFile(
    const pcl::PolygonMesh& mesh, 
    const std::string& objFilePath,
    const std::string& mtlFileName,
    const std::string& textureFileName) {
    
    // Nokta bulutu çıkar
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud = extractColors(mesh);
    
    // UV koordinatlarını oluştur
    std::vector<std::pair<float, float>> uvCoordinates = generateUVCoordinates(mesh);
    
    std::ofstream objFile(objFilePath);
    if (!objFile.is_open()) {
        std::cerr << "OBJ dosyası açılamadı: " << objFilePath << std::endl;
        return false;
    }
    
    // OBJ başlığı
    objFile << "# OBJ file created by 3D Model Creator" << std::endl;
    objFile << "mtllib " << mtlFileName << ".mtl" << std::endl;
    objFile << "o " << fs::path(objFilePath).filename().string() << std::endl;
    
    // Vertex pozisyonları
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const pcl::PointXYZRGB& p = cloud->points[i];
        objFile << "v " << p.x << " " << p.y << " " << p.z;
        if (useVertexColors) {
            objFile << " " << p.r / 255.0f << " " << p.g / 255.0f << " " << p.b / 255.0f;
        }
        objFile << std::endl;
    }
    
    // Texture koordinatları
    for (const auto& uv : uvCoordinates) {
        objFile << "vt " << uv.first << " " << uv.second << std::endl;
    }
    
    // Normal vektörleri (basit olarak Z+ yönünde)
    objFile << "vn 0.0 0.0 1.0" << std::endl;
    
    // Materyal kullan
    objFile << "usemtl " << fs::path(mtlFileName).filename().string() << "_material" << std::endl;
    
    // Yüzleri yaz
    for (size_t i = 0; i < mesh.polygons.size(); i++) {
        const pcl::Vertices& polygon = mesh.polygons[i];
        objFile << "f";
        
        for (size_t j = 0; j < polygon.vertices.size(); j++) {
            size_t vertexIndex = polygon.vertices[j] + 1; // OBJ 1-indexed
            // Vertex/Texture/Normal indeksi
            objFile << " " << vertexIndex << "/" << vertexIndex << "/1";
        }
        
        objFile << std::endl;
    }
    
    objFile.close();
    std::cout << "OBJ dosyası oluşturuldu: " << objFilePath << std::endl;
    
    return true;
}

bool OBJExporter::writeMTLFile(
    const std::string& mtlFilePath,
    const std::string& materialName,
    const std::string& textureFileName) {
    
    std::ofstream mtlFile(mtlFilePath);
    if (!mtlFile.is_open()) {
        std::cerr << "MTL dosyası açılamadı: " << mtlFilePath << std::endl;
        return false;
    }
    
    // MTL içeriği
    mtlFile << "# MTL file created by 3D Model Creator" << std::endl;
    mtlFile << "newmtl " << materialName << std::endl;
    mtlFile << "Ka 1.000 1.000 1.000" << std::endl;  // Ambient
    mtlFile << "Kd 1.000 1.000 1.000" << std::endl;  // Diffuse
    mtlFile << "Ks 0.000 0.000 0.000" << std::endl;  // Specular
    mtlFile << "Ns 10.0" << std::endl;                // Specular exponent
    mtlFile << "d 1.0" << std::endl;                  // Opacity
    mtlFile << "illum 2" << std::endl;                // Illumination model
    mtlFile << "map_Kd " << textureFileName << std::endl;  // Diffuse texture
    
    mtlFile.close();
    std::cout << "MTL dosyası oluşturuldu: " << mtlFilePath << std::endl;
    
    return true;
}

bool OBJExporter::writeTextureFile(
    const pcl::PolygonMesh& mesh,
    const std::string& textureFilePath) {
    
    // UV koordinatlarını oluştur
    std::vector<std::pair<float, float>> uvCoordinates = generateUVCoordinates(mesh);
    
    // Texture görüntüsünü oluştur
    cv::Mat texture = createTextureImage(mesh, uvCoordinates);
    
    // PNG olarak kaydet
    bool success = cv::imwrite(textureFilePath, texture);
    
    if (success) {
        std::cout << "Texture dosyası oluşturuldu: " << textureFilePath << std::endl;
    } else {
        std::cerr << "Texture dosyası kaydedilemedi!" << std::endl;
    }
    
    return success;
}

std::vector<std::pair<float, float>> OBJExporter::generateUVCoordinates(const pcl::PolygonMesh& mesh) {
    // Nokta bulutu çıkar
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh.cloud, *cloud);
    
    // Nesnenin bounding box'ını hesapla
    pcl::PointXYZ min_pt, max_pt;
    pcl::getMinMax3D(*cloud, min_pt, max_pt);
    
    // UV koordinatlarını hesapla (silindirik haritalama)
    std::vector<std::pair<float, float>> uvCoordinates;
    
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const pcl::PointXYZ& point = cloud->points[i];
        
        // Silindirik haritalama
        // U koordinatı için açıyı kullan
        float angle = std::atan2(point.z, point.x);
        float u = (angle + M_PI) / (2.0f * M_PI);
        
        // V koordinatı için normalize edilmiş yükseklik kullan
        float v = (point.y - min_pt.y) / (max_pt.y - min_pt.y);
        
        // Aralığı 0-1 arasında tutyoğa dikkat et
        u = std::min(1.0f, std::max(0.0f, u));
        v = std::min(1.0f, std::max(0.0f, v));
        
        uvCoordinates.push_back(std::make_pair(u, v));
    }
    
    return uvCoordinates;
}

pcl::PointCloud<pcl::PointXYZRGB>::Ptr OBJExporter::extractColors(const pcl::PolygonMesh& mesh) {
    // Mesh'ten renk bilgisini çıkar
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr colorCloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(mesh.cloud, *colorCloud);
    
    return colorCloud;
}

cv::Mat OBJExporter::createTextureImage(
    const pcl::PolygonMesh& mesh,
    const std::vector<std::pair<float, float>>& uvCoordinates) {
    
    // Nokta renklerini çıkar
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr colorCloud = extractColors(mesh);
    
    // Texture görüntüsünü oluştur
    cv::Mat texture = cv::Mat::zeros(textureHeight, textureWidth, CV_8UC3);
    
    // Her bir vertex için
    for (size_t i = 0; i < colorCloud->points.size(); i++) {
        if (i >= uvCoordinates.size()) continue;
        
        const pcl::PointXYZRGB& point = colorCloud->points[i];
        const std::pair<float, float>& uv = uvCoordinates[i];
        
        // UV koordinatlarını piksel konumuna dönüştür
        int x = static_cast<int>(uv.first * (textureWidth - 1));
        int y = static_cast<int>((1.0f - uv.second) * (textureHeight - 1)); // Y tersine çevrilir
        
        // Geçerli konumları kontrol et
        if (x >= 0 && x < textureWidth && y >= 0 && y < textureHeight) {
            // Rengi ayarla (BGR formatında)
            texture.at<cv::Vec3b>(y, x) = cv::Vec3b(point.b, point.g, point.r);
        }
    }
    
    // Doldurulmamış pikselleri interpole et - DÜZELTILMIŞ KISIM
    // 3 kanallı görüntüden tek kanallı maske oluştur
    cv::Mat emptyPixelsMask = cv::Mat::zeros(textureHeight, textureWidth, CV_8UC1);
    
    // Boş pikselleri tespit et (siyah olanlar)
    for (int y = 0; y < textureHeight; y++) {
        for (int x = 0; x < textureWidth; x++) {
            cv::Vec3b pixel = texture.at<cv::Vec3b>(y, x);
            if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) {
                emptyPixelsMask.at<uchar>(y, x) = 255;  // Boş piksel
            }
        }
    }
    
    // Basit bir yayılma algoritması
    for (int iterations = 0; iterations < 10; iterations++) {
        cv::Mat temp = texture.clone();
        
        for (int y = 1; y < textureHeight - 1; y++) {
            for (int x = 1; x < textureWidth - 1; x++) {
                if (emptyPixelsMask.at<uchar>(y, x) > 0) {
                    // 4 komşudaki renklerin ortalamasını al
                    cv::Vec3b sum(0, 0, 0);
                    int count = 0;
                    
                    for (int dy = -1; dy <= 1; dy += 2) {
                        if (y + dy >= 0 && y + dy < textureHeight && emptyPixelsMask.at<uchar>(y + dy, x) == 0) {
                            sum += texture.at<cv::Vec3b>(y + dy, x);
                            count++;
                        }
                    }
                    
                    for (int dx = -1; dx <= 1; dx += 2) {
                        if (x + dx >= 0 && x + dx < textureWidth && emptyPixelsMask.at<uchar>(y, x + dx) == 0) {
                            sum += texture.at<cv::Vec3b>(y, x + dx);
                            count++;
                        }
                    }
                    
                    if (count > 0) {
                        temp.at<cv::Vec3b>(y, x) = sum / count;
                        emptyPixelsMask.at<uchar>(y, x) = 0; // Bu pikseli doldurduk
                    }
                }
            }
        }
        
        texture = temp;
    }
    
    // Texture'ı yumuşat
    cv::GaussianBlur(texture, texture, cv::Size(3, 3), 0.5);
    
    return texture;
}