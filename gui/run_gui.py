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
    try:
        import paramiko
        import tkinter
        return True
    except ImportError as e:
        print(f"Missing dependency: {e}")
        return False

def install_dependencies():
    """Install required dependencies"""
    print("Installing required dependencies...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
        return True
    except subprocess.CalledProcessError:
        print("Failed to install dependencies. Please install manually:")
        print("pip install paramiko")
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
    except Exception as e:
        print(f"Error starting GUI: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main()) 