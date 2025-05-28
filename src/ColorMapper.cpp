#include "ColorMapper.h"

#include <pcl/io/vtk_lib_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/common/transforms.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <pcl/kdtree/kdtree_flann.h>

ColorMapper::ColorMapper() {
}

void ColorMapper::addColorData(const std::string& imagePath, float angle) {
    // Store the image path instead of loading the image
    colorImagePaths.push_back(imagePath);
    angles.push_back(angle);

    static bool firstLog = true;
    static bool lastLog = false;
    if (firstLog) {
        std::cout << "Renk verileri kaydediliyor - İlk açı: " << angle << "°" << std::endl;
        firstLog = false;
    }
    if (angle > 355.0f && !lastLog) {
        std::cout << "Renk verileri kaydediliyor - Son açı: " << angle << "°" << std::endl;
        lastLog = true;
    }
}

void ColorMapper::addColorDataLegacy(const cv::Mat& colorImage, float angle) {
    // Legacy function for backward compatibility - not recommended for large datasets
    std::cout << "Uyarı: Legacy addColorData kullanılıyor - büyük veri setleri için önerilmez!" << std::endl;
    
    // For legacy support, we would need to save the image temporarily
    // This is not implemented to avoid memory issues
    std::cout << "Legacy mod desteklenmiyor - lütfen dosya yolu tabanlı addColorData kullanın" << std::endl;
}

cv::Mat ColorMapper::loadImageAtIndex(size_t index) const {
    if (index >= colorImagePaths.size()) {
        std::cout << "Hata: Geçersiz görüntü indeksi: " << index << std::endl;
        return cv::Mat();
    }
    
    cv::Mat image = cv::imread(colorImagePaths[index]);
    if (image.empty()) {
        std::cout << "Hata: Görüntü yüklenemedi: " << colorImagePaths[index] << std::endl;
    }
    return image;
}

