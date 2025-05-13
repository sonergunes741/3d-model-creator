#include "MeshCreator.h"

#include <pcl/features/normal_3d_omp.h>
#include <pcl/surface/poisson.h>
#include <pcl/surface/mls.h>
#include <pcl/surface/gp3.h>
#include <pcl/surface/marching_cubes_hoppe.h>
#include <pcl/io/pcd_io.h>
#include <pcl/surface/simplification_remove_unused_vertices.h>
#include <iostream>

MeshCreator::MeshCreator(int depth)
    : poissonDepth(depth), smoothingIterations(3), smoothingFactor(0.25f) {
}

void MeshCreator::computeNormals(
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudWithNormals) {

    if (cloud->empty()) {
        std::cerr << "HATA: Boş nokta bulutu, normaller hesaplanamadı!" << std::endl;
        return;
    }

    std::cout << "Giriş nokta bulutu boyutu: " << cloud->points.size() << std::endl;
    
    // Normal tahmini için k-komşu sayısı
    int k = 15;  // Bardak için optimize edilmiş değer
    
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
    normalEstimation.setViewPoint(0, 0, 0);  // Bakış noktasını ayarla
    normalEstimation.compute(*normals);
    
    // Nokta ve normalleri birleştir
    pcl::concatenateFields(*cloud, *normals, *cloudWithNormals);
    
    std::cout << "Normal vektörleri hesaplandı. Toplam nokta sayısı: " << cloudWithNormals->size() << std::endl;
}

