#!/usr/bin/env python3
"""
Launcher script for 3D Scanner GUI
Checks dependencies and runs the application
"""

import sys
import subprocess
import os

def check_dependencies():
    """Check if required dependencies are installed"""
    missing_deps = []
    
    try:
        import paramiko
    except ImportError:
        missing_deps.append("paramiko")
    
    try:
        import tkinter
    except ImportError:
        missing_deps.append("tkinter")
    
    try:
        import matplotlib
    except ImportError:
        missing_deps.append("matplotlib")
    
    try:
        import numpy
    except ImportError:
        missing_deps.append("numpy")
    
    if missing_deps:
        print(f"Missing dependencies: {', '.join(missing_deps)}")
        return False
    
    return True

def install_dependencies():
    """Install required dependencies"""
    print("Installing required dependencies...")
    try:
        # Try normal pip install first
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
        return True
    except subprocess.CalledProcessError as e:
        # Check if it's an externally-managed environment error
        if "externally-managed-environment" in str(e) or "externally managed" in str(e):
            print("\nExternally-managed environment detected.")
            print("Trying alternative installation methods...")
            
            # Try with --break-system-packages flag
            try:
                subprocess.check_call([sys.executable, "-m", "pip", "install", "--break-system-packages", "-r", "requirements.txt"])
                print("✅ Dependencies installed successfully!")
                return True
            except subprocess.CalledProcessError:
                pass
            
            # Try with --user flag
            try:
                subprocess.check_call([sys.executable, "-m", "pip", "install", "--user", "-r", "requirements.txt"])
                print("✅ Dependencies installed successfully!")
                return True
            except subprocess.CalledProcessError:
                pass
        
        # If all methods fail, provide manual installation instructions
        print("❌ Failed to install dependencies automatically.")
        print("\nPlease install manually using one of these methods:")
        print("1. With --break-system-packages:")
        print("   pip install --break-system-packages paramiko matplotlib numpy")
        print("2. With --user flag:")
        print("   pip install --user paramiko matplotlib numpy")
        print("3. Using system package manager:")
        print("   sudo apt install python3-paramiko python3-matplotlib python3-numpy")
        print("4. Using virtual environment:")
        print("   python3 -m venv venv && source venv/bin/activate && pip install paramiko matplotlib numpy")
        return False

def main():
    print("3D Scanner GUI Launcher")
    print("=" * 30)
    
    # Check if we're in the right directory
    if not os.path.exists("3d_scanner_gui.py"):
        print("Error: 3d_scanner_gui.py not found!")
        print("Please run this script from the gui/ directory")
        return 1
    
    # Check dependencies
    if not check_dependencies():
        print("\nSome dependencies are missing.")
        response = input("Would you like to install them automatically? (y/n): ")
        if response.lower() in ['y', 'yes']:
            if not install_dependencies():
                return 1
        else:
            print("Please install the required dependencies manually and try again.")
            return 1
    
    # Run the GUI application
    print("\nStarting 3D Scanner GUI...")
    try:
        import importlib.util
        spec = importlib.util.spec_from_file_location("scanner_gui", "3d_scanner_gui.py")
        scanner_gui = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(scanner_gui)
        scanner_gui.main()
    except KeyboardInterrupt:
        print("\nKeyboard interrupt received. Shutting down gracefully...")
        return 0
    except Exception as e:
        print(f"Error starting GUI: {e}")
        return 1
    
    print("GUI application closed.")
    return 0

if __name__ == "__main__":
    sys.exit(main()) 