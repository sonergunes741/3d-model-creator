#include "PointCloudBuilder.h"

#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/voxel_grid.h>
#include <cmath>

PointCloudBuilder::PointCloudBuilder(const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs)
    : cameraMatrix(cameraMatrix.clone()), 
      distCoeffs(distCoeffs.clone()),
      scanRadius(200.0f), 
      scanCenterX(0.0f), 
      scanCenterY(0.0f), 
      scanCenterZ(0.0f) {
    // Yeni boş nokta bulutu oluştur
    cloud.reset(new pcl::PointCloud<pcl::PointXYZRGB>());
}

void PointCloudBuilder::addLineToCloud(
    const std::vector<cv::Point>& laserLine, 
    float angle, 
    int imageWidth, 
    int imageHeight) {
    
    if (laserLine.empty()) {
        return;
    }

    // Her bir lazer çizgisi noktası için
    for (const auto& point : laserLine) {
        // Görüntü noktasını 3D'ye dönüştür
        pcl::PointXYZRGB p3d = projectPointTo3D(point, angle, imageWidth, imageHeight);
        
        // Nokta bulutuna ekle
        cloud->points.push_back(p3d);
    }
}

pcl::PointXYZRGB PointCloudBuilder::projectPointTo3D(
    const cv::Point& point, 
    float angle, 
    int imageWidth, 
    int imageHeight) {
    
    // Açıyı radyana çevir
    float angleRad = angle * M_PI / 180.0f;
    
    // Görüntü merkezine göre normalize et
    float normalizedX = (point.x - imageWidth / 2.0f) / (imageWidth / 2.0f);
    float normalizedY = (point.y - imageHeight / 2.0f) / (imageHeight / 2.0f);
    
    // Basit bir projeksiyon modeli kullan
    // Gerçek bir uygulamada kamera kalibrasyonu kullanılmalı
    float scale = scanRadius;
    
    // Silindirik koordinatlardan Kartezyen koordinatlara dönüşüm
    float x = scanCenterX + scale * std::cos(angleRad);
    float y = scanCenterY + normalizedY * scale;
    float z = scanCenterZ + scale * std::sin(angleRad);
    
    // PCL nokta oluştur (şimdilik renksiz)
    pcl::PointXYZRGB p3d;
    p3d.x = x;
    p3d.y = y;
    p3d.z = z;
    p3d.r = 255; // Şimdilik sabit renk
    p3d.g = 255;
    p3d.b = 255;
    
    return p3d;
}

void PointCloudBuilder::processPointCloud() {
    if (cloud->empty()) {
        return;
    }
    
    // Nokta bulutu organize değil
    cloud->is_dense = false;
    cloud->width = cloud->points.size();
    cloud->height = 1;
    
    // Voksel ızgara tabanlı alt örnekleme
    pcl::VoxelGrid<pcl::PointXYZRGB> voxelGrid;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloudFiltered(new pcl::PointCloud<pcl::PointXYZRGB>);
    
    voxelGrid.setInputCloud(cloud);
    voxelGrid.setLeafSize(1.0f, 1.0f, 1.0f);  // 1mm voksel boyutu
    voxelGrid.filter(*cloudFiltered);
    
    // İstatistiksel aykırı değer temizleme
    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;
    sor.setInputCloud(cloudFiltered);
    sor.setMeanK(50);
    sor.setStddevMulThresh(1.0);
    sor.filter(*cloud);
    
    std::cout << "Nokta bulutu işlendi. Toplam nokta sayısı: " << cloud->points.size() << std::endl;
}

void PointCloudBuilder::setCameraParameters(const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs) {
    this->cameraMatrix = cameraMatrix.clone();
    this->distCoeffs = distCoeffs.clone();
}

void PointCloudBuilder::setScanParameters(float radius, float centerX, float centerY, float centerZ) {
    scanRadius = radius;
    scanCenterX = centerX;
    scanCenterY = centerY;
    scanCenterZ = centerZ;
}