#include "MeshExporter.h"
#include <pcl/io/pcd_io.h>
#include <pcl/io/obj_io.h>
#include <pcl/common/common.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

MeshExporter::MeshExporter() 
    : useVertexColors(true),
      textureWidth(1024),
      textureHeight(1024) {
}

bool MeshExporter::exportMesh(
    const pcl::PolygonMesh& mesh, 
    const std::string& outputPath, 
    ExportFormat format,
    const std::string& modelName) {
    
    switch (format) {
        case ExportFormat::OBJ:
            return exportOBJ(mesh, outputPath, modelName);
        case ExportFormat::PLY:
            return exportPLY(mesh, outputPath, modelName);
        case ExportFormat::FBX:
            return exportFBX(mesh, outputPath, modelName);
        default:
            std::cerr << "Unsupported export format!" << std::endl;
            return false;
    }
}

bool MeshExporter::exportAllFormats(
    const pcl::PolygonMesh& mesh,
    const std::string& basePath,
    const std::string& modelName) {
    
    std::cout << "Exporting to all formats (OBJ, PLY, FBX)..." << std::endl;
    
    bool success = true;
    
    // Export OBJ (best for Unreal Engine)
    std::cout << "Exporting OBJ format (recommended for Unreal Engine)..." << std::endl;
    success &= exportOBJ(mesh, basePath, modelName);
    
    // Export PLY (for 3D printing and other uses)
    std::cout << "Exporting PLY format (for 3D printing and general use)..." << std::endl;
    success &= exportPLY(mesh, basePath, modelName);
    
    // Export binary FBX using Assimp
    std::cout << "Exporting binary FBX format (for Unreal Engine)..." << std::endl;
    success &= exportFBX(mesh, basePath, modelName);
    
    return success;
}

bool MeshExporter::exportOBJ(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName) {
    return writeOBJFiles(mesh, outputPath, modelName);
}

bool MeshExporter::exportPLY(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName) {
    return writePLYFile(mesh, outputPath, modelName);
}

bool MeshExporter::exportFBX(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName) {
    return writeFBXFile(mesh, outputPath, modelName);
}



bool MeshExporter::writeOBJFiles(const pcl::PolygonMesh& mesh, const std::string& basePath, const std::string& modelName) {
    // Use the existing OBJ export logic
    std::string objPath = basePath + ".obj";
    std::string mtlPath = basePath + ".mtl";
    std::string texturePath = basePath + ".png";
    
    // Extract point cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(mesh.cloud, *cloud);
    
    // Generate UV coordinates
    auto uvCoordinates = generateUVCoordinates(mesh);
    
    // Create texture
    cv::Mat texture = createTextureImage(mesh, uvCoordinates);
    cv::imwrite(texturePath, texture);
    
    // Write MTL file
    std::ofstream mtlFile(mtlPath);
    if (!mtlFile.is_open()) {
        std::cerr << "Cannot create MTL file: " << mtlPath << std::endl;
        return false;
    }
    
    mtlFile << "# MTL file created by 3D Model Creator" << std::endl;
    mtlFile << "# Optimized for Unreal Engine import" << std::endl;
    mtlFile << "newmtl " << modelName << "_material" << std::endl;
    mtlFile << "Ka 0.200 0.200 0.200  # Ambient color" << std::endl;
    mtlFile << "Kd 1.000 1.000 1.000  # Diffuse color (white to show texture properly)" << std::endl;
    mtlFile << "Ks 0.000 0.000 0.000  # Specular color (disabled for better texture display)" << std::endl;
    mtlFile << "Ns 0.0                # Specular exponent (disabled)" << std::endl;
    mtlFile << "d 1.0                 # Transparency (fully opaque)" << std::endl;
    mtlFile << "illum 1               # Illumination model (diffuse only)" << std::endl;
    mtlFile << "map_Kd " << fs::path(texturePath).filename().string() << "  # Diffuse texture map" << std::endl;
    mtlFile.close();
    
    // Write OBJ file
    std::ofstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "Cannot create OBJ file: " << objPath << std::endl;
        return false;
    }
    
    objFile << "# OBJ file created by 3D Model Creator" << std::endl;
    objFile << "# Optimized for Unreal Engine import" << std::endl;
    objFile << "# Contains: vertices, texture coordinates, normals, and faces" << std::endl;
    objFile << "# Import this OBJ file along with the MTL and PNG files" << std::endl;
    objFile << "mtllib " << fs::path(mtlPath).filename().string() << std::endl;
    objFile << std::endl;
    
    // Write vertices
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const auto& p = cloud->points[i];
        objFile << "v " << p.x << " " << p.y << " " << p.z;
        if (useVertexColors) {
            objFile << " " << p.r / 255.0f << " " << p.g / 255.0f << " " << p.b / 255.0f;
        }
        objFile << std::endl;
    }
    
    // Write texture coordinates
    for (size_t i = 0; i < uvCoordinates.size(); i++) {
        objFile << "vt " << uvCoordinates[i].first << " " << uvCoordinates[i].second << std::endl;
    }
    
    // Write normals
    for (size_t i = 0; i < cloud->points.size(); i++) {
        objFile << "vn 0.0 1.0 0.0" << std::endl;
    }
    
    // Use material
    objFile << "usemtl " << modelName << "_material" << std::endl;
    
    // Write faces
    for (const auto& polygon : mesh.polygons) {
        if (polygon.vertices.size() >= 3) {
            objFile << "f";
            for (size_t j = 0; j < polygon.vertices.size(); j++) {
                size_t vertexIndex = polygon.vertices[j] + 1;
                objFile << " " << vertexIndex << "/" << vertexIndex << "/" << vertexIndex;
            }
            objFile << std::endl;
        }
    }
    
    objFile.close();
    
    std::cout << "OBJ files created: " << objPath << ", " << mtlPath << ", " << texturePath << std::endl;
    return true;
}

