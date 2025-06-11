# 3D Tarayıcı GUI Uygulaması

Raspberry Pi tarayıcıya bağlanmaktan oluşturulan 3D modelleri görüntülemeye kadar tüm 3D tarama iş akışını kontrol eden kapsamlı bir masaüstü GUI uygulaması.

## 🌟 Özellikler

- **SSH Bağlantısı**: Raspberry Pi tarayıcıya güvenli bağlantı
- **Gerçek Zamanlı İlerleme İzleme**: Tarama sürecinde canlı güncellemeler
- **Otomatik Dosya Yönetimi**: Tarama sonuçlarını indirme ve düzenleme
- **Yapılandırma Yönetimi**: Bağlantı ayarlarını kaydetme ve yükleme
- **Kapsamlı Günlük Kaydı**: Zaman damgalı detaylı işlem günlükleri

## 📋 Gereksinimler

### Sistem Gereksinimleri
- Python 3.7+
- tkinter (genellikle Python ile birlikte gelir)
- Raspberry Pi'ye ağ erişimi

### Python Bağımlılıkları
```bash
pip install paramiko>=2.7.0
```


## 🚀 Hızlı Başlangıç

### 1. Kurulum
```bash
# GUI dizinini klonlayın veya indirin
cd gui/

# Bağımlılıkları yükleyin
pip install -r requirements.txt
```

### 2. Uygulamayı Çalıştırın
```bash
# Yöntem 1: Doğrudan çalıştırma
python3 3d_scanner_gui.py

# Yöntem 2: Başlatıcı kullanarak (bağımlılıkları kontrol eder)
python3 run_gui.py
```

### 3. Yapılandırma
1. **Bağlantı Sekmesi**: Raspberry Pi detaylarını girin
   - Host: `realityshapers.local` (veya IP adresi)
   - Kullanıcı Adı: `realityshapers`
   - Şifre: `rs123`
   - Port: `5000`

2. **Bağlantıyı Test Et**: Doğrulamak için "Test Connection" düğmesine tıklayın

3. **Ayarlar Sekmesi**: Gerekirse yolları yapılandırın
   - Uzak yollar: `/home/pi/photos/laser` ve `/home/pi/photos/led`
   - Yerel yollar: `scan/laser` ve `scan/color`

## 🔄 İş Akışı

### Tam 3D Tarama Süreci:

1. **Pi'ye Bağlan** (Bağlantı Sekmesi)
   - Bağlantı detaylarını girin
   - Bağlantıyı test edin
   - Durum "Connected ✅" gösterir

2. **Görüntüleri Yakala** (3D Tarama Sekmesi)
   - "🔄 Start Full Scan" düğmesine tıklayın
   - İlerlemeyi gerçek zamanlı olarak izleyin

3. **İndir ve İşle** (Otomatik)
   - Fotoğraflar Pi'den indirilir
   - 3D model oluşturucu yerel olarak çalışır
   - İlerleme detaylı log lar ile gösterilir

4. **Sonuçları Görüntüle** (Çıktı Dosyaları Sekmesi)
   - Oluşturulan dosyalara göz atın
   - Web görüntüleyici için "🎨 View 3D" düğmesine tıklayın