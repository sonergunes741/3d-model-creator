#include "ColorMapper.h"

#include <pcl/io/vtk_lib_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/common/transforms.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <pcl/kdtree/kdtree_flann.h>

ColorMapper::ColorMapper() {
}

void ColorMapper::addColorData(const cv::Mat& colorImage, float angle) {
    // Store the full color image and its corresponding angle
    colorData.push_back(colorImage.clone());
    angles.push_back(angle);

    static bool firstLog = true;
    static bool lastLog = false;
    if (firstLog) {
        std::cout << "Renk verileri işleniyor - İlk açı: " << angle << "°" << std::endl;
        firstLog = false;
    }
    if (angle > 355.0f && !lastLog) {
        std::cout << "Renk verileri işleniyor - Son açı: " << angle << "°" << std::endl;
        lastLog = true;
    }
}

void ColorMapper::applyColorToMesh(
    pcl::PolygonMesh& mesh, 
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    const std::vector<PointCloudBuilder::PointWithImageInfo>& pointImageInfo) {

    if (colorData.empty() || angles.empty()) {
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
    std::cout << "Yüklü renkli görüntü sayısı: " << colorData.size() << std::endl;

    for (size_t i = 0; i < meshCloud->points.size(); i++) {
        const pcl::PointXYZ& meshVertex = meshCloud->points[i];
        pcl::PointXYZRGB coloredPoint;
        coloredPoint.x = meshVertex.x;
        coloredPoint.y = meshVertex.y;
        coloredPoint.z = meshVertex.z;
        coloredPoint.r = 255; // Default white
        coloredPoint.g = 255;
        coloredPoint.b = 255;

        std::vector<int> pointIdxNKNSearch(1);
        std::vector<float> pointNKNSquaredDistance(1);

        if (kdtree.nearestKSearch(meshVertex, 1, pointIdxNKNSearch, pointNKNSquaredDistance) > 0) {
            const auto& closestPointInfo = pointImageInfo[pointIdxNKNSearch[0]];
            int imgIdx = closestPointInfo.imageIndex;
            int imgX = closestPointInfo.imageX;
            int imgY = closestPointInfo.imageY;

            if (imgIdx >= 0 && imgIdx < colorData.size()) {
                const cv::Mat& sourceColorImage = colorData[imgIdx];
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
                } else {
                    // std::cout << "Debug: Out of bounds or empty image. imgIdx: " << imgIdx << " X: " << imgX << " Y: " << imgY << std::endl;
                }
            } else {
                // std::cout << "Debug: Invalid imageIndex: " << imgIdx << std::endl;
            }
        } else {
            // std::cout << "Debug: No nearest neighbor found for mesh vertex " << i << std::endl;
        }
        coloredCloud->points[i] = coloredPoint;
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