bool MeshExporter::writePLYFile(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName) {
    std::string plyPath = outputPath + ".ply";
    
    // Extract point cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(mesh.cloud, *cloud);
    
    std::ofstream plyFile(plyPath);
    if (!plyFile.is_open()) {
        std::cerr << "Cannot create PLY file: " << plyPath << std::endl;
        return false;
    }
    
    // Write PLY header
    plyFile << "ply" << std::endl;
    plyFile << "format ascii 1.0" << std::endl;
    plyFile << "comment Created by 3D Model Creator" << std::endl;
    plyFile << "element vertex " << cloud->points.size() << std::endl;
    plyFile << "property float x" << std::endl;
    plyFile << "property float y" << std::endl;
    plyFile << "property float z" << std::endl;
    if (useVertexColors) {
        plyFile << "property uchar red" << std::endl;
        plyFile << "property uchar green" << std::endl;
        plyFile << "property uchar blue" << std::endl;
    }
    plyFile << "element face " << mesh.polygons.size() << std::endl;
    plyFile << "property list uchar int vertex_indices" << std::endl;
    plyFile << "end_header" << std::endl;
    
    // Write vertices
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const auto& p = cloud->points[i];
        plyFile << p.x << " " << p.y << " " << p.z;
        if (useVertexColors) {
            plyFile << " " << (int)p.r << " " << (int)p.g << " " << (int)p.b;
        }
        plyFile << std::endl;
    }
    
    // Write faces
    for (const auto& polygon : mesh.polygons) {
        if (polygon.vertices.size() >= 3) {
            plyFile << polygon.vertices.size();
            for (size_t j = 0; j < polygon.vertices.size(); j++) {
                plyFile << " " << polygon.vertices[j];
            }
            plyFile << std::endl;
        }
    }
    
    plyFile.close();
    std::cout << "PLY file created: " << plyPath << std::endl;
    return true;
}

