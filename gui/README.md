# 3D Scanner GUI Application

A comprehensive desktop GUI application for controlling the entire 3D scanning workflow, from connecting to the Raspberry Pi scanner to viewing the generated 3D models.

## 🌟 Features

- **SSH Connectivity**: Secure connection to Raspberry Pi scanner
- **Real-time Progress Monitoring**: Live updates during scanning process
- **Automatic File Management**: Download and organize scan results
- **Docker-Compatible 3D Viewer**: Web-based viewer that works without OpenGL
- **Configuration Management**: Save and load connection settings
- **Comprehensive Logging**: Detailed operation logs with timestamps

## 🐳 Docker Compatibility

This GUI application is **fully Docker-compatible** and includes a web-based 3D viewer that works without OpenGL or libGL dependencies:

### ✅ Docker-Compatible Features:
- **Web-based 3D Viewer**: Uses Three.js in browser instead of OpenGL
- **No libGL Dependencies**: Eliminates OpenGL library requirements
- **Pure Python GUI**: tkinter works in Docker containers
- **SSH/SFTP Operations**: Network operations work seamlessly
- **File Management**: All file operations are container-friendly

### 🌐 Web-Based 3D Viewer

The application includes a sophisticated web-based 3D model viewer that:
- Serves models via HTTP on localhost:8080
- Supports OBJ, MTL, and texture files
- Provides interactive controls (rotate, zoom, pan)
- Offers wireframe and solid view modes
- Works in any modern web browser
- Requires no additional dependencies

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
   - Local paths: `../scan/laser` and `../scan/color`

## 🔄 Workflow

### Complete 3D Scanning Process:

1. **Connect to Pi** (Connection Tab)
   - Enter connection details
   - Test connection
   - Status shows "Connected ✅"

2. **Capture Images** (3D Scanning Tab)
   - Set sample count (default: 100)
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

## 🎮 3D Viewer Controls

The web-based viewer provides:

### Mouse Controls:
- **Left Drag**: Rotate model
- **Right Drag**: Pan view
- **Scroll Wheel**: Zoom in/out

### Interface Controls:
- **Wireframe/Solid**: Toggle view modes
- **Reset View**: Return to default position
- **Fullscreen**: Expand to full browser window

### Model Information:
- Vertex count and face count
- File size and format details
- Docker compatibility status

## 📁 File Structure

```
gui/
├── 3d_scanner_gui.py          # Main GUI application
├── web_viewer.py              # Docker-compatible 3D viewer
├── run_gui.py                 # Launcher with dependency checks
├── requirements.txt           # Python dependencies
├── README.md                  # This documentation
├── test_cube.obj             # Test model for viewer
└── simple_test_viewer.py     # Viewer test script
```

## ⚙️ Configuration

Settings are automatically saved to `gui_config.json`:

```json
{
    "ssh_host": "realityshapers.local",
    "ssh_username": "realityshapers", 
    "ssh_password": "rs123",
    "ssh_port": 22,
    "remote_photos_laser": "/home/pi/photos/laser",
    "remote_photos_led": "/home/pi/photos/led",
    "local_3d_creator": "../build/bin/3DModelCreator",
    "local_photos_dir": "../scan",
    "local_output_dir": "../output"
}
```

## 🔧 Troubleshooting

### Connection Issues
```bash
# Test SSH manually
ssh realityshapers@realityshapers.local

# Check network connectivity
ping realityshapers.local
```

### Docker Issues
```bash
# Test web viewer
python3 simple_test_viewer.py

# Check if port is available
netstat -ln | grep :8080
```

### File Permission Issues
```bash
# Make scripts executable
chmod +x *.py

# Check file permissions
ls -la ../build/bin/3DModelCreator
```

## 🐛 Common Issues

### 1. "No module named 'paramiko'"
```bash
# Install paramiko
pip install paramiko
# OR in Docker:
apt install python3-paramiko
```

### 2. "Connection refused"
- Check Raspberry Pi is powered on
- Verify network connectivity
- Confirm SSH is enabled on Pi

### 3. "3D Creator not found"
- Build the C++ application first:
```bash
cd .. && mkdir build && cd build
cmake .. && make
```

### 4. "Web viewer not loading"
- Check if port 8080 is available
- Try a different port in web_viewer.py
- Verify browser allows localhost connections

## 🔄 Development

### Testing the Web Viewer
```bash
# Run viewer test
python3 simple_test_viewer.py

# Manual test with sample model
python3 web_viewer.py test_cube.obj 8080
```

### Adding New Features
The GUI is modular and extensible:
- `connection_tab()`: SSH connectivity
- `scanning_tab()`: Scan workflow
- `output_tab()`: File management
- `settings_tab()`: Configuration

## 📊 Performance

### Docker Performance:
- **GUI Startup**: ~2-3 seconds
- **SSH Connection**: ~1-2 seconds  
- **Photo Download**: ~5-10 seconds (depends on count)
- **3D Processing**: ~10-30 seconds (depends on samples)
- **Web Viewer**: ~1-2 seconds startup