void ColorMapper::applyColorToMesh(
    pcl::PolygonMesh& mesh, 
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    const std::vector<PointCloudBuilder::PointWithImageInfo>& pointImageInfo) {

    if (colorImagePaths.empty() || angles.empty()) {
        std::cout << "Renk verisi bulunamadı!" << std::endl;
        return;
    }
    if (pointImageInfo.empty()) {
        std::cout << "Nokta görüntü eşleme bilgisi bulunamadı!" << std::endl;
        return;
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh.cloud, *meshCloud);

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr coloredCloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    coloredCloud->points.resize(meshCloud->points.size());

    // Create a PointCloud from pointImageInfo for KD-tree search
    pcl::PointCloud<pcl::PointXYZ>::Ptr sourcePoints(new pcl::PointCloud<pcl::PointXYZ>); 
    for(const auto& pii : pointImageInfo) {
        sourcePoints->push_back(pcl::PointXYZ(pii.point.x, pii.point.y, pii.point.z)); 
    }

    pcl::KdTreeFLANN<pcl::PointXYZ> kdtree;
    kdtree.setInputCloud(sourcePoints);

    std::cout << "Mesh vertex sayısı: " << meshCloud->points.size() << std::endl;
    std::cout << "Kaynak nokta sayısı (pointImageInfo): " << sourcePoints->points.size() << std::endl;
    std::cout << "Toplam renkli görüntü sayısı: " << colorImagePaths.size() << std::endl;
    std::cout << "Batch boyutu: " << BATCH_SIZE << " görüntü" << std::endl;

    // Create a mapping of mesh vertices to their closest point info for efficient processing
    std::cout << "Mesh vertex-nokta eşlemeleri hazırlanıyor..." << std::endl;
    std::vector<std::pair<size_t, int>> vertexToImageIndex; // (vertex_index, image_index)
    
    for (size_t i = 0; i < meshCloud->points.size(); i++) {
        const pcl::PointXYZ& meshVertex = meshCloud->points[i];
        std::vector<int> pointIdxNKNSearch(1);
        std::vector<float> pointNKNSquaredDistance(1);

        int imgIdx = -1;
        if (kdtree.nearestKSearch(meshVertex, 1, pointIdxNKNSearch, pointNKNSquaredDistance) > 0) {
            const auto& closestPointInfo = pointImageInfo[pointIdxNKNSearch[0]];
            imgIdx = closestPointInfo.imageIndex;
        }
        vertexToImageIndex.push_back({i, imgIdx});
    }
    
    // Sort vertices by image index to process them in batch order
    std::sort(vertexToImageIndex.begin(), vertexToImageIndex.end(), 
              [](const std::pair<size_t, int>& a, const std::pair<size_t, int>& b) {
                  return a.second < b.second;
              });
    
    std::cout << "Renklendirme başlıyor (batch sıralı işlem)..." << std::endl;
    
    // Process vertices in batch order
    std::vector<cv::Mat> currentBatch;
    int currentBatchStart = -1;
    size_t totalBatches = (colorImagePaths.size() + BATCH_SIZE - 1) / BATCH_SIZE;
    
    for (size_t idx = 0; idx < vertexToImageIndex.size(); idx++) {
        size_t i = vertexToImageIndex[idx].first;
        int imgIdx = vertexToImageIndex[idx].second;
        
        const pcl::PointXYZ& meshVertex = meshCloud->points[i];
        pcl::PointXYZRGB coloredPoint;
        coloredPoint.x = meshVertex.x;
        coloredPoint.y = meshVertex.y;
        coloredPoint.z = meshVertex.z;
        coloredPoint.r = 255; // Default white
        coloredPoint.g = 255;
        coloredPoint.b = 255;

        if (imgIdx >= 0 && imgIdx < colorImagePaths.size()) {
            // Check if we need to load a new batch
            int batchStart = (imgIdx / BATCH_SIZE) * BATCH_SIZE;
            if (batchStart != currentBatchStart) {
                // Clear previous batch to free memory
                currentBatch.clear();
                currentBatchStart = batchStart;
                
                // Load new batch
                int batchEnd = std::min(batchStart + (int)BATCH_SIZE, (int)colorImagePaths.size());
                size_t batchNumber = batchStart / BATCH_SIZE + 1;
                std::cout << "Batch " << batchNumber << "/" << totalBatches 
                          << " yükleniyor (görüntü " << batchStart << "-" << (batchEnd-1) << ")..." << std::endl;
                
                for (int j = batchStart; j < batchEnd; j++) {
                    cv::Mat img = loadImageAtIndex(j);
                    currentBatch.push_back(img);
                }
            }
            
            // Find the closest point info for this vertex
            std::vector<int> pointIdxNKNSearch(1);
            std::vector<float> pointNKNSquaredDistance(1);
            
            if (kdtree.nearestKSearch(meshVertex, 1, pointIdxNKNSearch, pointNKNSquaredDistance) > 0) {
                const auto& closestPointInfo = pointImageInfo[pointIdxNKNSearch[0]];
                int imgX = closestPointInfo.imageX;
                int imgY = closestPointInfo.imageY;
                
                // Get image from current batch
                size_t batchIndex = imgIdx - currentBatchStart;
                if (batchIndex < currentBatch.size()) {
                    const cv::Mat& sourceColorImage = currentBatch[batchIndex];
                    if (!sourceColorImage.empty() && 
                        imgY >= 0 && imgY < sourceColorImage.rows &&
                        imgX >= 0 && imgX < sourceColorImage.cols) {
                        // Average color in a 5x5 window, skipping dark pixels
                        int window = 2; // 5x5 window
                        int count = 0;
                        int sumB = 0, sumG = 0, sumR = 0;
                        for (int dy = -window; dy <= window; ++dy) {
                            int y = imgY + dy;
                            if (y < 0 || y >= sourceColorImage.rows) continue;
                            for (int dx = -window; dx <= window; ++dx) {
                                int x = imgX + dx;
                                if (x < 0 || x >= sourceColorImage.cols) continue;
                                cv::Vec3b pix = sourceColorImage.at<cv::Vec3b>(y, x);
                                int brightness = (int)pix[0] + (int)pix[1] + (int)pix[2];
                                if (brightness > 40 * 3) { // Only use non-dark pixels
                                    sumB += pix[0];
                                    sumG += pix[1];
                                    sumR += pix[2];
                                    count++;
                                }
                            }
                        }
                        if (count > 0) {
                            coloredPoint.r = sumR / count;
                            coloredPoint.g = sumG / count;
                            coloredPoint.b = sumB / count;
                        } else {
                            // Fallback to single pixel if all are dark
                            cv::Vec3b color = sourceColorImage.at<cv::Vec3b>(imgY, imgX);
                            coloredPoint.r = color[2];
                            coloredPoint.g = color[1];
                            coloredPoint.b = color[0];
                        }
                    }
                }
            }
        }
        coloredCloud->points[i] = coloredPoint;
        
        // Progress reporting every 10%
        if (idx % (vertexToImageIndex.size() / 10) == 0 || idx == vertexToImageIndex.size() - 1) {
            float progress = (float)(idx + 1) / vertexToImageIndex.size() * 100.0f;
            std::cout << "Renklendirme: " << std::fixed << std::setprecision(0) << progress << "% tamamlandı" << std::endl;
        }
    }

    pcl::toPCLPointCloud2(*coloredCloud, mesh.cloud);
    std::cout << "Mesh renklendirme tamamlandı (nokta-görüntü eşlemesi ile)." << std::endl;
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