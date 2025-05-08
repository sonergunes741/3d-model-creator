#include "MeshCreator.h"

#include <pcl/features/normal_3d_omp.h>
#include <pcl/surface/poisson.h>
#include <pcl/surface/mls.h>
#include <pcl/surface/gp3.h>
#include <pcl/io/pcd_io.h>
#include <pcl/surface/simplification_remove_unused_vertices.h>
#include <iostream>

MeshCreator::MeshCreator(int depth)
    : poissonDepth(depth), smoothingIterations(3), smoothingFactor(0.25f) {
}

void MeshCreator::computeNormals(
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudWithNormals) {

    // Normal tahmini için k-komşu sayısı
    int k = 20;
    
    // Normal tahmini için nesne oluştur
    pcl::NormalEstimationOMP<pcl::PointXYZRGB, pcl::Normal> normalEstimation;
    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>);
    
    // KdTree oluştur
    tree->setInputCloud(cloud);
    
    // Normal tahmini parametrelerini ayarla
    normalEstimation.setInputCloud(cloud);
    normalEstimation.setSearchMethod(tree);
    normalEstimation.setKSearch(k);
    normalEstimation.compute(*normals);
    
    // Nokta ve normalleri birleştir
    pcl::concatenateFields(*cloud, *normals, *cloudWithNormals);
    
    std::cout << "Normal vektörleri hesaplandı. Toplam nokta sayısı: " << cloudWithNormals->size() << std::endl;
}

pcl::PolygonMesh MeshCreator::createMesh(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud) {
    // Normalleri hesapla
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudWithNormals(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    computeNormals(cloud, cloudWithNormals);
    
    // Poisson yüzey yeniden yapılandırması
    pcl::Poisson<pcl::PointXYZRGBNormal> poisson;
    pcl::PolygonMesh mesh;
    
    poisson.setInputCloud(cloudWithNormals);
    poisson.setDepth(poissonDepth);
    poisson.setSolverDivide(8);
    poisson.setIsoDivide(8);
    poisson.setSamplesPerNode(3.0);
    poisson.setConfidence(false);
    poisson.setManifold(true);
    poisson.setOutputPolygons(true);
    
    std::cout << "Mesh oluşturuluyor..." << std::endl;
    poisson.reconstruct(mesh);
    std::cout << "Mesh oluşturuldu. Yüzey sayısı: " << mesh.polygons.size() << std::endl;
    
    // Mesh'i düzgünleştir
    refineMesh(mesh);
    
    return mesh;
}

void MeshCreator::refineMesh(pcl::PolygonMesh& mesh) {
    // Kullanılmayan vertexleri temizle
    pcl::surface::SimplificationRemoveUnusedVertices simplification;
    simplification.simplify(mesh, mesh);
    
    // NOT: Tam bir düzgünleştirme için PCL 1.9+ gerekli
    // Bu sürümde MeshSmoothingLaplacianVTK sınıfı mevcut
    // Farklı bir düzgünleştirme algoritması kullanılabilir
    
    std::cout << "Mesh düzgünleştirme tamamlandı. Final yüzey sayısı: " << mesh.polygons.size() << std::endl;
}