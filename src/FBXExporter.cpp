#include "FBXExporter.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <iostream>

FBXExporter::FBXExporter() 
    : fbxManager(nullptr), 
      fbxScene(nullptr),
      fbxMajorVersion(7), 
      fbxMinorVersion(5), 
      fbxRevision(0),
      useVertexColors(true) {
    
    // FBX SDK yöneticisi oluştur
    fbxManager = FbxManager::Create();
    
    if (!fbxManager) {
        std::cerr << "FBX Manager oluşturulamadı!" << std::endl;
        return;
    }
    
    // FbxIOSettings oluştur ve ayarla
    FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
    fbxManager->SetIOSettings(ios);
    
    // FBX sahne oluştur
    fbxScene = FbxScene::Create(fbxManager, "3DModelScene");
    
    std::cout << "FBX Exporter başlatıldı." << std::endl;
}

FBXExporter::~FBXExporter() {
    // Kaynakları temizle
    if (fbxScene) {
        fbxScene->Destroy();
        fbxScene = nullptr;
    }
    
    if (fbxManager) {
        fbxManager->Destroy();
        fbxManager = nullptr;
    }
}

bool FBXExporter::exportMesh(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName) {
    if (!fbxManager || !fbxScene) {
        std::cerr << "FBX Manager veya Scene oluşturulmadı!" << std::endl;
        return false;
    }
    
    // Mesh için FBX nodu oluştur
    FbxNode* meshNode = createFBXMesh(mesh, modelName);
    
    if (!meshNode) {
        std::cerr << "FBX Mesh nodu oluşturulamadı!" << std::endl;
        return false;
    }
    
    // Sahne kök noduna ekle
    fbxScene->GetRootNode()->AddChild(meshNode);
    
    // FBX dışa aktarma
    FbxExporter* exporter = FbxExporter::Create(fbxManager, "");
    
    // Çalışma alanını ayarla (Unreal için Z-up olmalı)
    FbxAxisSystem sceneAxisSystem = fbxScene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem::ECoordSystem coordSystem = FbxAxisSystem::eRightHanded;
    FbxAxisSystem::EUpVector upVector = FbxAxisSystem::eZAxis;
    FbxAxisSystem::EFrontVector frontVector = FbxAxisSystem::eParityOdd;
    
    FbxAxisSystem axisSystem(upVector, frontVector, coordSystem);
    axisSystem.ConvertScene(fbxScene);
    
    // Birim sistemini ayarla (cm)
    fbxScene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::cm);
    
    // FBX dosya formatını ayarla
    int fileFormat = fbxManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX binary (*.fbx)");
    
    // Dışa aktarıcıyı başlat
    bool exportStatus = exporter->Initialize(outputPath.c_str(), fileFormat, fbxManager->GetIOSettings());
    
    if (!exportStatus) {
        std::cerr << "FBX dışa aktarıcı başlatılamadı!" << std::endl;
        std::cerr << "Hata: " << exporter->GetStatus().GetErrorString() << std::endl;
        exporter->Destroy();
        return false;
    }
    
    // FBX sürümünü ayarla
    exporter->SetFileExportVersion(
        FbxString("FBX_") + 
        FbxString(std::to_string(fbxMajorVersion).c_str()) + 
        FbxString("0") + 
        FbxString(std::to_string(fbxMinorVersion).c_str()) + 
        FbxString("00")
    );
    
    // Dışa aktar
    exportStatus = exporter->Export(fbxScene);
    
    if (!exportStatus) {
        std::cerr << "FBX dışa aktarma başarısız oldu!" << std::endl;
        std::cerr << "Hata: " << exporter->GetStatus().GetErrorString() << std::endl;
    } else {
        std::cout << "Mesh başarıyla FBX olarak dışa aktarıldı: " << outputPath << std::endl;
    }
    
    // Dışa aktarıcıyı temizle
    exporter->Destroy();
    
    return exportStatus;
}

