#include "ColorMapper.h"

#include <pcl/io/vtk_lib_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/common/transforms.h>
#include <cmath>
#include <iostream>

ColorMapper::ColorMapper() {
}

void ColorMapper::addColorData(const cv::Mat& colorImage, float angle, int imageWidth, int imageHeight) {
    // Merkez çizgideki renk verilerini çıkar
    std::vector<cv::Vec3b> centerColors = extractCenterLineColors(colorImage, imageWidth);
    
    // Renkli dikey çizgiyi oluştur ve sakla
    cv::Mat centerLine(imageHeight, 1, CV_8UC3);
    for (int y = 0; y < imageHeight; y++) {
        if (y < centerColors.size()) {
            centerLine.at<cv::Vec3b>(y, 0) = centerColors[y];
        } else {
            centerLine.at<cv::Vec3b>(y, 0) = cv::Vec3b(0, 0, 0);
        }
    }
    
    // Renk verisi ve açıyı kaydet
    colorData.push_back(centerLine);
    angles.push_back(angle);
    
    std::cout << "Açı " << angle << " için renk verisi eklendi." << std::endl;
}

std::vector<cv::Vec3b> ColorMapper::extractCenterLineColors(const cv::Mat& colorImage, int imageWidth) {
    std::vector<cv::Vec3b> centerColors;
    
    // Görüntünün merkez sütununu al
    int centerX = imageWidth / 2;
    
    for (int y = 0; y < colorImage.rows; y++) {
        centerColors.push_back(colorImage.at<cv::Vec3b>(y, centerX));
    }
    
    return centerColors;
}

void ColorMapper::applyColorToMesh(pcl::PolygonMesh& mesh, pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud) {
    if (colorData.empty() || angles.empty()) {
        std::cout << "Renk verisi bulunamadı!" << std::endl;
        return;
    }
    
    // Mesh'ten nokta bulutu çıkar
    pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh.cloud, *meshCloud);
    
    // Vertex renklerini saklayacak nokta bulutu
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr coloredCloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    coloredCloud->points.resize(meshCloud->points.size());
    
    // Her bir mesh noktasına renk ata
    for (size_t i = 0; i < meshCloud->points.size(); i++) {
        const pcl::PointXYZ& point = meshCloud->points[i];
        
        // Noktanın silindirik açısını hesapla
        float pointAngle = calculateAngle(point);
        
        // En yakın açıyı bul
        int closestAngleIdx = -1;
        float minDiff = 360.0f;
        
        for (size_t j = 0; j < angles.size(); j++) {
            float diff = angleDifference(pointAngle, angles[j]);
            if (diff < minDiff) {
                minDiff = diff;
                closestAngleIdx = j;
            }
        }
        
        // Renk verisi varsa uygula
        if (closestAngleIdx >= 0) {
            // Y konumuna göre renk seç
            // Burada çok basit bir yaklaşım kullanılıyor
            // Gerçek uygulamada daha karmaşık bir eşleme olacaktır
            cv::Mat& centerLine = colorData[closestAngleIdx];
            
            // Y koordinatını normalize et ve merkez çizgi yüksekliğine dönüştür
            float normalizedY = (point.y - cloud->points[0].y) / 
                                (cloud->points.back().y - cloud->points[0].y);
            int colorY = std::min(std::max(0, (int)(normalizedY * centerLine.rows)), centerLine.rows - 1);
            
            // Renk bilgisini al
            cv::Vec3b color = centerLine.at<cv::Vec3b>(colorY, 0);
            
            // RGB noktası oluştur
            pcl::PointXYZRGB coloredPoint;
            coloredPoint.x = point.x;
            coloredPoint.y = point.y;
            coloredPoint.z = point.z;
            coloredPoint.r = color[2]; // OpenCV: BGR, PCL: RGB
            coloredPoint.g = color[1];
            coloredPoint.b = color[0];
            
            coloredCloud->points[i] = coloredPoint;
        } else {
            // Renk bulunamazsa varsayılan beyaz kullan
            pcl::PointXYZRGB coloredPoint;
            coloredPoint.x = point.x;
            coloredPoint.y = point.y;
            coloredPoint.z = point.z;
            coloredPoint.r = 255;
            coloredPoint.g = 255;
            coloredPoint.b = 255;
            
            coloredCloud->points[i] = coloredPoint;
        }
    }
    
    // Renkli nokta bulutunu mesh'e dönüştür
    pcl::toPCLPointCloud2(*coloredCloud, mesh.cloud);
    
    std::cout << "Mesh renklendirme tamamlandı." << std::endl;
}

float ColorMapper::calculateAngle(const pcl::PointXYZ& point) {
    // XZ düzlemindeki açıyı hesapla
    float angle = std::atan2(point.z, point.x) * 180.0f / M_PI;
    
    // Açıyı 0-360 aralığına getir
    if (angle < 0) {
        angle += 360.0f;
    }
    
    return angle;
}

float ColorMapper::angleDifference(float angle1, float angle2) {
    // İki açı arasındaki farkı 0-180 aralığında hesapla
    float diff = std::fabs(angle1 - angle2);
    while (diff > 180.0f) {
        diff = std::fabs(diff - 360.0f);
    }
    
    return diff;
}