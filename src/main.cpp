#include <iostream>
#include <string>
#include <vector>
#include <filesystem>  // C++17 için
#include <opencv2/opencv.hpp>

#include "LaserLineDetector.h"
#include "PointCloudBuilder.h"
#include "MeshCreator.h"
#include "ColorMapper.h"
#include "OBJExporter.h"

namespace fs = std::filesystem;  // C++17 ile kullanılabilir

// Komut satırı argümanları
struct CommandLineArgs {
    std::string laserFolder = "/scan/laser/";
    std::string colorFolder = "/scan/color/";
    std::string outputPath = "output/model.obj";
    int sampleCount = 200;
    bool debugMode = false;
    bool useROI = false;
    int roiX = 0, roiY = 0, roiWidth = 0, roiHeight = 0;
};

// Komut satırı argümanları işleme
CommandLineArgs parseCommandLine(int argc, char** argv) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--laser" && i + 1 < argc) {
            args.laserFolder = argv[++i];
        } else if (arg == "--color" && i + 1 < argc) {
            args.colorFolder = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            args.outputPath = argv[++i];
        } else if (arg == "--samples" && i + 1 < argc) {
            args.sampleCount = std::stoi(argv[++i]);
        } else if (arg == "--debug") {
            args.debugMode = true;
        } else if (arg == "--roi" && i + 4 < argc) {
            args.useROI = true;
            args.roiX = std::stoi(argv[++i]);
            args.roiY = std::stoi(argv[++i]);
            args.roiWidth = std::stoi(argv[++i]);
            args.roiHeight = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "3D Model Oluşturma Modülü" << std::endl;
            std::cout << "Kullanım: " << argv[0] << " [seçenekler]" << std::endl;
            std::cout << "Seçenekler:" << std::endl;
            std::cout << "  --laser <klasör>   Lazer görüntüleri klasörü (varsayılan: /scan/laser/)" << std::endl;
            std::cout << "  --color <klasör>   Renk görüntüleri klasörü (varsayılan: /scan/color/)" << std::endl;
            std::cout << "  --output <dosya>   Çıktı OBJ dosyası (varsayılan: output/model.obj)" << std::endl;
            std::cout << "  --samples <sayı>   İşlenecek görüntü sayısı (varsayılan: 200)" << std::endl;
            std::cout << "  --debug            Debug modunu etkinleştir" << std::endl;
            std::cout << "  --roi x y w h      İlgi bölgesini (ROI) belirle (x,y: sol üst köşe, w,h: genişlik ve yükseklik)" << std::endl;
            std::cout << "  --help             Bu yardım mesajını göster" << std::endl;
            exit(0);
        }
    }
    
    return args;
}

// Klasördeki görüntü dosyalarını yükle
std::vector<std::string> getImageFiles(const std::string& folderPath) {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                // JPG, JPEG, PNG formatlarını kabul et
                if (extension == ".jpg" || extension == ".jpeg" || extension == ".png") {
                    files.push_back(entry.path().string());
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Klasör okuma hatası: " << e.what() << std::endl;
    }
    
    // Dosyaları sırayla işlemek için alfabetik olarak sırala
    std::sort(files.begin(), files.end());
    
    return files;
}