bool MeshExporter::writeFBXFile(const pcl::PolygonMesh& mesh, const std::string& outputPath, const std::string& modelName) {
    std::string fbxPath = outputPath + ".fbx";
    
    std::cout << "Creating binary FBX file using Assimp: " << fbxPath << std::endl;
    
    // Extract point cloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(mesh.cloud, *cloud);
    
    // Create Assimp scene
    aiScene* scene = new aiScene();
    
    // Set up scene root node
    scene->mRootNode = new aiNode();
    scene->mRootNode->mName = aiString("RootNode");
    
    // Create mesh node
    aiNode* meshNode = new aiNode();
    meshNode->mName = aiString(modelName);
    meshNode->mNumMeshes = 1;
    meshNode->mMeshes = new unsigned int[1];
    meshNode->mMeshes[0] = 0;
    
    // Add mesh node to root
    scene->mRootNode->mNumChildren = 1;
    scene->mRootNode->mChildren = new aiNode*[1];
    scene->mRootNode->mChildren[0] = meshNode;
    meshNode->mParent = scene->mRootNode;
    
    // Create mesh
    scene->mNumMeshes = 1;
    scene->mMeshes = new aiMesh*[1];
    aiMesh* aiMeshPtr = new aiMesh();
    scene->mMeshes[0] = aiMeshPtr;
    
    aiMeshPtr->mName = aiString(modelName);
    aiMeshPtr->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    
    // Set vertices
    aiMeshPtr->mNumVertices = cloud->points.size();
    aiMeshPtr->mVertices = new aiVector3D[cloud->points.size()];
    
    // Set vertex colors
    aiMeshPtr->mColors[0] = new aiColor4D[cloud->points.size()];
    
    // Generate UV coordinates for texture mapping
    auto uvCoordinates = generateUVCoordinates(mesh);
    aiMeshPtr->mTextureCoords[0] = new aiVector3D[cloud->points.size()];
    
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const auto& p = cloud->points[i];
        aiMeshPtr->mVertices[i] = aiVector3D(p.x, p.y, p.z);
        aiMeshPtr->mColors[0][i] = aiColor4D(p.r / 255.0f, p.g / 255.0f, p.b / 255.0f, 1.0f);
        
        // Add UV coordinates
        if (i < uvCoordinates.size()) {
            aiMeshPtr->mTextureCoords[0][i] = aiVector3D(uvCoordinates[i].first, uvCoordinates[i].second, 0.0f);
        } else {
            aiMeshPtr->mTextureCoords[0][i] = aiVector3D(0.0f, 0.0f, 0.0f);
        }
    }
    
    // Set the number of UV coordinate sets
    aiMeshPtr->mNumUVComponents[0] = 2;
    
    // Set normals (simple upward normals for now)
    aiMeshPtr->mNormals = new aiVector3D[cloud->points.size()];
    for (size_t i = 0; i < cloud->points.size(); i++) {
        aiMeshPtr->mNormals[i] = aiVector3D(0.0f, 1.0f, 0.0f);
    }
    
    // Set faces - only include valid triangles
    std::vector<aiFace> validFaces;
    for (size_t i = 0; i < mesh.polygons.size(); i++) {
        const auto& polygon = mesh.polygons[i];
        if (polygon.vertices.size() >= 3) {
            aiFace face;
            face.mNumIndices = polygon.vertices.size();
            face.mIndices = new unsigned int[polygon.vertices.size()];
            
            for (size_t j = 0; j < polygon.vertices.size(); j++) {
                face.mIndices[j] = polygon.vertices[j];
            }
            validFaces.push_back(face);
        }
    }
    
    aiMeshPtr->mNumFaces = validFaces.size();
    aiMeshPtr->mFaces = new aiFace[validFaces.size()];
    for (size_t i = 0; i < validFaces.size(); i++) {
        aiMeshPtr->mFaces[i] = validFaces[i];
    }
    
    // Create texture image first
    cv::Mat texture = createTextureImage(mesh, uvCoordinates);
    std::string texturePath = outputPath + ".png";
    cv::imwrite(texturePath, texture);
    
    // Create material
    scene->mNumMaterials = 1;
    scene->mMaterials = new aiMaterial*[1];
    aiMaterial* material = new aiMaterial();
    scene->mMaterials[0] = material;
    
    // Set material properties for Unreal Engine
    aiString matName(modelName + "_Material");
    material->AddProperty(&matName, AI_MATKEY_NAME);
    
    // Set diffuse color to white so texture shows properly
    aiColor3D diffuse(1.0f, 1.0f, 1.0f);
    material->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
    
    // Set ambient color
    aiColor3D ambient(0.2f, 0.2f, 0.2f);
    material->AddProperty(&ambient, 1, AI_MATKEY_COLOR_AMBIENT);
    
    // Disable specular for better texture display
    aiColor3D specular(0.0f, 0.0f, 0.0f);
    material->AddProperty(&specular, 1, AI_MATKEY_COLOR_SPECULAR);
    
    float shininess = 0.0f;
    material->AddProperty(&shininess, 1, AI_MATKEY_SHININESS);
    
    // Set opacity
    float opacity = 1.0f;
    material->AddProperty(&opacity, 1, AI_MATKEY_OPACITY);
    
    // Add diffuse texture
    std::string textureFilename = fs::path(texturePath).filename().string();
    aiString texPath(textureFilename);
    material->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
    
    // Set texture mapping mode
    int mappingMode = aiTextureMapMode_Wrap;
    material->AddProperty(&mappingMode, 1, AI_MATKEY_MAPPINGMODE_U(aiTextureType_DIFFUSE, 0));
    material->AddProperty(&mappingMode, 1, AI_MATKEY_MAPPINGMODE_V(aiTextureType_DIFFUSE, 0));
    
    // Set shading model for Unreal Engine compatibility
    int shadingModel = aiShadingMode_Phong;
    material->AddProperty(&shadingModel, 1, AI_MATKEY_SHADING_MODEL);
    
    // Assign material to mesh
    aiMeshPtr->mMaterialIndex = 0;
    
    // Export to binary FBX
    Assimp::Exporter exporter;
    
    // Check if FBX export is supported
    bool fbxSupported = false;
    std::string fbxFormatId = "";
    for (size_t i = 0; i < exporter.GetExportFormatCount(); i++) {
        const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
        std::string formatId = std::string(desc->id);
        if (formatId == "fbx" || formatId == "fbxa" || formatId == "fbxb") {
            fbxSupported = true;
            fbxFormatId = formatId;
            std::cout << "Found FBX export format: " << desc->id << " - " << desc->description << std::endl;
            break;
        }
    }
    
    if (!fbxSupported) {
        std::cerr << "FBX export not supported by Assimp" << std::endl;
        
        // List all available formats for debugging
        std::cout << "Available export formats:" << std::endl;
        for (size_t i = 0; i < exporter.GetExportFormatCount(); i++) {
            const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
            std::cout << "  " << desc->id << " - " << desc->description << std::endl;
        }
        
        delete scene;
        return false;
    }
    
    // Validate scene before export
    if (aiMeshPtr->mNumVertices == 0 || aiMeshPtr->mNumFaces == 0) {
        std::cerr << "Invalid mesh data: vertices=" << aiMeshPtr->mNumVertices 
                  << ", faces=" << aiMeshPtr->mNumFaces << std::endl;
        delete scene;
        return false;
    }
    
    // Try different export approaches for better compatibility
    aiReturn result = AI_FAILURE;
    
    // First try: Standard FBX export without post-processing
    std::cout << "Attempting FBX export with format: " << fbxFormatId << std::endl;
    result = exporter.Export(scene, fbxFormatId.c_str(), fbxPath.c_str(), 0);
    
    if (result != AI_SUCCESS) {
        std::cout << "Standard export failed, trying with triangulation..." << std::endl;
        result = exporter.Export(scene, fbxFormatId.c_str(), fbxPath.c_str(), aiProcess_Triangulate);
    }
    
    if (result != AI_SUCCESS) {
        std::cout << "Triangulation export failed, trying ASCII FBX..." << std::endl;
        // Try ASCII FBX as fallback
        for (size_t i = 0; i < exporter.GetExportFormatCount(); i++) {
            const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
            std::string formatId = std::string(desc->id);
            if (formatId == "fbxa") {
                result = exporter.Export(scene, "fbxa", fbxPath.c_str(), 0);
                if (result == AI_SUCCESS) {
                    std::cout << "Successfully exported as ASCII FBX" << std::endl;
                    break;
                }
            }
        }
    }
    
    if (result != AI_SUCCESS) {
        std::cerr << "All FBX export attempts failed: " << exporter.GetErrorString() << std::endl;
        std::cerr << "This might be due to Assimp's FBX export limitations." << std::endl;
        std::cerr << "Recommendation: Use OBJ format for Unreal Engine instead." << std::endl;
        delete scene;
        return false;
    }
    
    // Clean up
    delete scene;
    
    // Validate the exported file
    if (!fs::exists(fbxPath)) {
        std::cerr << "FBX file was not created: " << fbxPath << std::endl;
        delete scene;
        return false;
    }
    
    // Check file size
    auto fileSize = fs::file_size(fbxPath);
    if (fileSize < 100) {  // FBX files should be at least a few hundred bytes
        std::cerr << "FBX file seems too small (" << fileSize << " bytes), might be corrupted" << std::endl;
        delete scene;
        return false;
    }
    
    std::cout << "FBX file created successfully: " << fbxPath << std::endl;
    std::cout << "  - File size: " << fileSize << " bytes" << std::endl;
    std::cout << "  - Vertices: " << aiMeshPtr->mNumVertices << std::endl;
    std::cout << "  - Faces: " << aiMeshPtr->mNumFaces << std::endl;
    std::cout << "  - Materials: " << scene->mNumMaterials << std::endl;
    std::cout << "  - Texture: " << texturePath << std::endl;
    std::cout << "  - UV Coordinates: " << (aiMeshPtr->mTextureCoords[0] ? "Yes" : "No") << std::endl;
    std::cout << "  - Vertex Colors: " << (aiMeshPtr->mColors[0] ? "Yes" : "No") << std::endl;
    
    std::cout << "\nIMPORTANT: If FBX import fails in Unreal Engine, use the OBJ format instead:" << std::endl;
    std::cout << "  - Import: " << outputPath << ".obj" << std::endl;
    std::cout << "  - Along with: " << outputPath << ".mtl and " << outputPath << ".png" << std::endl;
    
    return true;
}



