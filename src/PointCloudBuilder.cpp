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
    
    // Scanner approach parameters
    rotationCenterX = 0;
    useCustomRotationCenter = false;
}

// Constructor with camera parameters
PointCloudBuilder::PointCloudBuilder(const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs)
    : cameraMatrix(cameraMatrix.clone()), 
      distCoeffs(distCoeffs.clone()),
      scanRadius(50.0f), // Bardak için daha küçük bir yarıçap
      scanCenterX(0.0f), 
      scanCenterY(0.0f), 
      scanCenterZ(0.0f),
      rotationCenterX(0),
      useCustomRotationCenter(false) {
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
    
    // Convert angle to radians
    float angleRad = angle * M_PI / 180.0f;
    
    // Scanner approach: Use cylindrical coordinates with proper height and distance calculation
    // This is more accurate than the previous approach
    
    // Calculate height (H) - relative to bottom of image
    // In scanner, bottomR represents the bottom reference point
    // For simplicity, we'll use the bottom of the image as reference
    int bottomR = imageHeight - 1;  // Bottom of image
    double H = point.y - bottomR;   // Height relative to bottom (negative values = above bottom)
    
    // Calculate distance from rotation center
    // In scanner, centerC is determined from the first image's laser line
    // For now, we'll use a reasonable estimate or make it configurable
    int centerC = useCustomRotationCenter ? rotationCenterX : imageWidth / 2;  // Use custom center if available
    double dist = point.x - centerC;  // Distance from center (can be negative)
    
    // Convert to cylindrical coordinates using scanner's getVertex function logic
    // Scanner's getVertex: x = d * cos(t), y = d * sin(t), z = H
    double x = dist * std::cos(angleRad);
    double y = dist * std::sin(angleRad);
    double z = H;
    
    // Apply scaling and centering based on scan parameters
    float x_world = scanCenterX + x * (scanRadius / 100.0f);  // Scale by radius
    float y_world = scanCenterY + y * (scanRadius / 100.0f);
    float z_world = scanCenterZ + z * (scanRadius / 100.0f);
    
    // Create PCL point
    pcl::PointXYZRGB p3d;
    p3d.x = x_world;
    p3d.y = y_world;
    p3d.z = z_world;
    p3d.r = 255; // Default color, will be overwritten by ColorMapper
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

void PointCloudBuilder::setRotationCenterX(int centerX) {
    rotationCenterX = centerX;
    useCustomRotationCenter = true;
}

void PointCloudBuilder::determineRotationCenterFromFirstImage(const std::vector<cv::Point>& laserLine, int imageWidth) {
    if (laserLine.empty()) {
        // Fallback to image center
        rotationCenterX = imageWidth / 2;
        useCustomRotationCenter = true;
        return;
    }
    
    // Scanner approach: Find the rightmost point and add offset
    int maxX = 0;
    for (const auto& point : laserLine) {
        if (point.x > maxX) {
            maxX = point.x;
        }
    }
    
    // Add offset like in scanner (centerC = cIndex + 40)
    rotationCenterX = maxX + 40;
    
    // Ensure it's within image bounds
    if (rotationCenterX >= imageWidth) {
        rotationCenterX = imageWidth - 1;
    }
    
    useCustomRotationCenter = true;
    
    std::cout << "Rotasyon merkezi belirlendi: " << rotationCenterX << std::endl;
}

void PointCloudBuilder::applyScannerStyleFiltering(int verticalPrecision) {
    if (!cloud || cloud->empty()) {
        std::cout << "UYARI: Nokta bulutu boş, filtreleme yapılamıyor!" << std::endl;
        return;
    }
    
    if (verticalPrecision >= 100) {
        std::cout << "Dikey hassasiyet %100, filtreleme yapılmıyor." << std::endl;
        return;
    }
    
    std::cout << "Scanner yaklaşımı ile nokta filtreleme uygulanıyor..." << std::endl;
    std::cout << "Filtreleme öncesi nokta sayısı: " << cloud->points.size() << std::endl;
    
    // Calculate how many points to keep based on vertical precision
    int itemsToKeep = static_cast<int>(cloud->points.size() * (verticalPrecision / 100.0));
    itemsToKeep = std::max(itemsToKeep, 1);  // At least keep 1 point
    
    if (itemsToKeep >= cloud->points.size()) {
        std::cout << "Filtreleme gerekmiyor, tüm noktalar korunuyor." << std::endl;
        return;
    }
    
    // Calculate step size for uniform sampling
    double stepSize = static_cast<double>(cloud->points.size()) / itemsToKeep;
    
    // Create new filtered point cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr filteredCloud(new pcl::PointCloud<pcl::PointXYZRGB>());
    filteredCloud->reserve(itemsToKeep);
    
    // Sample points uniformly
    for (double i = 0; i < cloud->points.size(); i += stepSize) {
        int index = static_cast<int>(i);
        if (index < cloud->points.size()) {
            filteredCloud->points.push_back(cloud->points[index]);
        }
    }
    
    // Update the cloud
    *cloud = *filteredCloud;
    cloud->width = cloud->points.size();
    cloud->height = 1;
    cloud->is_dense = false;
    
    std::cout << "Filtreleme sonrası nokta sayısı: " << cloud->points.size() << std::endl;
}