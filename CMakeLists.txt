cmake_minimum_required(VERSION 3.10)
project(3DModelCreator)

# C++11 standardını kullan
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV, PCL, ve Eigen kütüphanelerini bul
find_package(OpenCV REQUIRED)
find_package(PCL 1.8 REQUIRED COMPONENTS common io filters surface)
find_package(Eigen3 REQUIRED)

# FBX SDK için dizinleri ayarla (sisteminize göre düzenleyin)
set(FBX_SDK_ROOT "/usr/local/fbxsdk" CACHE PATH "FBX SDK root directory")
set(FBX_SDK_INCLUDE_DIR "${FBX_SDK_ROOT}/include")
set(FBX_SDK_LIBRARY_DIR "${FBX_SDK_ROOT}/lib/gcc/x64/release")

# Include dizinlerini ayarla
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${PCL_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${FBX_SDK_INCLUDE_DIR}
)

# Derleyici flaglarını ayarla
add_definitions(${PCL_DEFINITIONS})
link_directories(${PCL_LIBRARY_DIRS} ${FBX_SDK_LIBRARY_DIR})

# Kaynak dosyaları ekle
add_executable(3DModelCreator 
    src/main.cpp
    src/LaserLineDetector.cpp
    src/PointCloudBuilder.cpp
    src/MeshCreator.cpp
    src/ColorMapper.cpp
    src/FBXExporter.cpp
)

# Kütüphaneleri bağla
target_link_libraries(3DModelCreator
    ${OpenCV_LIBS}
    ${PCL_LIBRARIES}
    -lfbxsdk
)

# Output dosyasının konumunu ayarla
set_target_properties(3DModelCreator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Oluşturulan dosyaları göster
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "PCL libraries: ${PCL_LIBRARIES}")