// Çıktı klasörünü oluştur
void createOutputDirectory(const std::string& outputPath) {
    fs::path outputDir = fs::path(outputPath).parent_path();
    
    try {
        if (!fs::exists(outputDir)) {
            fs::create_directories(outputDir);
            std::cout << "Çıktı klasörü oluşturuldu: " << outputDir.string() << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Klasör oluşturma hatası: " << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    // Komut satırı argümanlarını işle
    CommandLineArgs args = parseCommandLine(argc, argv);
    
    // Görüntü dosyalarını bul
    std::vector<std::string> laserFiles = getImageFiles(args.laserFolder);
    std::vector<std::string> colorFiles = getImageFiles(args.colorFolder);
    
    if (laserFiles.empty() || colorFiles.empty()) {
        std::cerr << "Görüntü dosyaları bulunamadı!" << std::endl;
        return 1;
    }
    
    // Görüntü sayısını kontrol et
    if (laserFiles.size() < args.sampleCount || colorFiles.size() < args.sampleCount) {
        std::cout << "Uyarı: İstenilen görüntü sayısı (" << args.sampleCount 
                 << ") mevcut dosya sayısından fazla." << std::endl;
        args.sampleCount = std::min(laserFiles.size(), colorFiles.size());
        std::cout << "Toplam " << args.sampleCount << " görüntü işlenecek." << std::endl;
    }
    
    // Çıktı klasörünü oluştur
    createOutputDirectory(args.outputPath);
    
    // Bileşenleri oluştur
    LaserLineDetector laserDetector;
    
    // Daha düşük eşik değerleri ayarla
    laserDetector.setThresholds(cv::Scalar(140, 30, 30), cv::Scalar(179, 255, 255));
    
    // Kontrast artırmayı ayarla
    laserDetector.setContrastEnhancement(2.0, 0);  // Daha güçlü kontrast
    
    // ROI ayarla (eğer belirtildiyse)
    if (args.useROI) {
        laserDetector.setROI(args.roiX, args.roiY, args.roiWidth, args.roiHeight);
        std::cout << "ROI etkinleştirildi: x=" << args.roiX << ", y=" << args.roiY 
                 << ", genişlik=" << args.roiWidth << ", yükseklik=" << args.roiHeight << std::endl;
    }
    
    PointCloudBuilder pointCloudBuilder;
    MeshCreator meshCreator;
    ColorMapper colorMapper;
    OBJExporter objExporter;
    
    // Debug modu kontrolü
    laserDetector.setDebugMode(args.debugMode);
    
    // İşlenecek toplam görüntü sayısı
    const int totalImages = args.sampleCount;
    int imageWidth = 0, imageHeight = 0;
    
    // Adım 1: Lazer çizgileri işle ve 3D nokta bulutu oluştur
    std::cout << "1. Aşama: Lazer çizgilerinden 3D nokta bulutu oluşturuluyor..." << std::endl;
    
    for (int i = 0; i < totalImages; i++) {
        // İlerleme yüzdesi
        float progress = (float)i / totalImages * 100.0f;
        std::cout << "\rİlerleme: %" << progress << " (" << i + 1 << "/" << totalImages << ")" << std::flush;
        
        // Lazer görüntüsünü yükle
        cv::Mat laserImage = cv::imread(laserFiles[i]);
        
        if (laserImage.empty()) {
            std::cerr << "\nHata: Lazer görüntüsü yüklenemedi: " << laserFiles[i] << std::endl;
            continue;
        }
        
        // İlk görüntüden boyutları al
        if (i == 0) {
            imageWidth = laserImage.cols;
            imageHeight = laserImage.rows;
            
            // ROI belirtilmediyse ve ilk görüntüde debug modu açıksa, kullanıcıdan ROI seçmesini iste
            if (args.debugMode && !args.useROI) {
                std::cout << "\nİlgi alanı (ROI) seçmek ister misiniz? (e/h): ";
                char response;
                std::cin >> response;
                
                if (response == 'e' || response == 'E') {
                    // Debug penceresi oluştur ve ROI seçimine hazırla
                    cv::Mat firstImage = laserImage.clone();
                    cv::namedWindow("Select ROI", cv::WINDOW_NORMAL);
                    cv::resizeWindow("Select ROI", 800, 600);
                    cv::imshow("Select ROI", firstImage);
                    
                    std::cout << "\nLütfen lazer çizgisini içeren bölgeyi seçin (fareyle dikdörtgen çizin)." << std::endl;
                    cv::Rect selectedROI = cv::selectROI("Select ROI", firstImage, false, false);
                    
                    // ROI'yi ayarla
                    laserDetector.setROI(selectedROI.x, selectedROI.y, selectedROI.width, selectedROI.height);
                    std::cout << "\nROI seçildi: x=" << selectedROI.x << ", y=" << selectedROI.y 
                             << ", genişlik=" << selectedROI.width << ", yükseklik=" << selectedROI.height << std::endl;
                    
                    cv::destroyWindow("Select ROI");
                }
            }
        }
        
        // Açı hesapla (her bir görüntü için 360 / totalImages derece dönüş)
        float angle = (float)i / totalImages * 360.0f;
        
        // Lazer çizgisini tespit et
        std::vector<cv::Point> laserLine = laserDetector.detectLaserLine(laserImage);
        
        // Nokta bulutuna ekle
        pointCloudBuilder.addLineToCloud(laserLine, angle, imageWidth, imageHeight);
    }
    
    std::cout << "\nNokta bulutu tamamlandı, filtreleniyor..." << std::endl;
    
    // Nokta bulutunu filtrele ve hazırla
    pointCloudBuilder.processPointCloud();
    
    // Adım 2: Renk bilgisini işle
    std::cout << "\n2. Aşama: Renk bilgisi işleniyor..." << std::endl;
    
    for (int i = 0; i < totalImages; i++) {
        // İlerleme yüzdesi
        float progress = (float)i / totalImages * 100.0f;
        std::cout << "\rİlerleme: %" << progress << " (" << i + 1 << "/" << totalImages << ")" << std::flush;
        
        // Renkli görüntüyü yükle
        cv::Mat colorImage = cv::imread(colorFiles[i]);
        
        if (colorImage.empty()) {
            std::cerr << "\nHata: Renk görüntüsü yüklenemedi: " << colorFiles[i] << std::endl;
            continue;
        }
        
        // Açı hesapla (her bir görüntü için 360 / totalImages derece dönüş)
        float angle = (float)i / totalImages * 360.0f;
        
        // Renk bilgisini ekle
        colorMapper.addColorData(colorImage, angle, imageWidth, imageHeight);
    }
    
    // Adım 3: Mesh oluştur
    std::cout << "\n3. Aşama: 3D mesh oluşturuluyor..." << std::endl;
    
    // Nokta bulutundan mesh oluştur
    pcl::PolygonMesh mesh = meshCreator.createMesh(pointCloudBuilder.getPointCloud());
    
    // Adım 4: Mesh'e renk bilgisi uygula
    std::cout << "4. Aşama: Mesh renklendiriliyor..." << std::endl;
    
    // Nokta bulutundan renk bilgisini al
    colorMapper.applyColorToMesh(mesh, pointCloudBuilder.getPointCloud());
    
    // Adım 5: OBJ olarak dışa aktar
    std::cout << "5. Aşama: OBJ/MTL/PNG dosyaları oluşturuluyor..." << std::endl;
    
    // Mesh'i OBJ olarak dışa aktar
    objExporter.setTextureResolution(2048, 2048); // Yüksek çözünürlüklü texture
    bool exportSuccess = objExporter.exportMesh(mesh, args.outputPath);
    
    if (exportSuccess) {
        std::cout << "\nİşlem başarıyla tamamlandı!" << std::endl;
        std::cout << "3D model oluşturuldu: " << args.outputPath << std::endl;
        std::cout << "(.obj, .mtl ve .png dosyaları aynı dizinde)" << std::endl;
    } else {
        std::cerr << "\nOBJ dosyası oluşturulurken bir hata oluştu!" << std::endl;
        return 1;
    }
    
    return 0;
}