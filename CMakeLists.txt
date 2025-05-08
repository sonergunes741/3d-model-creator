cmake_minimum_required(VERSION 3.10)
project(3DModelCreator)

# C++17 standardını kullan (filesystem için gerekli)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenMP'yi bul (NormalEstimationOMP için)
find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# OpenCV, PCL, ve Eigen kütüphanelerini bul
find_package(OpenCV REQUIRED)
find_package(PCL 1.8 REQUIRED COMPONENTS common io filters surface features kdtree search)
find_package(Eigen3 REQUIRED)

# Include dizinlerini ayarla
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${PCL_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

# Derleyici flaglarını ayarla
add_definitions(${PCL_DEFINITIONS})
link_directories(${PCL_LIBRARY_DIRS})

# Kaynak dosyaları ekle
add_executable(3DModelCreator 
    src/main.cpp
    src/LaserLineDetector.cpp
    src/PointCloudBuilder.cpp
    src/MeshCreator.cpp
    src/ColorMapper.cpp
    src/OBJExporter.cpp
)

# Kütüphaneleri bağla
target_link_libraries(3DModelCreator
    ${OpenCV_LIBS}
    ${PCL_LIBRARIES}
    stdc++fs  # filesystem kütüphanesi için
    ${OpenMP_CXX_LIBRARIES}  # OpenMP kütüphaneleri
)

# Output dosyasının konumunu ayarla
set_target_properties(3DModelCreator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Oluşturulan dosyaları göster
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "PCL libraries: ${PCL_LIBRARIES}")