std::vector<std::pair<float, float>> MeshExporter::generateUVCoordinates(const pcl::PolygonMesh& mesh) {
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromPCLPointCloud2(mesh.cloud, *cloud);
    
    pcl::PointXYZ min_pt, max_pt;
    pcl::getMinMax3D(*cloud, min_pt, max_pt);
    
    std::vector<std::pair<float, float>> uvCoordinates;
    
    for (size_t i = 0; i < cloud->points.size(); i++) {
        const pcl::PointXYZ& point = cloud->points[i];
        
        float angle = std::atan2(point.z, point.x);
        float u = (angle + M_PI) / (2.0f * M_PI);
        
        float v = 0.5f;
        float height_range = max_pt.y - min_pt.y;
        if (height_range > 0.0001f) {
            v = (point.y - min_pt.y) / height_range;
        }
        
        u = std::min(1.0f, std::max(0.0f, u));
        v = std::min(1.0f, std::max(0.0f, v));
        
        uvCoordinates.push_back(std::make_pair(u, v));
    }
    
    return uvCoordinates;
}

pcl::PointCloud<pcl::PointXYZRGB>::Ptr MeshExporter::extractColors(const pcl::PolygonMesh& mesh) {
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr colorCloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::fromPCLPointCloud2(mesh.cloud, *colorCloud);
    
    if (colorCloud->empty()) {
        std::cerr << "WARNING: No color information found in mesh." << std::endl;
        colorCloud->points.resize(1);
        colorCloud->points[0].r = 255;
        colorCloud->points[0].g = 255;
        colorCloud->points[0].b = 255;
    }
    
    return colorCloud;
}

cv::Mat MeshExporter::createTextureImage(const pcl::PolygonMesh& mesh, const std::vector<std::pair<float, float>>& uvCoordinates) {
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr colorCloud = extractColors(mesh);
    cv::Mat texture = cv::Mat::zeros(textureHeight, textureWidth, CV_8UC3);
    
    if (colorCloud->empty() || uvCoordinates.empty()) {
        texture = cv::Mat(textureHeight, textureWidth, CV_8UC3, cv::Scalar(255, 255, 255));
        return texture;
    }
    
    for (size_t i = 0; i < colorCloud->points.size() && i < uvCoordinates.size(); i++) {
        const pcl::PointXYZRGB& point = colorCloud->points[i];
        const std::pair<float, float>& uv = uvCoordinates[i];
        
        int x = static_cast<int>(uv.first * (textureWidth - 1));
        int y = static_cast<int>((1.0f - uv.second) * (textureHeight - 1));
        
        if (x >= 0 && x < textureWidth && y >= 0 && y < textureHeight) {
            texture.at<cv::Vec3b>(y, x) = cv::Vec3b(point.b, point.g, point.r);
        }
    }
    
    return texture;
} 