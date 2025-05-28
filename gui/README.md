# 3D Scanner GUI Application

A desktop application to control the 3D scanning process on Raspberry Pi via SSH.

## Features

- **SSH Connection Management**: Connect to Raspberry Pi with password or SSH key authentication
- **Remote Scanner Control**: Run scanner tests and full 3D scanning processes
- **Real-time Monitoring**: View scan progress and logs in real-time
- **File Management**: Automatically download generated 3D model files
- **Configuration Management**: Save and load connection settings

## Requirements

- Python 3.6 or higher
- tkinter (usually included with Python)
- paramiko (for SSH connectivity)

## Installation

1. **Install Python dependencies:**
   ```bash
   cd gui/
   pip install -r requirements.txt
   ```

2. **Or install manually:**
   ```bash
   pip install paramiko
   ```

## Usage

### Quick Start

1. **Run the application:**
   ```bash
   cd gui/
   python run_gui.py
   ```
   
   Or directly:
   ```bash
   python 3d_scanner_gui.py
   ```

2. **Configure connection** (Connection tab):
   - Enter Raspberry Pi IP address
   - Enter username (usually "pi")
   - Enter password or select SSH key file
   - Click "Connect"

3. **Start scanning** (3D Scanning tab):
   - Set number of samples (default: 100)
   - Click "Start Full Scan"
   - Monitor progress in the log window

4. **View results** (Output Files tab):
   - Files are automatically downloaded after scanning
   - Click "Open Output Folder" to view files
   - Use generated OBJ files for Unreal Engine import

### Application Tabs

#### 1. Connection Tab
- **IP Address**: Raspberry Pi's IP address on your network
- **Username**: SSH username (typically "pi")
- **Password**: SSH password
- **SSH Key File**: Alternative to password authentication
- **Connect/Disconnect**: Manage SSH connection
- **Test Connection**: Verify SSH connectivity

#### 2. 3D Scanning Tab
- **Number of samples**: Controls scan resolution (more = higher quality, longer time)
- **Start Full Scan**: Runs complete 3D scanning workflow:
  1. Captures laser and color images
  2. Processes images into 3D model
  3. Downloads output files
- **Test Scanner**: Tests scanner hardware without full scan
- **Stop Scan**: Requests scan termination
- **Progress Log**: Real-time output from scanning process

#### 3. Output Files Tab
- **Output Directory**: Local folder for downloaded files
- **File List**: Shows downloaded 3D model files with sizes and dates
- **Refresh File List**: Updates file display
- **Open Output Folder**: Opens folder in file explorer
- **Download Latest**: Manually download files from Raspberry Pi

#### 4. Settings Tab
- **Remote Paths**: Configure paths on Raspberry Pi
  - Test Script: Path to scanner test script
  - 3D Creator: Path to 3D model creation executable
  - Remote Output: Output directory on Raspberry Pi
- **Configuration**: Save/load/reset application settings

## Configuration

### Default Settings
```json
{
    "raspberry_ip": "192.168.1.100",
    "username": "pi",
    "password": "",
    "key_file": "",
    "remote_test_script": "/home/pi/scanner/test_scanner.py",
    "remote_3d_creator": "/home/pi/3d-model-creator/build/bin/3DModelCreator",
    "remote_output_dir": "/home/pi/3d-model-creator/build/output",
    "local_output_dir": "./output"
}
```

### SSH Key Authentication
For better security, use SSH key authentication:

1. Generate SSH key pair on your computer:
   ```bash
   ssh-keygen -t rsa -b 4096 -f ~/.ssh/pi_key
   ```

2. Copy public key to Raspberry Pi:
   ```bash
   ssh-copy-id -i ~/.ssh/pi_key.pub pi@192.168.1.100
   ```

3. In the GUI, browse and select the private key file (`~/.ssh/pi_key`)

## Output Files

The application generates these file types:

- **model.obj**: 3D geometry (recommended for Unreal Engine)
- **model.mtl**: Material definition
- **model.png**: Texture with vertex colors
- **model.ply**: Point cloud format (for 3D printing)

### Importing to Unreal Engine

1. Import the **OBJ file** into Unreal Engine
2. Enable "Import Materials" and "Import Textures" in import dialog
3. The MTL and PNG files will be automatically imported
4. The model will appear with proper geometry and colors

## Troubleshooting

### Connection Issues
- **"Connection refused"**: Check Raspberry Pi IP address and SSH service
- **"Authentication failed"**: Verify username/password or SSH key
- **"Host key verification failed"**: The application auto-accepts new host keys

### Scanning Issues
- **"Scanner test failed"**: Check hardware connections and camera/laser setup
- **"3D Creator failed"**: Verify the executable path in Settings tab
- **"No images found"**: Ensure scanner captures images to correct directories

### File Download Issues
- **"Download failed"**: Check network connection and remote file permissions
- **"Files not found"**: Verify remote output directory path in Settings

### Performance Tips
- **Reduce sample count** for faster scanning (lower quality)
- **Use SSH keys** instead of passwords for better security
- **Close other network applications** during file downloads

## Development

### File Structure
```
gui/
├── 3d_scanner_gui.py    # Main GUI application
├── run_gui.py           # Launcher script
├── requirements.txt     # Python dependencies
└── README.md           # This file
```

### Extending the Application
The GUI is modular and can be extended with:
- Additional scanning parameters
- Different file format support
- Advanced progress monitoring
- Batch scanning capabilities

## Support

For issues or questions:
1. Check the scan log for error messages
2. Verify all paths in Settings tab
3. Test SSH connection manually: `ssh pi@192.168.1.100`
4. Ensure Raspberry Pi has sufficient disk space 