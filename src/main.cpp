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
    bool interactiveMode = false; // İnteraktif mod için yeni değişken
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
        } else if (arg == "--interactive") {
            args.interactiveMode = true;
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
            std::cout << "  --interactive      İnteraktif lazer tespiti modunu etkinleştir" << std::endl;
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
    
    // Lazer rengi ayarla - bardağın yüzeyindeki lazer çizgisi için uygun değerler
    laserDetector.setThresholds(cv::Scalar(120, 20, 100), cv::Scalar(179, 255, 255));
    
    // Kontrast ve keskinlik ayarla
    laserDetector.setContrastEnhancement(2.5, 0);  // Daha güçlü kontrast
    
    // Blurlama ayarla
    laserDetector.setMedianBlur(5);  // 5x5 median blur
    laserDetector.setGaussianBlur(5, 1.5);  // 5x5 gaussian blur, sigma=1.5
    
    // Morfolojik işlemler ayarla
    laserDetector.setErosion(1, 3);  // 1 iterasyon, 3x3 kernel
    laserDetector.setDilation(2, 3);  // 2 iterasyon, 3x3 kernel
    
    // Debug modunu ayarla
    laserDetector.setDebugMode(args.debugMode);
    
    // ROI ayarla (eğer belirtildiyse)
    if (args.useROI) {
        laserDetector.setROI(args.roiX, args.roiY, args.roiWidth, args.roiHeight);
        std::cout << "ROI etkinleştirildi: x=" << args.roiX << ", y=" << args.roiY 
                 << ", genişlik=" << args.roiWidth << ", yükseklik=" << args.roiHeight << std::endl;
    }
    
    // İnteraktif mod kontrolü
    if (args.interactiveMode) {
        std::cout << "İnteraktif lazer tespiti modu başlatılıyor..." << std::endl;
        
        if (laserFiles.empty()) {
            std::cerr << "İnteraktif mod için görüntü bulunamadı!" << std::endl;
            return 1;
        }
        
        // İlk görüntüyü yükle
        cv::Mat firstImage = cv::imread(laserFiles[0]);
        
        if (firstImage.empty()) {
            std::cerr << "İlk lazer görüntüsü yüklenemedi: " << laserFiles[0] << std::endl;
            return 1;
        }
        
        // İnteraktif lazer optimizasyonu yap
        bool optimizationSuccess = laserDetector.optimizeROIAndDetectLaser(firstImage);
        
        if (!optimizationSuccess) {
            std::cout << "İnteraktif mod iptal edildi. Çıkılıyor..." << std::endl;
            return 0;
        }
        
        // Kullanıcıya devam etmek isteyip istemediğini sor
        std::cout << "Taramaya devam etmek istiyor musunuz? (e/h): ";
        char response;
        std::cin >> response;
        
        if (response != 'e' && response != 'E') {
            std::cout << "İşlem kullanıcı tarafından sonlandırıldı." << std::endl;
            return 0;
        }
    }
    
    PointCloudBuilder pointCloudBuilder;
    // Bardak için optimal tarama parametreleri
    pointCloudBuilder.setScanParameters(50.0f, 0.0f, 0.0f, 0.0f);
    
    // Optimum mesh kalitesi için 7 derinlik kullan (bardak gibi nesneler için)
    MeshCreator meshCreator(7);
    
    // Set aggressive smoothing parameters
    meshCreator.setSmoothingParameters(100, 0.0005f);
    
    ColorMapper colorMapper;
    OBJExporter objExporter;
    
    // Belirli aralıklarla görselleştirme için numaralar
    std::vector<int> previewIndices = {0, 40, 90, 140, 190}; // 1., 41., 91., 141., 191. görüntüler
    
    // İşlenecek toplam görüntü sayısı
    const int totalImages = args.sampleCount;
    int imageWidth = 0, imageHeight = 0;
    
    // Adım 1: Lazer çizgileri işle ve 3D nokta bulutu oluştur
    std::cout << "1. Aşama: Lazer çizgilerinden 3D nokta bulutu oluşturuluyor - Başladı" << std::endl;
    
    for (int i = 0; i < totalImages; i++) {
        // İlerleme göster
        if (i % 10 == 0) {
            std::cout << "İşlenen görüntü: " << i << "/" << totalImages << " (" 
                     << (i * 100 / totalImages) << "%)\r" << std::flush;
        }
        
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
            
            // ROI belirtilmediyse ve debug modu açıksa, kullanıcıdan ROI seçmesini iste
            if (args.debugMode && !args.useROI && !args.interactiveMode) {
                std::cout << "İlgi alanı (ROI) seçmek ister misiniz? (e/h): ";
                char response;
                std::cin >> response;
                
                if (response == 'e' || response == 'E') {
                    // Debug penceresi oluştur ve ROI seçimine hazırla
                    cv::Mat firstImage = laserImage.clone();
                    cv::namedWindow("Select ROI", cv::WINDOW_NORMAL);
                    cv::resizeWindow("Select ROI", 640, 480);
                    
                    std::cout << "Lütfen lazer çizgisini içeren bölgeyi seçin (fareyle dikdörtgen çizin)." << std::endl;
                    cv::Rect selectedROI = cv::selectROI("Select ROI", firstImage, false, false);
                    
                    // ROI'yi ayarla
                    laserDetector.setROI(selectedROI.x, selectedROI.y, selectedROI.width, selectedROI.height);
                    std::cout << "ROI seçildi: x=" << selectedROI.x << ", y=" << selectedROI.y 
                             << ", genişlik=" << selectedROI.width << ", yükseklik=" << selectedROI.height << std::endl;
                    
                    cv::destroyWindow("Select ROI");
                }
            }
        }
        
        // Belirli görüntüler için debug modu etkinleştir
        bool isPreviewIndex = std::find(previewIndices.begin(), previewIndices.end(), i) != previewIndices.end();
        laserDetector.setDebugMode(args.debugMode && isPreviewIndex);
        
        if (isPreviewIndex && args.debugMode) {
            std::cout << "Görüntü " << (i + 1) << " için lazer tespiti görselleştiriliyor..." << std::endl;
        }
        
        // Açı hesapla (her bir görüntü için 360 / totalImages derece dönüş)
        float angle = (float)i / totalImages * 360.0f;
        
        // Lazer çizgisini tespit et
        std::vector<cv::Point> laserLine = laserDetector.detectLaserLine(laserImage);

        if(args.debugMode){
            cv::Mat vis = laserImage.clone();
            for (const auto& pt : laserLine) {
                cv::circle(vis, pt, 2, cv::Scalar(0, 0, 255), -1);
            }
            cv::imshow("Lazer Line Detection", vis);
            cv::waitKey(1);
        }
        
        // Nokta bulutuna ekle
        size_t prevSize = pointCloudBuilder.pointImageInfo.size();
        pointCloudBuilder.addLineToCloud(laserLine, angle, imageWidth, imageHeight);
        size_t newSize = pointCloudBuilder.pointImageInfo.size();
        for (size_t idx = prevSize; idx < newSize; ++idx) {
            pointCloudBuilder.pointImageInfo[idx].imageIndex = i;
        }
    }
    
    std::cout << "\n1. Aşama: Lazer çizgilerinden 3D nokta bulutu oluşturuluyor - Tamamlandı" << std::endl;
    std::cout << "Nokta bulutu filtreleniyor..." << std::endl;
    
    // Nokta bulutunu filtrele ve hazırla
    pointCloudBuilder.processPointCloud();

    if(args.debugMode) {
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud = pointCloudBuilder.getCloud();
        pcl::visualization::CloudViewer viewer("Point Cloud Viewer");
        viewer.showCloud(cloud);
        std::cout << "Nokta bulutu görselleştiriliyor... Pencereyi kapatınca devam edecek..." << std::endl;
        while(!viewer.wasStopped()){
           //
        }
    }
    
    // Adım 2: Renk bilgisini işle
    std::cout << "2. Aşama: Renk bilgisi işleniyor - Başladı" << std::endl;
    
    // Store detected laser lines to pass to color mapper
    std::vector<std::vector<cv::Point>> allLaserLines(totalImages);

    // First pass to get all laser lines (re-use existing loop if possible or adapt)
    // This assumes laserFiles and totalImages are already defined and populated
    // We need to ensure laserDetector settings are appropriate for this pass as well
    // For simplicity, I am re-running the laser detection part here.
    // In a more optimized version, you would store laserLines from the first loop.

    std::cout << "Lazer çizgileri renk eşlemesi için yeniden tespit ediliyor..." << std::endl;
    for (int i = 0; i < totalImages; i++) {
        cv::Mat laserImage = cv::imread(laserFiles[i]);
        if (laserImage.empty()) {
            // Error handling as in the original loop
            std::cerr << "\nHata: Lazer görüntüsü yüklenemedi (renk eşlemesi için): " << laserFiles[i] << std::endl;
            allLaserLines[i] = {}; // Store empty line
            continue;
        }
        // Ensure laserDetector is in a non-debug state for this pass if it was changed before
        laserDetector.setDebugMode(false); // Or based on args.debugMode if you want to see it
        allLaserLines[i] = laserDetector.detectLaserLine(laserImage);
    }
    std::cout << "Lazer çizgileri renk eşlemesi için tespit edildi." << std::endl;

    for (int i = 0; i < totalImages; i++) {
        // İlerleme göster
        if (i % 10 == 0) {
            std::cout << "İşlenen renk görüntüsü: " << i << "/" << totalImages << " (" 
                     << (i * 100 / totalImages) << "% )\r" << std::flush;
        }
        
        // Renkli görüntüyü yükle
        cv::Mat colorImage = cv::imread(colorFiles[i]);
        
        if (colorImage.empty()) {
            std::cerr << "\nHata: Renk görüntüsü yüklenemedi: " << colorFiles[i] << std::endl;
            continue;
        }
        
        // Açı hesapla (her bir görüntü için 360 / totalImages derece dönüş)
        float angle = (float)i / totalImages * 360.0f;
        
        // Renk bilgisini ekle
        colorMapper.addColorData(colorImage, angle);
    }
    
    std::cout << "\n2. Aşama: Renk bilgisi işleniyor - Tamamlandı" << std::endl;
    
    // Adım 3: Mesh oluştur
    std::cout << "3. Aşama: 3D mesh oluşturuluyor - Başladı" << std::endl;
    
    // Nokta bulutunu al - getCloud() fonksiyonunu kullan
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud = pointCloudBuilder.getCloud();
    
    if (cloud->empty()) {
        std::cerr << "KRITIK HATA: Nokta bulutu boş! Model oluşturulamadı." << std::endl;
        return 1;
    }
    
    // Nokta bulutundan mesh oluştur
    meshCreator.setSmoothingParameters(100, 0.0005f); // 100 iterations, strong smoothing
    pcl::PolygonMesh mesh = meshCreator.createMesh(cloud);

    if (mesh.polygons.empty()) {
        std::cerr << "KRITIK HATA: Mesh oluşturulamadı! Hiç polygon yok." << std::endl;
        
        // Try a different mesh creation approach or parameters
        std::cout << "Alternatif mesh oluşturma yöntemi deneniyor..." << std::endl;
        meshCreator.setDepth(8);  // Adjust depth parameter
        meshCreator.setSmoothingParameters(100, 0.0005f);  // Disable smoothing
        mesh = meshCreator.createMesh(cloud);
        
        if (mesh.polygons.empty()) {
            std::cerr << "Alternatif yöntem de başarısız oldu. İşlem durduruluyor." << std::endl;
            return 1;
        }
    }
    
    std::cout << "3. Aşama: 3D mesh oluşturuluyor - Tamamlandı" << std::endl;
    std::cout << "Oluşturulan mesh polygon sayısı: " << mesh.polygons.size() << std::endl;
    
    // Adım 4: Mesh'e renk bilgisi uygula
    std::cout << "4. Aşama: Mesh renklendiriliyor - Başladı" << std::endl;
    
    // Nokta bulutundan renk bilgisini al
    colorMapper.applyColorToMesh(mesh, cloud, pointCloudBuilder.pointImageInfo);
    
    std::cout << "4. Aşama: Mesh renklendiriliyor - Tamamlandı" << std::endl;
    
    // Adım 5: OBJ olarak dışa aktar
    std::cout << "5. Aşama: OBJ/MTL/PNG dosyaları oluşturuluyor - Başladı" << std::endl;
    
    // Texture boyutunu ayarla (OBJExporter'a texture çözünürlüğü ayarı için method eklenmeli)
    objExporter.setUseVertexColors(true);
    objExporter.setTextureResolution(2048, 2048);
    
    // Mesh'i OBJ olarak dışa aktar
    std::string modelName = fs::path(args.outputPath).stem().string();
    bool exportSuccess = objExporter.exportMesh(mesh, args.outputPath, modelName);
    
    std::cout << "5. Aşama: OBJ/MTL/PNG dosyaları oluşturuluyor - Tamamlandı" << std::endl;
    
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