pcl::PolygonMesh MeshCreator::createMesh(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud) {
    // Giriş kontrolü
    if (cloud->empty()) {
        std::cerr << "HATA: Boş nokta bulutu!" << std::endl;
        return pcl::PolygonMesh();
    }
    
    // Nokta bulutu bilgilerini yazdır
    std::cout << "Mesh oluşturma için nokta sayısı: " << cloud->points.size() << std::endl;
    
    // Normalleri hesapla
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudWithNormals(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    computeNormals(cloud, cloudWithNormals);
    
    // Sonuç mesh'i
    pcl::PolygonMesh resultMesh;
    bool meshCreated = false;
    
    // YÖNTEM 1: Greedy Projection Triangulation - Bardak gibi nesneler için daha güvenilir
    if (!meshCreated) {
        try {
            pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal> gp3;
            pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
            tree->setInputCloud(cloudWithNormals);
            
            // Bardak gibi nesneler için optimize edilmiş parametreler
            gp3.setSearchRadius(10.0);  // Daha büyük arama yarıçapı 
            gp3.setMu(5.0);  // Daha geniş arama
            gp3.setMaximumNearestNeighbors(100);  // Daha fazla komşu
            gp3.setMinimumAngle(M_PI/18);  // 10 derece
            gp3.setMaximumAngle(2*M_PI/2.5);  // ~140 derece
            gp3.setNormalConsistency(false);  // Normal tutarlılığını devre dışı bırak
            
            gp3.setInputCloud(cloudWithNormals);
            gp3.setSearchMethod(tree);
            
            std::cout << "Greedy Projection mesh oluşturuluyor..." << std::endl;
            gp3.reconstruct(resultMesh);
            
            if (resultMesh.polygons.size() > 0) {
                std::cout << "Greedy Projection mesh oluşturuldu. Yüzey sayısı: " << resultMesh.polygons.size() << std::endl;
                meshCreated = true;
            } else {
                std::cout << "Greedy Projection mesh oluşturulamadı, diğer yöntem deneniyor..." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Greedy Projection hatası: " << e.what() << std::endl;
        }
    }
    
    // YÖNTEM 2: Poisson Surface Reconstruction
    if (!meshCreated) {
        try {
            pcl::Poisson<pcl::PointXYZRGBNormal> poisson;
            
            poisson.setInputCloud(cloudWithNormals);
            poisson.setDepth(7);  // Bardak için uygun derinlik
            poisson.setSolverDivide(7);
            poisson.setIsoDivide(7);
            poisson.setSamplesPerNode(1.0);  // Daha esnek oluşturma
            poisson.setConfidence(false);
            poisson.setManifold(false);  // Manifold kısıtlamasını kaldır
            poisson.setOutputPolygons(true);
            
            std::cout << "Poisson mesh oluşturuluyor..." << std::endl;
            poisson.reconstruct(resultMesh);
            
            if (resultMesh.polygons.size() > 0) {
                std::cout << "Poisson mesh oluşturuldu. Yüzey sayısı: " << resultMesh.polygons.size() << std::endl;
                meshCreated = true;
            } else {
                std::cout << "Poisson mesh oluşturulamadı, diğer yöntem deneniyor..." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Poisson hatası: " << e.what() << std::endl;
        }
    }
    
    // YÖNTEM 3: Marching Cubes - Son çare
    if (!meshCreated) {
        try {
            pcl::MarchingCubesHoppe<pcl::PointXYZRGBNormal> mc;
            mc.setInputCloud(cloudWithNormals);
            mc.setGridResolution(50, 50, 50);  // Grid çözünürlüğü
            mc.setIsoLevel(0.0f);
            mc.setPercentageExtendGrid(0.1f);  // Grid genişletme
            
            std::cout << "Marching Cubes mesh oluşturuluyor..." << std::endl;
            mc.reconstruct(resultMesh);
            
            if (resultMesh.polygons.size() > 0) {
                std::cout << "Marching Cubes mesh oluşturuldu. Yüzey sayısı: " << resultMesh.polygons.size() << std::endl;
                meshCreated = true;
            } else {
                std::cout << "Marching Cubes mesh oluşturulamadı!" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Marching Cubes hatası: " << e.what() << std::endl;
        }
    }
    
    // YÖNTEM 4: Manuel mesh oluşturma (Son çare)
    if (!meshCreated) {
        std::cout << "Tüm mesh oluşturma algoritmaları başarısız oldu, manuel yöntem deneniyor..." << std::endl;
        
        // Nokta bulutu kopyala
        pcl::PointCloud<pcl::PointXYZ>::Ptr simpleCloud(new pcl::PointCloud<pcl::PointXYZ>);
        simpleCloud->points.resize(cloud->size());
        
        for (size_t i = 0; i < cloud->size(); i++) {
            simpleCloud->points[i].x = cloud->points[i].x;
            simpleCloud->points[i].y = cloud->points[i].y;
            simpleCloud->points[i].z = cloud->points[i].z;
        }
        
        // PCLPointCloud2 formatına dönüştür
        pcl::toPCLPointCloud2(*simpleCloud, resultMesh.cloud);
        
        // Basit üçgenler oluştur - grid oluşturarak
        int width = std::sqrt(cloud->size());
        int height = cloud->size() / width;
        
        for (int y = 0; y < height - 1; y++) {
            for (int x = 0; x < width - 1; x++) {
                pcl::Vertices v;
                v.vertices.resize(3);
                
                // İlk üçgen
                v.vertices[0] = y * width + x;
                v.vertices[1] = (y + 1) * width + x;
                v.vertices[2] = y * width + x + 1;
                resultMesh.polygons.push_back(v);
                
                // İkinci üçgen
                v.vertices[0] = (y + 1) * width + x;
                v.vertices[1] = (y + 1) * width + x + 1;
                v.vertices[2] = y * width + x + 1;
                resultMesh.polygons.push_back(v);
            }
        }
        
        std::cout << "Manuel mesh oluşturuldu. Yüzey sayısı: " << resultMesh.polygons.size() << std::endl;
        meshCreated = true;
    }
    
    // Mesh iyileştirme
    if (meshCreated && resultMesh.polygons.size() > 0) {
        refineMesh(resultMesh);
    } else {
        std::cerr << "KRITIK HATA: Tüm mesh oluşturma yöntemleri başarısız oldu!" << std::endl;
    }
    
    return resultMesh;
}

void MeshCreator::refineMesh(pcl::PolygonMesh& mesh) {
    // Kullanılmayan vertexleri temizle
    pcl::surface::SimplificationRemoveUnusedVertices simplification;
    simplification.simplify(mesh, mesh);
    
    std::cout << "Mesh düzgünleştirme tamamlandı. Final yüzey sayısı: " << mesh.polygons.size() << std::endl;
}