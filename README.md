# 3D Model Oluşturma Modülü

Bu modül, lazer ve LED aydınlatmalı fotoğrafları işleyerek nesnenin 3D modelini oluşturur ve OBJ formatında dışa aktarır.

## Özellikler

- Lazer çizgisi tespiti ve 3D nokta bulutu oluşturma
- Renkli görüntülerden renk bilgisi çıkarımı
- Poisson yüzey yeniden yapılandırması ile mesh üretimi
- Renk bilgisinin mesh'e uygulanması
- OBJ formatında model, MTL formatında materyal ve PNG formatında doku (texture) çıktısı

## Gereksinimler

- C++ Derleyici (C++11 veya üstü)
- CMake (3.10 veya üstü)
- OpenCV (4.x önerilen)
- PCL (Point Cloud Library) 1.8 veya üstü
- Eigen3 

## Kurulum

Uygulamayı iki farklı şekilde çalıştırabilirsiniz:

1. **GUI Uygulamasını Çalıştırma:**
   Eğer doğrudan kullanıcı arayüzü ile uygulamayı çalıştırmak istiyorsanız, terminal üzerinden:
   ```bash
   cd gui
   python3 run_gui.py
   ```
   komutlarını çalıştırmanız yeterlidir.

2. **3D Modelleme Algoritmasını Çalıştırma:**
   Eğer yalnızca 3D modelleme algoritmasını çalıştırmak istiyorsanız, aşağıdaki adımları izleyin:

### Bağımlılıkları Yükleme

Ubuntu/Debian tabanlı sistemler için:

```bash
# OpenCV, PCL ve Eigen kütüphanelerini yükle
sudo apt update
sudo apt install build-essential cmake libopencv-dev libpcl-dev libeigen3-dev
```

### Derleme

```bash
# Projeyi klonla
git clone https://github.com/kullanici/3d-model-creator.git
cd 3d-model-creator

# Build klasörü oluştur
mkdir build && cd build

# CMake yapılandır
cmake ..

# Derle
make -j4
```

### Uygulamayı Çalıştırma

Uygulamayı kullanırken sadece hazır bir veri seti ile modelleme yapmak isterseniz, [bu Google Drive linkinden](https://drive.google.com/drive/folders/1ks7azBOS-bTONS5KZfvoTU17vXnqz7C8?usp=sharing) `laser` ve `scan` klasörlerini indirin. Ardından, bu klasörleri `gui` klasöründeki `laser` ve `scan` klasörleriyle değiştirin. 

Eğer yalnızca 3D modelleme algoritmasını çalıştırmak istiyorsanız, bu klasörleri(drive'daki scan ve laser) projenin ana klasöründeki `scan` klasörü içindeki `laser` ve `color` klasörleriyle değiştirin.

## Kullanım

```bash
# Temel kullanım
./bin/3DModelCreator --laser ../scan/laser/ --color ../scan/color/ --output output/model.obj

# Region of Interest (ROI) seçerek kullanım
./bin/3DModelCreator --laser ../scan/laser/ --color ../scan/color/ --output output/model.obj --debug

# Threshold Adjust ile kullanım
./bin/3DModelCreator --laser ../scan/laser/ --color ../scan/color/ --output output/model.obj --debug --adjust-threshold

# Recommended
# Interactive mode ile Digital Image Processing teknikleri kullanımı
./bin/3DModelCreator --laser ../scan/laser/ --color ../scan/color/ --output output/model.obj --debug --interactive

# Tüm parametrelerle
./bin/3DModelCreator --laser /scan/laser/ --color /scan/color/ --output output/model.obj --samples 360 --debug
```

### Komut Satırı Parametreleri

- `--laser <klasör>`: Lazer görüntüleri klasörü (varsayılan: /scan/laser/)
- `--color <klasör>`: Renk görüntüleri klasörü (varsayılan: /scan/color/)
- `--output <dosya>`: Çıktı OBJ dosyası (varsayılan: output/model.obj)
- `--samples <sayı>`: İşlenecek görüntü sayısı (varsayılan: 200)
- `--debug`: Debug modunu etkinleştirir (OpenCV pencerelerini gösterir)
- `--help`: Yardım mesajını gösterir

## Çıktı

Program, belirtilen yola şu dosyayı oluşturur:

1. `model.obj`: 3D mesh geometrisi


## Ek Notlar

- Tarama işleminde lazerin kameraya göre konumu önemlidir, kalibrasyon yapılması gerekebilir
- Lazer çizgisi tespiti için renk eşikleri ayarlanabilir
- OBJ formatı yaygın olarak desteklenir ve FBX'e göre daha basittir

## Proje Dosyaları

- `src/main.cpp`: Ana uygulama kodu
- `src/LaserLineDetector.cpp`: Lazer çizgisi tespiti
- `src/PointCloudBuilder.cpp`: 3D nokta bulutu oluşturma
- `src/MeshCreator.cpp`: Mesh oluşturma ve düzgünleştirme
- `src/ColorMapper.cpp`: Renk bilgisi uygulama
- `src/OBJExporter.cpp`: OBJ/MTL/PNG formatında dışa aktarma

## Lisans

Bu proje [MIT Lisansı](LICENSE) altında lisanslanmıştır.