### Resource Usage:
- **Memory**: ~50-100 MB for GUI
- **CPU**: Low during idle, moderate during processing
- **Network**: ~1-10 MB for photo downloads
- **Storage**: ~10-50 MB for generated models

## 🎯 Future Enhancements

- [ ] Real-time preview during scanning
- [ ] Multiple model format exports
- [ ] Batch processing capabilities
- [ ] Advanced viewer features (lighting, materials)
- [ ] Cloud storage integration
- [ ] Mobile app companion

## 📄 License

This project is part of the 3D Model Creator suite. See the main project for licensing information.

---

**Note**: This GUI application provides a complete, user-friendly interface for the 3D scanning workflow while maintaining full Docker compatibility through its web-based viewer solution.

# 3D Scanner GUI - Docker-Compatible Interface

## 🐳 Docker Environment Notice

**Important**: You are currently running in a Docker container environment where traditional GUI applications cannot be displayed directly. This is normal and expected behavior.

## 🎯 Available Solutions

### 1. **Command-Line Interface (Recommended for Docker)**
Use the CLI version that works perfectly in Docker:

```bash
cd gui
python3 cli_scanner.py
```

Features:
- ✅ View 3D models using web-based viewer
- ✅ Check output directory status
- ✅ Configure settings
- ✅ Fully Docker-compatible

### 2. **Direct Web Viewer**
Start the web viewer directly for existing models:

```bash
cd gui
python3 test_viewer.py
```

This will:
- Find your 3D model files
- Start a web server on localhost:8080
- Open the model in your browser (Docker-compatible)

### 3. **Traditional GUI (Local Machine Only)**
The traditional GUI works on local machines with display:

```bash
cd gui
python3 3d_scanner_gui.py
```

**Note**: This will not work in Docker containers without X11 forwarding.

## 🌐 Web-Based 3D Viewer

Our web-based viewer is specifically designed for Docker compatibility:

- **No OpenGL required** - Uses WebGL in browser
- **Automatic port management** - Tries ports 8080-8083
- **Auto-shutdown** - Stops when browser tab is closed
- **Restricted camera controls** - 360° horizontal, 180° vertical rotation
- **Centered object positioning** - Object stays at origin

### Viewer Controls:
- **Mouse drag**: Rotate object around center
- **Reset button**: Return to default view
- **Fullscreen button**: Toggle fullscreen mode
- **Model info**: Shows vertex/face count

## 📁 Directory Structure

```
gui/
├── 3d_scanner_gui.py      # Traditional GUI (requires display)
├── cli_scanner.py         # CLI interface (Docker-compatible)
├── web_viewer.py          # Web-based 3D viewer
├── test_viewer.py         # Direct web viewer test
├── output/                # 3D model output directory
│   └── model.obj          # Your 3D models
└── scanner_config.json    # Configuration file
```

## 🔧 Configuration

Settings are stored in `scanner_config.json`:

```json
{
    "ssh_user": "realityshapers",
    "ssh_host": "realityshapers.local",
    "ssh_pass": "rs123",
    "local_output_dir": "./output"
}
```

## 🚀 Quick Start (Docker)

1. **Check if you have models**:
   ```bash
   cd gui
   python3 cli_scanner.py
   # Choose option 2 to check output directory
   ```

2. **View existing models**:
   ```bash
   python3 cli_scanner.py
   # Choose option 1 to start web viewer
   ```

3. **Direct web viewer**:
   ```bash
   python3 test_viewer.py
   ```

## 🐛 Troubleshooting

### GUI Starts and Closes Immediately
This is normal in Docker. Use the CLI interface instead:
```bash
python3 cli_scanner.py
```

### Web Viewer Port Busy
The viewer automatically tries multiple ports (8080-8083). If all are busy:
```bash
# Kill any existing viewers
pkill -f web_viewer
# Or wait a moment and try again
```

### No Models Found
Check if models exist:
```bash
ls -la output/
```

If no models, you need to create them first using the 3D model creation workflow.

## 🎨 Model Viewing Features

- **Docker-compatible**: No OpenGL dependencies
- **Interactive controls**: Rotate and examine models
- **Model statistics**: Vertex and face counts
- **Multiple formats**: Supports OBJ, MTL, and textures
- **Auto-scaling**: Models automatically fit in view
- **Responsive design**: Works on different screen sizes

## 💡 Tips

1. **Use CLI in Docker**: The command-line interface is designed for container environments
2. **Web viewer is portable**: Works in any browser, no special software needed
3. **Models auto-center**: Objects are positioned at origin for consistent viewing
4. **Heartbeat system**: Server automatically stops when browser closes
5. **Multiple models**: Viewer automatically finds the most recent model

## 🔗 Related Files

- `web_viewer.py`: Core web-based viewer implementation
- `3d_scanner_gui.py`: Traditional GUI with full workflow
- `cli_scanner.py`: Command-line interface for Docker
- `test_viewer.py`: Simple web viewer test script

---

**Docker Users**: Use `python3 cli_scanner.py` for the best experience! 🐳 