FbxNode* FBXExporter::createFBXMesh(const pcl::PolygonMesh& mesh, const std::string& modelName) {
    // PCL mesh'i nokta bulutu şeklinde oku
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(mesh.cloud, *cloud);
    
    // FBX mesh ve nodu oluştur
    FbxMesh* fbxMesh = FbxMesh::Create(fbxScene, modelName.c_str());
    FbxNode* meshNode = FbxNode::Create(fbxScene, modelName.c_str());
    
    // Mesh'i noda ekle
    meshNode->SetNodeAttribute(fbxMesh);
    
    // Kontrol nokta sayısını ayarla
    fbxMesh->InitControlPoints(cloud->points.size());
    FbxVector4* controlPoints = fbxMesh->GetControlPoints();
    
    // Kontrol noktaları ata
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const pcl::PointXYZRGB& point = cloud->points[i];
        controlPoints[i] = FbxVector4(point.x, point.y, point.z);
    }
    
    // Vertex renkleri için eleman oluştur
    FbxGeometryElementVertexColor* vertexColorElement = nullptr;
    
    if (useVertexColors) {
        vertexColorElement = fbxMesh->CreateElementVertexColor();
        vertexColorElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
        vertexColorElement->SetReferenceMode(FbxGeometryElement::eDirect);
    }
    
    // Vertex renkleri ata
    if (useVertexColors && vertexColorElement) {
        for (size_t i = 0; i < cloud->points.size(); i++) {
            const pcl::PointXYZRGB& point = cloud->points[i];
            FbxColor color(
                point.r / 255.0, 
                point.g / 255.0, 
                point.b / 255.0, 
                1.0
            );
            vertexColorElement->GetDirectArray().Add(color);
        }
    }
    
    // Normal vektörleri için eleman oluştur
    FbxGeometryElementNormal* normalElement = fbxMesh->CreateElementNormal();
    normalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
    normalElement->SetReferenceMode(FbxGeometryElement::eDirect);
    
    // Şimdilik basit normallerle doldur
    // Gerçek bir uygulamada mesh'ten hesaplanabilir
    for (size_t i = 0; i < cloud->points.size(); i++) {
        FbxVector4 normal(0, 0, 1);
        normalElement->GetDirectArray().Add(normal);
    }
    
    // Polygon yüzlerini ekle
    for (const auto& polygon : mesh.polygons) {
        fbxMesh->BeginPolygon();
        for (const auto& vertexIndex : polygon.vertices) {
            fbxMesh->AddPolygon(vertexIndex);
        }
        fbxMesh->EndPolygon();
    }
    
    // Materyal oluştur ve ekle
    FbxSurfacePhong* material = createMaterial(modelName + "_Material");
    if (material) {
        meshNode->AddMaterial(material);
    }
    
    return meshNode;
}

FbxSurfacePhong* FBXExporter::createMaterial(const std::string& materialName) {
    // Phong materyal oluştur
    FbxSurfacePhong* material = FbxSurfacePhong::Create(fbxScene, materialName.c_str());
    
    // Renkleri ayarla
    FbxDouble3 white(1.0, 1.0, 1.0);
    FbxDouble3 black(0.0, 0.0, 0.0);
    
    material->Diffuse.Set(white);
    material->Ambient.Set(FbxDouble3(0.5, 0.5, 0.5));
    material->Specular.Set(white);
    material->Emissive.Set(black);
    
    // Materyal özellikleri
    material->Shininess.Set(30.0);
    material->ShadingModel.Set("Phong");
    
    return material;
}

FbxFileTexture* FBXExporter::createTexture(const std::string& textureName, const pcl::PolygonMesh& mesh) {
    // Bu fonksiyon gerçek bir uygulamada, mesh vertex renklerinden
    // bir doku oluşturup dışa aktarabilir
    
    // Şimdilik sadece bir iskelet
    FbxFileTexture* texture = FbxFileTexture::Create(fbxScene, textureName.c_str());
    
    // Doku özellikleri
    texture->SetFileName("texture.png");
    texture->SetTextureUse(FbxTexture::eStandard);
    texture->SetMappingType(FbxTexture::eUV);
    texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
    texture->SetSwapUV(false);
    texture->SetTranslation(0.0, 0.0);
    texture->SetScale(1.0, 1.0);
    texture->SetRotation(0.0, 0.0);
    
    return texture;
}