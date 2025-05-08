# 3D Model Oluşturma Modülü

Bu modül, lazer ve LED aydınlatmalı fotoğrafları işleyerek nesnenin 3D modelini oluşturur ve FBX formatında dışa aktarır.

## Özellikler

- Lazer çizgisi tespiti ve 3D nokta bulutu oluşturma
- Renkli görüntülerden renk bilgisi çıkarımı
- Poisson yüzey yeniden yapılandırması ile mesh üretimi
- Renk bilgisinin mesh'e uygulanması
- FBX formatında dışa aktarma (Unreal Engine uyumlu)

## Gereksinimler

- C++ Derleyici (C++11 veya üstü)
- CMake (3.10 veya üstü)
- OpenCV (4.x önerilen)
- PCL (Point Cloud Library) 1.8 veya üstü
- Eigen3 
- FBX SDK (2020 veya üstü önerilen)

## Kurulum

### Bağımlılıkları Yükleme

Ubuntu/Debian tabanlı sistemler için:

```bash
# OpenCV, PCL ve Eigen kütüphanelerini yükle
sudo apt update
sudo apt install build-essential cmake libopencv-dev libpcl-dev libeigen3-dev

# FBX SDK'yi indirin ve kurun (Autodesk'ten manuel indirme gerekiyor)
# https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk
```

### Derleme

```bash
# Projeyi klonla
git clone https://github.com/kullanici/3d-model-creator.git
cd 3d-model-creator

# Build klasörü oluştur
mkdir build && cd build

# CMake yapılandır (FBX SDK dizini belirtilmeli)
cmake .. -DFBX_SDK_ROOT=/usr/local/fbxsdk

# Derle
make -j4
```

## Kullanım

```bash
# Temel kullanım
./bin/3DModelCreator --laser /scan/laser/ --color /scan/color/ --output output/model.fbx

# Tüm parametrelerle
./bin/3DModelCreator --laser /scan/laser/ --color /scan/color/ --output output/model.fbx --samples 360 --debug
```

### Komut Satırı Parametreleri

- `--laser <klasör>`: Lazer görüntüleri klasörü (varsayılan: /scan/laser/)
- `--color <klasör>`: Renk görüntüleri klasörü (varsayılan: /scan/color/)
- `--output <dosya>`: Çıktı FBX dosyası (varsayılan: output/model.fbx)
- `--samples <sayı>`: İşlenecek görüntü sayısı (varsayılan: 200)
- `--debug`: Debug modunu etkinleştirir (OpenCV pencerelerini gösterir)
- `--help`: Yardım mesajını gösterir

## Çıktı

Program, belirtilen yola bir FBX dosyası oluşturur. Bu dosya:

- 3D mesh geometrisi
- Vertex renkleri veya texture
- Unreal Engine 5 ile uyumlu FBX formatı

## Ek Notlar

- Tarama işleminde lazerin kameraya göre konumu önemlidir, kalibrasyon yapılması gerekebilir
- Lazer çizgisi tespiti için renk eşikleri ayarlanabilir
- FBX SDK sürümü ve bağlantı sorunları yaşanırsa, CMakeLists.txt içindeki FBX yolunu kontrol edin

## Proje Dosyaları

- `src/main.cpp`: Ana uygulama kodu
- `src/LaserLineDetector.cpp`: Lazer çizgisi tespiti
- `src/PointCloudBuilder.cpp`: 3D nokta bulutu oluşturma
- `src/MeshCreator.cpp`: Mesh oluşturma ve düzgünleştirme
- `src/ColorMapper.cpp`: Renk bilgisi uygulama
- `src/FBXExporter.cpp`: FBX formatında dışa aktarma

## Lisans

Bu proje [MIT Lisansı](LICENSE) altında lisanslanmıştır.