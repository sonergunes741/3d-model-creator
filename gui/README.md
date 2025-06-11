# 3D Scanner GUI Application

A comprehensive desktop GUI application for controlling the entire 3D scanning workflow, from connecting to the Raspberry Pi scanner to viewing the generated 3D models.

## 🌟 Features

- **SSH Connectivity**: Secure connection to Raspberry Pi scanner
- **Real-time Progress Monitoring**: Live updates during scanning process
- **Automatic File Management**: Download and organize scan results
- **Configuration Management**: Save and load connection settings
- **Comprehensive Logging**: Detailed operation logs with timestamps


## 📋 Requirements

### System Requirements
- Python 3.7+
- tkinter (usually included with Python)
- Network access to Raspberry Pi

### Python Dependencies
```bash
pip install paramiko>=2.7.0
```

Or install via apt in Docker:
```bash
apt update && apt install python3-paramiko
```

## 🚀 Quick Start

### 1. Installation
```bash
# Clone or download the GUI directory
cd gui/

# Install dependencies
pip install -r requirements.txt
# OR in Docker:
# apt install python3-paramiko
```

### 2. Run the Application
```bash
# Method 1: Direct execution
python3 3d_scanner_gui.py

# Method 2: Using launcher (checks dependencies)
python3 run_gui.py
```

### 3. Configuration
1. **Connection Tab**: Enter Raspberry Pi details
   - Host: `realityshapers.local` (or IP address)
   - Username: `realityshapers`
   - Password: `rs123`
   - Port: `22`

2. **Test Connection**: Click "Test Connection" to verify

3. **Settings Tab**: Configure paths if needed
   - Remote paths: `/home/pi/photos/laser` and `/home/pi/photos/led`
   - Local paths: `scan/laser` and `scan/color`

## 🔄 Workflow

### Complete 3D Scanning Process:

1. **Connect to Pi** (Connection Tab)
   - Enter connection details
   - Test connection
   - Status shows "Connected ✅"

2. **Capture Images** (3D Scanning Tab)
   - Click "🔄 Start Full Scan"
   - Monitor progress in real-time

3. **Download & Process** (Automatic)
   - Photos downloaded from Pi
   - 3D model creator runs locally
   - Progress shown with detailed logs

4. **View Results** (Output Files Tab)
   - Browse generated files
   - Click "🎨 View 3D" for web viewer
   - Download files if needed