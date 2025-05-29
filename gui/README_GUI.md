# 3D Scanner GUI Application

This directory contains a self-contained GUI application for controlling the 3D scanning workflow from Raspberry Pi to Windows.

## Directory Structure

```
gui/
├── 3d_scanner_gui.py          # Main Python GUI application
├── ModelViewer.h              # C++ OpenGL viewer header
├── ModelViewer.cpp            # C++ OpenGL viewer implementation
├── viewer_main.cpp            # C++ viewer main executable
├── CMakeLists.txt             # CMake build configuration
├── build_viewer.sh            # Build script for C++ viewer
├── requirements.txt           # Python dependencies
├── run_gui.py                 # GUI launcher with dependency checking
├── README.md                  # Main GUI documentation
└── README_GUI.md              # This file
```

## Quick Start

### 1. Build the C++ Viewer (First Time Only)

```bash
cd gui
./build_viewer.sh
```

This will:
- Install OpenGL dependencies
- Build the C++ ModelViewer executable
- Place it in `./build/bin/ModelViewer`

### 2. Install Python Dependencies

```bash
pip install -r requirements.txt
```

### 3. Run the GUI

```bash
python3 3d_scanner_gui.py
# or
python3 run_gui.py
```

## Features

### Self-Contained Design
- All C++ viewer files are in the GUI directory
- Independent build system using CMake
- No dependencies on parent project structure
- GUI manages its own output directory

### Workflow
1. **Connect** to Raspberry Pi via SSH
2. **Scan & Download** photos from Pi to local directories
3. **Create Model** using local 3D creator executable
4. **View 3D** using built-in C++ OpenGL viewer

### File Locations
- **Photos**: `./scan/laser/` and `./scan/color/`
- **Models**: `./output/` (OBJ, MTL, PNG, PLY files)
- **Viewer**: `./build/bin/ModelViewer`

## C++ Viewer Controls

- **Mouse**: Left click + drag to rotate
- **Scroll**: Zoom in/out
- **W**: Toggle wireframe
- **S**: Toggle solid faces
- **R**: Reset camera
- **ESC**: Exit viewer

## Configuration

The GUI saves configuration in `scanner_config.json` including:
- SSH connection details
- Remote Raspberry Pi paths
- Local file paths
- Viewer executable location

## Troubleshooting

### C++ Viewer Not Found
```bash
cd gui
./build_viewer.sh
```

### Missing OpenGL Libraries
```bash
sudo apt-get install libglfw3-dev libglew-dev libglm-dev libgl1-mesa-dev libglu1-mesa-dev
```

### Python Dependencies Missing
```bash
pip install paramiko matplotlib numpy
```

## Independence

This GUI application is completely independent and can:
- Run without the main 3D creator project
- Be copied to any directory
- Work with any 3D creator executable (configurable path)
- Use its own C++ viewer for fast 3D visualization 