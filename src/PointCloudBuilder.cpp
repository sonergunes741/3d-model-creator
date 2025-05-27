#include "PointCloudBuilder.h"

#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/voxel_grid.h>
#include <cmath>
#include <iostream>

// Default constructor that initializes the cloud
PointCloudBuilder::PointCloudBuilder() {
    // Initialize an empty point cloud
    cloud.reset(new pcl::PointCloud<pcl::PointXYZRGB>());
    
    // Set default scanning parameters
    scanRadius = 50.0f;
    scanCenterX = 0.0f;
    scanCenterY = 0.0f;
    scanCenterZ = 0.0f;
}

// Constructor with camera parameters
PointCloudBuilder::PointCloudBuilder(const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs)
    : cameraMatrix(cameraMatrix.clone()), 
      distCoeffs(distCoeffs.clone()),
      scanRadius(50.0f), // Bardak için daha küçük bir yarıçap
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
    
    // Make sure the cloud is initialized
    if (!cloud) {
        cloud.reset(new pcl::PointCloud<pcl::PointXYZRGB>());
    }

    // Her bir lazer çizgisi noktası için
    for (const auto& point : laserLine) {
        // Görüntü noktasını 3D'ye dönüştür
        pcl::PointXYZRGB p3d = projectPointTo3D(point, angle, imageWidth, imageHeight);
        
        // Make sure the point has valid coordinates
        if (std::isfinite(p3d.x) && std::isfinite(p3d.y) && std::isfinite(p3d.z)) {
            // Nokta bulutuna ekle
            cloud->points.push_back(p3d);
            // Store image info for color mapping
            PointWithImageInfo info;
            info.point = p3d;
            info.angle = angle;
            info.imageX = point.x;
            info.imageY = point.y;
            // imageIndex is the scan index, which can be derived from angle if needed
            // For now, set to -1 (will be set in main loop if needed)
            info.imageIndex = -1;
            pointImageInfo.push_back(info);
        }
    }
}

pcl::PointXYZRGB PointCloudBuilder::projectPointTo3D(
    const cv::Point& point, 
    float angle, 
    int imageWidth, 
    int imageHeight) {
    
    // Açıyı radyana çevir
    float angleRad = angle * M_PI / 180.0f;
    
    // Görüntü koordinatlarını normalize et ve merkezi sıfır yap
    float centerX = imageWidth / 2.0f;  // Görüntü merkezi
    float centerY = imageHeight / 2.0f;
    
    // Merkeze göre normalize edilmiş koordinatlar (-0.5 ile +0.5 arası)
    float normalizedX = (point.x - centerX) / imageWidth;  // Merkeze göre mesafe
    float normalizedY = (point.y - centerY) / imageHeight; // Merkeze göre yükseklik
    
    // Silindirik koordinatlara dönüştür
    float radius = normalizedX * scanRadius;  // Negatif olabilir (merkez çizgisinin sol tarafı)
    float height = -normalizedY * scanRadius; // Y ekseni ters (görüntüde yukarı = negatif Z)
    
    // 3D koordinatları hesapla (silindirik koordinat sistemi)
    // Y ekseni dikey (yükseklik), X ve Z eksenleri yatay (dairesel hareket)
    float x = radius * cos(angleRad) + scanCenterX;
    float y = height + scanCenterY;  // Y ekseni dikey (yükseklik)
    float z = radius * sin(angleRad) + scanCenterZ;  // Z ekseni yatay
    
    // PCL noktası oluştur
    pcl::PointXYZRGB p3d;
    p3d.x = x;
    p3d.y = y;
    p3d.z = z;
    p3d.r = 255; // Varsayılan renk, ColorMapper tarafından değiştirilecek
    p3d.g = 255;
    p3d.b = 255;
    
    return p3d;
}

void PointCloudBuilder::processPointCloud() {
    if (!cloud || cloud->empty()) {
        std::cerr << "HATA: Boş nokta bulutu!" << std::endl;
        return;
    }
    
    // Nokta bulutu organize değil
    cloud->is_dense = false;
    cloud->width = cloud->points.size();
    cloud->height = 1;
    
    std::cout << "İşlenmemiş nokta bulutu boyutu: " << cloud->points.size() << std::endl;
    
    // ÖNEMLI: Çok az nokta varsa filtreleme yapma
    if (cloud->points.size() < 100) {
        std::cout << "UYARI: Çok az nokta var, filtreleme yapılmıyor!" << std::endl;
        return;
    }
    
    // AŞAMA 1: Voxel tabanlı alt örnekleme - devre dışı bırakıldı
    // pcl::VoxelGrid<pcl::PointXYZRGB> voxelGrid;
    // pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloudFiltered(new pcl::PointCloud<pcl::PointXYZRGB>);
    // voxelGrid.setInputCloud(cloud);
    // voxelGrid.setLeafSize(0.1f, 0.1f, 0.1f);  // 0.1mm voksel boyutu - neredeyse hiç filtreleme yok
    // voxelGrid.filter(*cloudFiltered);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloudFiltered = cloud; // Voxel grid disabled, use original
    
    std::cout << "İşlenmemiş nokta bulutu boyutu: " << cloudFiltered->points.size() << std::endl;
    
    // Çok az nokta kaldıysa, filtrelemeden önceki nokta bulutunu kullan
    if (cloudFiltered->points.size() < 50) {
        std::cout << "UYARI: Voxel filtreleme çok fazla nokta kaldırdı! Orijinal nokta bulutu kullanılıyor." << std::endl;
        return;
    }
    
    // AŞAMA 2: İstatistiksel aykırı değer temizleme - minimum filtreleme
    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloudOutlierRemoved(new pcl::PointCloud<pcl::PointXYZRGB>);
    
    sor.setInputCloud(cloudFiltered);
    sor.setMeanK(50);  // 50 komşu - daha fazla komşu kullan
    sor.setStddevMulThresh(5.0);  // 5.0 standart sapma - minimum filtreleme
    sor.filter(*cloudOutlierRemoved);
    
    std::cout << "Aykırı değer temizlemeden sonra nokta sayısı: " << cloudOutlierRemoved->points.size() << std::endl;
    
    // Çok az nokta kaldıysa, sadece voxel filtrelemesi kullan
    if (cloudOutlierRemoved->points.size() < 50) {
        std::cout << "UYARI: Aykırı değer temizleme çok fazla nokta kaldırdı! Sadece voxel filtrelemesi kullanılıyor." << std::endl;
        *cloud = *cloudFiltered;
    } else {
        *cloud = *cloudOutlierRemoved;
    }
    
    std::cout << "Final nokta bulutu boyutu: " << cloud->points.size() << std::endl;
}

// Get the point cloud (added getter method)
pcl::PointCloud<pcl::PointXYZRGB>::Ptr PointCloudBuilder::getCloud() {
    if (!cloud) {
        cloud.reset(new pcl::PointCloud<pcl::PointXYZRGB>());
    }
    return cloud;
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