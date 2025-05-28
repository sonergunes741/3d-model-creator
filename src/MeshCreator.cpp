#include "MeshCreator.h"

#include <pcl/features/normal_3d_omp.h>
#include <pcl/surface/poisson.h>
#include <pcl/surface/mls.h>
#include <pcl/surface/gp3.h>
#include <pcl/surface/marching_cubes_hoppe.h>
#include <pcl/io/pcd_io.h>
#include <pcl/surface/simplification_remove_unused_vertices.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <iostream>
#include <pcl/surface/vtk_smoothing/vtk_utils.h>
#include <pcl/surface/vtk_smoothing/vtk_mesh_smoothing_laplacian.h>

MeshCreator::MeshCreator(int depth)
    : poissonDepth(depth), smoothingIterations(1), smoothingFactor(0.1f) {
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
    
    // Nokta bulutunu ön işleme tabi tut
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr processedCloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    
    // 1. Voxel Grid filtreleme ile nokta sayısını azalt
    pcl::VoxelGrid<pcl::PointXYZRGB> voxelGrid;
    voxelGrid.setInputCloud(cloud);
    voxelGrid.setLeafSize(0.2f, 0.2f, 0.2f);  // 0.5cm voxel boyutu
    voxelGrid.filter(*processedCloud);
    
    std::cout << "Voxel Grid filtrelemeden sonra nokta sayısı: " << processedCloud->points.size() << std::endl;
    
    // 2. Statistical Outlier Removal ile gürültüyü azalt
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloudFiltered(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> sor;
    sor.setInputCloud(processedCloud);
    sor.setMeanK(60);  // Komşu sayısı
    sor.setStddevMulThresh(0.8);  // Standart sapma çarpanı
    sor.filter(*cloudFiltered);
    
    std::cout << "Gürültü temizlemeden sonra nokta sayısı: " << cloudFiltered->points.size() << std::endl;
    
    // Normalleri hesapla
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudWithNormals(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    computeNormals(cloudFiltered, cloudWithNormals);
    
    // Sonuç mesh'i
    pcl::PolygonMesh resultMesh;
    bool meshCreated = false;
    
    // YÖNTEM 1: Poisson Surface Reconstruction - Deliksiz ve düzgün mesh için birincil yöntem
    if (!meshCreated) {
        try {
            /*
                pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal> gp3;
            pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
            tree->setInputCloud(cloudWithNormals);
            /*
            200 foto için
            gp3.setSearchRadius(40.0);  // Çok daha büyük arama yarıçapı
            gp3.setMu(10.0);  // Çok daha geniş arama
            gp3.setMaximumNearestNeighbors(300);  // Çok daha fazla komşu
            gp3.setMinimumAngle(M_PI/18);  // 10 derece
            gp3.setMaximumAngle(2*M_PI/2.5);  // ~140 derece
            
            // Bardak gibi nesneler için optimize edilmiş parametreler
            gp3.setSearchRadius(20.0);  // Arama yarıçapını daha da artır
            gp3.setMu(2.5);  // Daha geniş arama
            gp3.setMaximumNearestNeighbors(100);  // Komşu sayısını daha da artır
            gp3.setMinimumAngle(M_PI/18);  // 2 derece - çok daha küçük açılar için
            gp3.setMaximumAngle(2*M_PI/2.5);  // 300 derece - çok daha geniş açılar için
            gp3.setNormalConsistency(false);  // Normal tutarlılığını devre dışı bırak
            //gp3.setConsistentVertexOrdering(true);  // Tutarlı köşe sıralaması
            
            
            gp3.setInputCloud(cloudWithNormals);
            gp3.setSearchMethod(tree);
            
            std::cout << "Greedy Projection mesh oluşturuluyor..." << std::endl;
            gp3.reconstruct(resultMesh)
            */
            pcl::Poisson<pcl::PointXYZRGBNormal> poisson;
            poisson.setInputCloud(cloudWithNormals);
            poisson.setDepth(12);  // 10-14 arası deneyebilirsiniz
            poisson.setSolverDivide(8);
            poisson.setIsoDivide(8);
            poisson.setSamplesPerNode(1.0);
            poisson.setConfidence(false);
            poisson.setManifold(true); // Manifold mesh üretimi için
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
    
    // YÖNTEM 2: Greedy Projection Triangulation - Yedek olarak bırakıldı
    /*
    if (!meshCreated) {
        try {
            pcl::GreedyProjectionTriangulation<pcl::PointXYZRGBNormal> gp3;
            pcl::search::KdTree<pcl::PointXYZRGBNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
            tree->setInputCloud(cloudWithNormals);
            // Parametreler...
            gp3.setSearchRadius(20.0);
            gp3.setMu(2.5);
            gp3.setMaximumNearestNeighbors(100);
            gp3.setMinimumAngle(M_PI/18);
            gp3.setMaximumAngle(2*M_PI/2.5);
            gp3.setNormalConsistency(false);
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
    */
    
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
        
        // Nokta bulutu kopyala ve organize et
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr organizedCloud(new pcl::PointCloud<pcl::PointXYZRGB>());
        organizedCloud->points = cloud->points;
        
        // Nokta sayısından grid boyutları tahmin et
        int width = std::sqrt(cloud->size());
        // Nokta bulutu organize değilse, organize hale getir
        organizedCloud->width = width;
        organizedCloud->height = std::ceil(static_cast<float>(cloud->size()) / width);
        organizedCloud->is_dense = false;
        
        // PCLPointCloud2 formatına dönüştür
        pcl::toPCLPointCloud2(*organizedCloud, resultMesh.cloud);
        
        // Üçgenleri manuel oluştur
        int height = organizedCloud->height;
        width = organizedCloud->width;
        
        for (int y = 0; y < height - 1; y++) {
            for (int x = 0; x < width - 1; x++) {
                // Her dörtgen için iki üçgen oluştur
                pcl::Vertices triangle1, triangle2;
                triangle1.vertices.resize(3);
                triangle2.vertices.resize(3);
                
                // İlk üçgen (saat yönünde)
                triangle1.vertices[0] = y * width + x;
                triangle1.vertices[1] = y * width + (x + 1);
                triangle1.vertices[2] = (y + 1) * width + x;
                
                // İkinci üçgen (saat yönünde)
                triangle2.vertices[0] = y * width + (x + 1);
                triangle2.vertices[1] = (y + 1) * width + (x + 1);
                triangle2.vertices[2] = (y + 1) * width + x;
                
                // Geçerli indisler kontrolü
                bool valid = true;
                for (int i = 0; i < 3; i++) {
                    if (triangle1.vertices[i] >= organizedCloud->size() || 
                        triangle2.vertices[i] >= organizedCloud->size()) {
                        valid = false;
                        break;
                    }
                }
                
                if (valid) {
                    resultMesh.polygons.push_back(triangle1);
                    resultMesh.polygons.push_back(triangle2);
                }
            }
        }
        
        std::cout << "Manuel mesh oluşturuldu. Yüzey sayısı: " << resultMesh.polygons.size() << std::endl;
        meshCreated = true;
    }
    
    // Hiçbir yöntem başarılı olamazsa, son çare: Minimum düzeyde üçgenleştirme
    if (!meshCreated || resultMesh.polygons.size() == 0) {
        std::cout << "Kritik: Fallback mesh oluşturuluyor..." << std::endl;
        
        // Tüm noktaları PCLPointCloud2 formatına dönüştür
        pcl::toPCLPointCloud2(*cloud, resultMesh.cloud);
        
        // Minimum sayıda üçgen oluştur
        for (size_t i = 0; i < cloud->size() - 2; i += 3) {
            pcl::Vertices v;
            v.vertices.resize(3);
            v.vertices[0] = i;
            v.vertices[1] = i + 1;
            v.vertices[2] = i + 2;
            resultMesh.polygons.push_back(v);
        }
        
        // Tek bir üçgen bile olsa mesh oluşturmayı garantile
        if (resultMesh.polygons.size() == 0 && cloud->size() >= 3) {
            pcl::Vertices v;
            v.vertices.resize(3);
            v.vertices[0] = 0;
            v.vertices[1] = 1;
            v.vertices[2] = 2;
            resultMesh.polygons.push_back(v);
        }
        
        std::cout << "Fallback mesh oluşturuldu. Yüzey sayısı: " << resultMesh.polygons.size() << std::endl;
    }
    
    // Mesh iyileştirme
    if (resultMesh.polygons.size() > 0) {
        refineMesh(resultMesh);
        std::cout << "Final mesh polygon sayısı: " << resultMesh.polygons.size() << std::endl;
    } else {
        std::cerr << "KRITIK HATA: Mesh oluşturulamadı, hiç polygon yok!" << std::endl;
    }
    
    return resultMesh;
}

void MeshCreator::refineMesh(pcl::PolygonMesh& mesh) {
    // Save original mesh in case smoothing fails
    pcl::PolygonMesh originalMesh = mesh;

    // Apply Laplacian smoothing using PCL's VTK wrapper
    try {
        pcl::MeshSmoothingLaplacianVTK vtkSmoother;
        vtkSmoother.setInputMesh(pcl::make_shared<pcl::PolygonMesh>(mesh));
        vtkSmoother.setNumIter(smoothingIterations > 0 ? smoothingIterations : 10);
        vtkSmoother.setConvergence(smoothingFactor > 0 ? smoothingFactor : 0.01f);
        vtkSmoother.setRelaxationFactor(0.01f); // Default, can be parameterized
        vtkSmoother.setFeatureEdgeSmoothing(false);
        vtkSmoother.setBoundarySmoothing(true);
        vtkSmoother.process(mesh);

        // Check if smoothing produced a valid mesh
        if (mesh.polygons.empty()) {
            std::cerr << "Uyarı: Düzgünleştirme tüm polygonları kaldırdı! Orijinal mesh kullanılıyor." << std::endl;
            mesh = originalMesh;
        } else {
            std::cout << "Laplacian mesh smoothing uygulandı." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Mesh düzgünleştirme hatası: " << e.what() << std::endl;
        std::cerr << "Orijinal mesh kullanılıyor." << std::endl;
        mesh = originalMesh;
    }
}