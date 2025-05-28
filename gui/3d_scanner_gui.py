#!/usr/bin/env python3
"""
3D Scanner GUI Application
Raspberry Pi → Windows Workflow Controller
Based on laser_scan.sh bash script
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import paramiko
import threading
import os
import time
import subprocess
from datetime import datetime
import json

class ScannerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("LASER SCANNER & 3D MODEL CREATOR - Raspberry Pi → Windows Workflow")
        self.root.geometry("900x700")
        self.root.resizable(True, True)
        
        # SSH connection variables
        self.ssh_client = None
        
        # Status variables (matching bash script)
        self.connection_status = False
        self.scan_status = False
        self.scanning_active = False  # New: indicates scanning is in progress
        self.download_status = False
        self.model_status = False
        self.model_active = False  # New: indicates model creation is in progress
        
        # Configuration (matching bash script paths)
        self.config = {
            # Raspberry Pi connection
            "ssh_user": "realityshapers",
            "ssh_host": "realityshapers.local",
            "ssh_pass": "rs123",
            
            # Raspberry Pi remote paths
            "remote_scanner_project": "/home/realityshapers/Desktop/scanner_project",
            "remote_lazer_path": "/home/realityshapers/Desktop/scanner_project/lazer",
            "remote_leds_path": "/home/realityshapers/Desktop/scanner_project/leds",
            
            # Windows local paths (self-contained within gui/)
            "local_3d_creator": "./bin/3DModelCreator",
            "local_scan_dir": "./scan",
            "local_laser_dir": "./scan/laser",
            "local_color_dir": "./scan/color",
            "local_build_dir": ".",
            "local_output_dir": "./output"
        }
        
        self.setup_ui()
        self.load_config()
        self.update_status_display()
        
    def setup_ui(self):
        # Create main frame with banner
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Banner
        self.setup_banner(main_frame)
        
        # Status display
        self.setup_status_display(main_frame)
        
        # Main workflow buttons
        self.setup_workflow_buttons(main_frame)
        
        # Log and additional controls
        self.setup_log_and_controls(main_frame)
        
    def setup_banner(self, parent):
        banner_frame = ttk.LabelFrame(parent, text="", padding=10)
        banner_frame.pack(fill=tk.X, pady=(0, 10))
        
        title_label = ttk.Label(banner_frame, text="LASER SCANNER & 3D MODEL CREATOR", 
                               font=("Arial", 16, "bold"), foreground="purple")
        title_label.pack()
        
        subtitle_label = ttk.Label(banner_frame, text="Raspberry Pi → Windows Workflow", 
                                  font=("Arial", 12), foreground="blue")
        subtitle_label.pack()
        
    def setup_status_display(self, parent):
        status_frame = ttk.LabelFrame(parent, text="WORKFLOW STATUS", padding=10)
        status_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Connection info
        info_frame = ttk.Frame(status_frame)
        info_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Label(info_frame, text="Target Device:", font=("Arial", 10, "bold")).pack(side=tk.LEFT)
        self.connection_info_var = tk.StringVar()
        ttk.Label(info_frame, textvariable=self.connection_info_var, foreground="blue").pack(side=tk.LEFT, padx=(5, 0))
        
        # Status grid
        status_grid = ttk.Frame(status_frame)
        status_grid.pack(fill=tk.X)
        
        # Status labels
        self.status_labels = {}
        status_items = [
            ("1. CONNECTION", "connection_status"),
            ("2. SCAN & DOWNLOAD", "scan_status"),
            ("3. 3D MODEL", "model_status")
        ]
        
        for i, (label, status_key) in enumerate(status_items):
            row = i // 2
            col = (i % 2) * 2
            
            ttk.Label(status_grid, text=label, font=("Arial", 10, "bold")).grid(row=row, column=col, sticky=tk.W, padx=5, pady=2)
            
            status_var = tk.StringVar()
            self.status_labels[status_key] = status_var
            status_label = ttk.Label(status_grid, textvariable=status_var, font=("Arial", 10))
            status_label.grid(row=row, column=col+1, sticky=tk.W, padx=5, pady=2)
        
    def setup_workflow_buttons(self, parent):
        workflow_frame = ttk.LabelFrame(parent, text="MAIN WORKFLOW", padding=10)
        workflow_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Button grid
        button_grid = ttk.Frame(workflow_frame)
        button_grid.pack()
        
        # Main workflow buttons
        self.connect_btn = ttk.Button(button_grid, text="1. CONNECT", command=self.connect_to_pi, width=15)
        self.connect_btn.grid(row=0, column=0, padx=5, pady=5)
        
        self.scan_btn = ttk.Button(button_grid, text="2. SCAN & DOWNLOAD", command=self.run_laser_scan, width=20, state=tk.DISABLED)
        self.scan_btn.grid(row=0, column=1, padx=5, pady=5)
        
        self.model_btn = ttk.Button(button_grid, text="3. CREATE MODEL", command=self.create_3d_model, width=15, state=tk.DISABLED)
        self.model_btn.grid(row=0, column=2, padx=5, pady=5)
        
        # Additional control buttons
        control_frame = ttk.Frame(workflow_frame)
        control_frame.pack(pady=(10, 0))
        
        ttk.Button(control_frame, text="STATUS", command=self.show_detailed_status, width=12).pack(side=tk.LEFT, padx=2)
        ttk.Button(control_frame, text="DIAGNOSIS", command=self.network_diagnosis, width=12).pack(side=tk.LEFT, padx=2)
        ttk.Button(control_frame, text="RESET", command=self.reset_status, width=12).pack(side=tk.LEFT, padx=2)
        ttk.Button(control_frame, text="FOLDERS", command=self.check_local_folders, width=12).pack(side=tk.LEFT, padx=2)
        ttk.Button(control_frame, text="SETTINGS", command=self.show_settings, width=12).pack(side=tk.LEFT, padx=2)
        
    def setup_log_and_controls(self, parent):
        # Progress frame
        progress_frame = ttk.LabelFrame(parent, text="PROGRESS", padding=10)
        progress_frame.pack(fill=tk.BOTH, expand=True)
        
        # Progress bar and status
        progress_top = ttk.Frame(progress_frame)
        progress_top.pack(fill=tk.X, pady=(0, 10))
        
        self.progress_var = tk.StringVar(value="Ready - Click CONNECT to start")
        ttk.Label(progress_top, textvariable=self.progress_var, font=("Arial", 10, "bold")).pack(side=tk.LEFT)
        
        self.progress_bar = ttk.Progressbar(progress_top, mode='indeterminate')
        self.progress_bar.pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(10, 0))
        
        # Log output
        log_frame = ttk.Frame(progress_frame)
        log_frame.pack(fill=tk.BOTH, expand=True)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=15, width=80, font=("Consolas", 9))
        self.log_text.pack(fill=tk.BOTH, expand=True)
        
        
    def update_status_display(self):
        """Update all status displays"""
        # Update connection info
        self.connection_info_var.set(f"{self.config['ssh_user']}@{self.config['ssh_host']}")
        
        # Check if local photos exist (for enabling CREATE MODEL button)
        local_photos_exist = self.check_local_photos_exist()
        
        # Update status labels
        statuses = {
            "connection_status": ("✅ Connected" if self.connection_status else "❌ Not Connected", 
                                "green" if self.connection_status else "red"),
            "scan_status": ("✅ Completed" if self.download_status else 
                           ("🔄 Active" if self.scanning_active else 
                            ("📁 Local Photos Available" if local_photos_exist else "⏸️ Pending")), 
                           "green" if self.download_status else 
                           ("blue" if self.scanning_active else 
                            ("blue" if local_photos_exist else "orange"))), 
            "model_status": ("✅ Created" if self.model_status else 
                            ("🔄 Active" if self.model_active else "⏸️ Pending"), 
                            "green" if self.model_status else 
                            ("blue" if self.model_active else "orange"))
        }
        
        for status_key, (text, color) in statuses.items():
            if status_key in self.status_labels:
                self.status_labels[status_key].set(text)
        
        # Update button states
        self.scan_btn.config(state=tk.NORMAL if self.connection_status else tk.DISABLED)
        # Enable CREATE MODEL if photos downloaded OR if local photos exist
        self.model_btn.config(state=tk.NORMAL if (self.download_status or local_photos_exist) else tk.DISABLED)
        
        # Update connect button text
        self.connect_btn.config(text="✅ CONNECTED" if self.connection_status else "1. CONNECT")
        
    def check_local_photos_exist(self):
        """Check if local photos exist for both laser and color"""
        try:
            laser_dir = self.config['local_laser_dir']
            color_dir = self.config['local_color_dir']
            
            # Check if directories exist
            if not (os.path.exists(laser_dir) and os.path.exists(color_dir)):
                return False
            
            # Count image files in each directory
            laser_photos = [f for f in os.listdir(laser_dir) 
                          if f.lower().endswith(('.jpg', '.jpeg', '.png'))]
            color_photos = [f for f in os.listdir(color_dir) 
                          if f.lower().endswith(('.jpg', '.jpeg', '.png'))]
            
            # Return True if both directories have photos
            return len(laser_photos) > 0 and len(color_photos) > 0
            
        except Exception:
            return False
    
    def log_message(self, message, level="INFO"):
        """Add message to log with timestamp and color coding"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        
        # Color coding based on message content
        if "✅" in message or "SUCCESS" in message.upper():
            prefix = "✅"
        elif "❌" in message or "ERROR" in message.upper() or "FAILED" in message.upper():
            prefix = "❌"
        elif "⚠️" in message or "WARNING" in message.upper():
            prefix = "⚠️"
        elif "🔄" in message or "PROCESSING" in message.upper():
            prefix = "🔄"
        elif "📥" in message or "DOWNLOAD" in message.upper():
            prefix = "📥"
        elif "🔴" in message or "SCAN" in message.upper():
            prefix = "🔴"
        elif "🏗️" in message or "BUILD" in message.upper():
            prefix = "🏗️"
        else:
            prefix = "ℹ️"
        
        formatted_message = f"[{timestamp}] {prefix} {message}\n"
        self.log_text.insert(tk.END, formatted_message)
        self.log_text.see(tk.END)
        self.root.update_idletasks()
    
    def connect_to_pi(self):
        """Connect to Raspberry Pi via SSH"""
        if self.connection_status:
            messagebox.showinfo("Already Connected", "Already connected to Raspberry Pi!")
            return
        
        # Check if password is available
        if not self.config.get('ssh_pass') or self.config['ssh_pass'] == "":
            # Prompt for password
            password = self.prompt_for_password()
            if not password:
                return  # User cancelled
            self.config['ssh_pass'] = password
            
        def connect_thread():
            try:
                self.progress_bar.start()
                self.progress_var.set("Connecting to Raspberry Pi...")
                
                self.log_message("🔄 Connecting to Raspberry Pi...")
                self.log_message(f"   Target: {self.config['ssh_user']}@{self.config['ssh_host']}")
                
                self.ssh_client = paramiko.SSHClient()
                self.ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
                
                # Try to connect
                self.ssh_client.connect(
                    hostname=self.config['ssh_host'],
                    username=self.config['ssh_user'],
                    password=self.config['ssh_pass'],
                    timeout=15
                )
                
                # Test connection
                stdin, stdout, stderr = self.ssh_client.exec_command("echo 'Connection test successful'")
                result = stdout.read().decode().strip()
                
                if result:
                    self.connection_status = True
                    self.log_message("✅ CONNECTION SUCCESSFUL!")
                    self.progress_var.set("Connected to Raspberry Pi")
                    messagebox.showinfo("Success", "Successfully connected to Raspberry Pi!")
                else:
                    raise Exception("Connection test failed")
                    
            except Exception as e:
                self.log_message(f"❌ Connection failed: {str(e)}")
                self.progress_var.set("Connection failed")
                messagebox.showerror("Connection Error", f"Failed to connect to Raspberry Pi:\n{str(e)}")
            finally:
                self.progress_bar.stop()
                self.update_status_display()
        
        threading.Thread(target=connect_thread, daemon=True).start()
    
    def prompt_for_password(self):
        """Prompt user for SSH password"""
        password_window = tk.Toplevel(self.root)
        password_window.title("SSH Password Required")
        password_window.geometry("400x200")
        password_window.resizable(False, False)
        
        # Center the window
        password_window.transient(self.root)
        password_window.grab_set()
        
        # Variables
        password_var = tk.StringVar()
        result = [None]  # Use list to store result from inner function
        
        # Main frame
        main_frame = ttk.Frame(password_window, padding=20)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Info
        ttk.Label(main_frame, text="SSH Password Required", font=("Arial", 12, "bold")).pack(pady=(0, 10))
        ttk.Label(main_frame, text=f"Connecting to: {self.config['ssh_user']}@{self.config['ssh_host']}", 
                 font=("Arial", 10)).pack(pady=(0, 15))
        
        # Password entry
        pass_frame = ttk.Frame(main_frame)
        pass_frame.pack(fill=tk.X, pady=(0, 15))
        
        ttk.Label(pass_frame, text="Password:", font=("Arial", 10, "bold")).pack(side=tk.LEFT)
        pass_entry = ttk.Entry(pass_frame, textvariable=password_var, show="*", width=25, font=("Arial", 10))
        pass_entry.pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(10, 0))
        
        # Show password checkbox
        show_var = tk.BooleanVar()
        def toggle_show():
            if show_var.get():
                pass_entry.config(show="")
            else:
                pass_entry.config(show="*")
        
        ttk.Checkbutton(main_frame, text="Show password", variable=show_var, command=toggle_show).pack(anchor=tk.W)
        
        # Buttons
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=(15, 0))
        
        def on_ok():
            password = password_var.get().strip()
            if password:
                result[0] = password
                password_window.destroy()
            else:
                messagebox.showwarning("Invalid Input", "Please enter a password!")
        
        def on_cancel():
            result[0] = None
            password_window.destroy()
        
        def on_settings():
            password_window.destroy()
            self.show_settings()
        
        ttk.Button(button_frame, text="✅ Connect", command=on_ok, width=12).pack(side=tk.LEFT, padx=(0, 5))
        ttk.Button(button_frame, text="⚙️ Settings", command=on_settings, width=12).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="❌ Cancel", command=on_cancel, width=12).pack(side=tk.RIGHT)
        
        # Enter key binding
        def on_enter(event):
            on_ok()
        
        pass_entry.bind('<Return>', on_enter)
        pass_entry.focus_set()
        
        # Wait for window to close
        password_window.wait_window()
        
        return result[0]
    
    def run_laser_scan(self):
        """Run laser scanning on Raspberry Pi"""
        if not self.connection_status:
            messagebox.showwarning("Not Connected", "Please connect to Raspberry Pi first!")
            return
        
        # Check if local photos exist and require user confirmation for deletion
        if self.check_local_photos_exist():
            laser_count = len([f for f in os.listdir(self.config['local_laser_dir']) 
                             if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
            color_count = len([f for f in os.listdir(self.config['local_color_dir']) 
                             if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
            
            result = messagebox.askyesno(
                "Delete Existing Photos?", 
                f"Local photos already exist:\n"
                f"• Laser photos: {laser_count}\n"
                f"• Color photos: {color_count}\n\n"
                f"To start scanning, all existing photos must be deleted:\n"
                f"Do you want to delete all existing photos and start scanning?"
            )
            
            if not result:
                self.log_message("⏸️ Scanning cancelled by user - photos not deleted")
                return
            
        def scan_thread():
            try:
                # Disable all buttons except FOLDERS and STATUS
                self.disable_buttons_during_scan()
                
                # Set scanning as active
                self.scanning_active = True
                self.update_status_display()
                
                self.progress_bar.start()
                self.progress_var.set("Deleting old photos and starting scan...")
                
                self.log_message("🔴 LASER SCANNING STARTED")
                
                # Delete local photos first (if user confirmed)
                if self.check_local_photos_exist():
                    self.log_message("   Deleting all files in local directories...")
                    laser_dir = self.config['local_laser_dir']
                    color_dir = self.config['local_color_dir']
                    
                    for folder in [laser_dir, color_dir]:
                        if os.path.exists(folder):
                            for file in os.listdir(folder):
                                file_path = os.path.join(folder, file)
                                if os.path.isfile(file_path):  # Only delete files, not subdirectories
                                    os.remove(file_path)
                    
                    self.log_message("   ✅ All local files deleted")
                
                # Delete remote photos
                self.log_message("   Deleting old remote photos...")
                clean_cmd = f"rm -f {self.config['remote_lazer_path']}/* {self.config['remote_leds_path']}/*"
                stdin, stdout, stderr = self.ssh_client.exec_command(clean_cmd)
                stdout.read()  # Wait for completion
                self.log_message("   ✅ Remote photos deleted")
                
                # Now start the scanning process
                self.log_message("   Starting Raspberry Pi scanner program...")
                self.progress_var.set("Running laser scan on Raspberry Pi...")
                
                # Run scanner program
                self.log_message("   Running scanner program...")
                self.log_message("   (This may take several minutes)")
                
                scan_cmd = f"cd {self.config['remote_scanner_project']} && sudo ./test"
                stdin, stdout, stderr = self.ssh_client.exec_command(scan_cmd)
                
                # Read output in real-time
                for line in iter(stdout.readline, ""):
                    if line:
                        self.log_message(f"Scanner: {line.strip()}")
                
                exit_code = stdout.channel.recv_exit_status()
                
                if exit_code == 0:
                    self.scan_status = True
                    self.log_message("✅ LASER SCANNING COMPLETED!")
                    self.check_photo_count()
                    
                    # Automatically start download process
                    self.log_message("📥 STARTING AUTOMATIC DOWNLOAD...")
                    self.progress_var.set("Downloading photos...")
                    self.download_photos_internal()
                    
                else:
                    error_output = stderr.read().decode()
                    raise Exception(f"Scanner failed with exit code {exit_code}: {error_output}")
                    
            except Exception as e:
                self.log_message(f"❌ Scanning failed: {str(e)}")
                self.progress_var.set("Scanning failed")
                messagebox.showerror("Scan Error", f"Laser scanning failed:\n{str(e)}")
            finally:
                # Set scanning as inactive
                self.scanning_active = False
                self.progress_bar.stop()
                # Re-enable buttons after scanning completes
                self.enable_buttons_after_scan()
                self.update_status_display()
        
        threading.Thread(target=scan_thread, daemon=True).start()
    
    def check_photo_count(self):
        """Check number of photos captured"""
        try:
            self.log_message("📊 Checking captured photos...")
            
            # Count laser photos
            laser_cmd = f"find {self.config['remote_lazer_path']} -name '*.jpg' -o -name '*.png' | wc -l"
            stdin, stdout, stderr = self.ssh_client.exec_command(laser_cmd)
            laser_count = int(stdout.read().decode().strip())
            
            # Count LED photos
            led_cmd = f"find {self.config['remote_leds_path']} -name '*.jpg' -o -name '*.png' | wc -l"
            stdin, stdout, stderr = self.ssh_client.exec_command(led_cmd)
            led_count = int(stdout.read().decode().strip())
            
            self.log_message(f"   Laser photos: {laser_count}")
            self.log_message(f"   LED photos: {led_count}")
            
            if laser_count > 0 and led_count > 0:
                self.log_message("✅ Photos captured successfully!")
            else:
                self.log_message("⚠️ Fewer photos than expected")
                
        except Exception as e:
            self.log_message(f"⚠️ Could not check photo count: {str(e)}")
    
    def download_photos_internal(self):
        """Internal method to download photos (called automatically after scanning)"""
        try:
            self.log_message("📥 DOWNLOADING PHOTOS TO WINDOWS")
            
            # Create local directories
            self.create_local_directories()
            
            # Photos were already cleaned at scan start, just download to empty directories
            laser_dir = self.config['local_laser_dir']
            color_dir = self.config['local_color_dir']
            
            # Download using SFTP
            sftp = self.ssh_client.open_sftp()
            
            # Download laser photos
            self.log_message("   Downloading laser photos...")
            try:
                laser_files = sftp.listdir(self.config['remote_lazer_path'])
                laser_count = 0
                for filename in laser_files:
                    if filename.lower().endswith(('.jpg', '.jpeg', '.png')):
                        remote_path = f"{self.config['remote_lazer_path']}/{filename}"
                        local_path = os.path.join(laser_dir, filename)
                        sftp.get(remote_path, local_path)
                        laser_count += 1
                self.log_message(f"   Downloaded {laser_count} laser photos")
            except Exception as e:
                self.log_message(f"⚠️ Laser photo download warning: {str(e)}")
            
            # Download LED photos to color directory
            self.log_message("   Downloading LED photos...")
            try:
                led_files = sftp.listdir(self.config['remote_leds_path'])
                led_count = 0
                for filename in led_files:
                    if filename.lower().endswith(('.jpg', '.jpeg', '.png')):
                        remote_path = f"{self.config['remote_leds_path']}/{filename}"
                        local_path = os.path.join(color_dir, filename)
                        sftp.get(remote_path, local_path)
                        led_count += 1
                self.log_message(f"   Downloaded {led_count} LED photos")
            except Exception as e:
                self.log_message(f"⚠️ LED photo download warning: {str(e)}")
            
            sftp.close()
            
            # Verify downloads
            local_laser_count = len([f for f in os.listdir(laser_dir) 
                                   if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
            local_color_count = len([f for f in os.listdir(color_dir) 
                                   if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
            
            self.log_message(f"   Final count - Laser: {local_laser_count}, Color: {local_color_count}")
            
            if local_laser_count > 0 and local_color_count > 0:
                self.download_status = True
                self.log_message("✅ PHOTOS DOWNLOADED SUCCESSFULLY!")
                self.log_message(f"   Laser: {laser_dir}")
                self.log_message(f"   Color: {color_dir}")
                self.progress_var.set("Scan and download completed - Ready to create model")
                messagebox.showinfo("Success", "Laser scanning and photo download completed successfully!")
            else:
                raise Exception("No photos were downloaded")
                
        except Exception as e:
            self.log_message(f"❌ Download failed: {str(e)}")
            self.progress_var.set("Download failed")
            raise e  # Re-raise to be caught by the scan thread
    
    def create_local_directories(self):
        """Create local directory structure"""
        self.log_message("📁 Creating local directories...")
        
        # Create scan directories
        scan_directories = [
            self.config['local_scan_dir'],
            self.config['local_laser_dir'],
            self.config['local_color_dir']
        ]
        
        for directory in scan_directories:
            os.makedirs(directory, exist_ok=True)
        
        # Create output directory
        os.makedirs(self.config['local_output_dir'], exist_ok=True)
        
        # Check if 3D creator executable exists
        if os.path.exists(self.config['local_3d_creator']):
            self.log_message("   ✅ 3DModelCreator executable found")
        else:
            self.log_message("   ⚠️ 3DModelCreator executable not found - will be built")
        
        self.log_message("   ✅ Local directories ready")
    
    def create_3d_model(self):
        """Create 3D model using downloaded photos"""
        # Check if photos exist (either downloaded or already present)
        if not self.check_local_photos_exist():
            messagebox.showwarning("No Photos Found", "Please scan and download photos first, or ensure local photos exist!")
            return
        
        # Show photo status
        laser_count = len([f for f in os.listdir(self.config['local_laser_dir']) 
                         if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
        color_count = len([f for f in os.listdir(self.config['local_color_dir']) 
                         if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
        
        photo_source = "downloaded from Raspberry Pi" if self.download_status else "found locally"
        
        result = messagebox.askyesno(
            "Create 3D Model", 
            f"Ready to create 3D model using photos {photo_source}:\n\n"
            f"• Laser photos: {laser_count}\n"
            f"• Color photos: {color_count}\n\n"
            f"This process may take several minutes.\n\n"
            f"Continue?"
        )
        
        if not result:
            self.log_message("⏸️ 3D model creation cancelled by user")
            return
        
        def model_thread():
            try:
                # Disable all buttons except FOLDERS and STATUS
                self.disable_buttons_during_scan()
                
                # Set model creation as active
                self.model_active = True
                self.update_status_display()
                
                self.progress_bar.start()
                self.progress_var.set("Creating 3D model...")
                
                self.log_message("🏗️ CREATING 3D MODEL")
                self.log_message(f"   Using photos {photo_source}")
                self.log_message(f"   Laser photos: {laser_count}")
                self.log_message(f"   Color photos: {color_count}")
                self.log_message("   Windows model creator will run...")
                
                # Clean old output files
                self.log_message("   Cleaning old model files...")
                output_dir = self.config['local_output_dir']
                if os.path.exists(output_dir):
                    for file in os.listdir(output_dir):
                        if file.endswith('.obj'):
                            os.remove(os.path.join(output_dir, file))
                
                # Check if executable exists
                executable_path = self.config['local_3d_creator']
                if not os.path.exists(executable_path):
                    raise Exception(f"3DModelCreator executable not found at: {executable_path}\n"
                                  f"Please ensure the executable is in the correct location.")
                
                self.log_message("   ✅ 3DModelCreator executable found")
                
                # Run 3D Model Creator with real-time output streaming
                self.log_message("   Creating 3D model...")
                self.log_message("   (This may take several minutes)")
                
                # Use interactive ROI selection workflow
                executable_path = os.path.abspath(self.config['local_3d_creator'])
                
                creator_cmd = [
                    executable_path,
                    '--interactive'
                ]
                
                # Debug logging
                self.log_message(f"   Executable: {executable_path}")
                self.log_message(f"   Using default scan directories (./scan/laser and ./scan/color)")
                self.log_message(f"   Interactive ROI selection enabled")
                self.log_message(f"   Debug mode enabled for visualization")
                
                # Run 3D Model Creator with real-time output streaming
                process = subprocess.Popen(
                    creator_cmd, 
                    stdout=subprocess.PIPE, 
                    stderr=subprocess.STDOUT,
                    text=True,
                    bufsize=1,
                    universal_newlines=True
                )
                
                # Stream output in real-time
                while True:
                    output = process.stdout.readline()
                    if output == '' and process.poll() is not None:
                        break
                    if output:
                        self.log_message(f"Creator: {output.strip()}")
                        self.root.update_idletasks()  # Force GUI update
                
                # Get the return code
                return_code = process.poll()
                
                if return_code != 0:
                    # Get any remaining error output
                    remaining_output = process.stdout.read()
                    if remaining_output:
                        self.log_message(f"Creator Error: {remaining_output}")
                    raise Exception(f"3D Model Creator failed with exit code {return_code}")
                
                # Check if model files were created
                output_files = []
                # Check the default 3D creator output directory
                default_output_dir = '../build/output'
                
                if os.path.exists(default_output_dir):
                    for file in os.listdir(default_output_dir):
                        if file.endswith(('.obj', '.mtl', '.png', '.ply')):
                            output_files.append(file)
                    output_dir_used = default_output_dir
                else:
                    # Fallback to GUI configured output directory
                    output_dir_used = self.config['local_output_dir']
                    if os.path.exists(output_dir_used):
                        for file in os.listdir(output_dir_used):
                            if file.endswith(('.obj', '.mtl', '.png', '.ply')):
                                output_files.append(file)
                
                if output_files:
                    self.model_status = True
                    self.log_message("✅ 3D MODEL CREATED SUCCESSFULLY!")
                    self.log_message("📄 Generated files:")
                    for file in output_files:
                        file_path = os.path.join(output_dir_used, file)
                        size = os.path.getsize(file_path)
                        self.log_message(f"   • {file} ({size} bytes)")
                    
                    self.progress_var.set("3D model created successfully")
                    messagebox.showinfo("Success", f"3D model created successfully!\n\nGenerated files:\n" + "\n".join(output_files))
                else:
                    raise Exception("No output files were generated")
                    
            except Exception as e:
                self.log_message(f"❌ Model creation failed: {str(e)}")
                self.progress_var.set("Model creation failed")
                messagebox.showerror("Model Error", f"3D model creation failed:\n{str(e)}")
            finally:
                # Set model creation as inactive
                self.model_active = False
                self.progress_bar.stop()
                # Re-enable buttons after model creation completes
                self.enable_buttons_after_scan()
                self.update_status_display()
        
        threading.Thread(target=model_thread, daemon=True).start()
    
    def show_detailed_status(self):
        """Show detailed status information"""
        status_window = tk.Toplevel(self.root)
        status_window.title("Detailed Status")
        status_window.geometry("500x400")
        
        text_widget = scrolledtext.ScrolledText(status_window, font=("Consolas", 10))
        text_widget.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        status_text = f"""
DETAILED WORKFLOW STATUS
{'=' * 50}

Raspberry Pi Target: {self.config['ssh_user']}@{self.config['ssh_host']}

1. Connection    : {'✅ Connected' if self.connection_status else '❌ Not Connected'}
2. Scanning      : {'✅ Completed' if self.scan_status else '⏸️ Pending'}
3. Download      : {'✅ Completed' if self.download_status else '⏸️ Pending'}
4. 3D Model      : {'✅ Created' if self.model_status else '⏸️ Pending'}

Working Directory: {os.getcwd()}

LOCAL FOLDER STATUS:
{'=' * 30}
"""
        
        # Check local folders
        folders_to_check = [
            ("3D Creator Executable", self.config['local_3d_creator']),
            ("Scan Directory", self.config['local_scan_dir']),
            ("Laser Photos", self.config['local_laser_dir']),
            ("Color Photos", self.config['local_color_dir']),
            ("Output Directory", self.config['local_output_dir'])
        ]
        
        for name, path in folders_to_check:
            if os.path.exists(path):
                if os.path.isdir(path):
                    if name == "Scan Directory":
                        # Count only subdirectories (laser, color)
                        subdirs = [d for d in os.listdir(path) 
                                 if os.path.isdir(os.path.join(path, d)) and not d.startswith('.')]
                        file_count = len(subdirs)
                        status_text += f"✅ {name}: {file_count} subdirectories\n"
                    elif "Photos" in name:
                        # Count only image files
                        images = [f for f in os.listdir(path) 
                                if f.lower().endswith(('.jpg', '.jpeg', '.png')) and not f.startswith('.')]
                        file_count = len(images)
                        status_text += f"✅ {name}: {file_count} photos\n"
                    elif name == "Output Directory":
                        # Count only model files
                        models = [f for f in os.listdir(path) 
                                if f.endswith(('.obj', '.ply', '.fbx', '.mtl', '.png')) and not f.startswith('.')]
                        file_count = len(models)
                        status_text += f"✅ {name}: {file_count} model files\n"
                    else:
                        # General count (excluding hidden files)
                        items = [f for f in os.listdir(path) if not f.startswith('.')]
                        file_count = len(items)
                        status_text += f"✅ {name}: {file_count} items\n"
                else:
                    status_text += f"✅ {name}: exists\n"
            else:
                status_text += f"❌ {name}: NOT FOUND\n"
        
        # Check output files
        output_dir = self.config['local_output_dir']
        if os.path.exists(output_dir):
            obj_files = [f for f in os.listdir(output_dir) if f.endswith('.obj')]
            if obj_files:
                status_text += f"\nOUTPUT FILES:\n"
                for obj_file in obj_files:
                    file_path = os.path.join(output_dir, obj_file)
                    size = os.path.getsize(file_path)
                    status_text += f"• {obj_file} ({size} bytes)\n"
        
        text_widget.insert(tk.END, status_text)
        text_widget.config(state=tk.DISABLED)
    
    def network_diagnosis(self):
        """Run network diagnosis"""
        diag_window = tk.Toplevel(self.root)
        diag_window.title("Network Diagnosis")
        diag_window.geometry("600x400")
        
        text_widget = scrolledtext.ScrolledText(diag_window, font=("Consolas", 10))
        text_widget.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        def run_diagnosis():
            text_widget.insert(tk.END, "🔍 NETWORK CONNECTION DIAGNOSIS\n")
            text_widget.insert(tk.END, "=" * 50 + "\n\n")
            
            text_widget.insert(tk.END, f"🎯 Target Device:\n")
            text_widget.insert(tk.END, f"   Hostname: {self.config['ssh_host']}\n")
            text_widget.insert(tk.END, f"   Username: {self.config['ssh_user']}\n\n")
            
            # Test SSH port
            text_widget.insert(tk.END, "🔌 SSH Port Test:\n")
            try:
                import socket
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                result = sock.connect_ex((self.config['ssh_host'], 22))
                sock.close()
                
                if result == 0:
                    text_widget.insert(tk.END, "   ✅ SSH port (22) is open\n")
                else:
                    text_widget.insert(tk.END, "   ❌ SSH port is closed or unreachable\n")
            except Exception as e:
                text_widget.insert(tk.END, f"   ❌ Port test failed: {str(e)}\n")
            
            text_widget.insert(tk.END, "\n🛠️ RECOMMENDATIONS:\n")
            text_widget.insert(tk.END, "   1. Check that Raspberry Pi is powered on\n")
            text_widget.insert(tk.END, "   2. Verify network connection\n")
            text_widget.insert(tk.END, "   3. Ensure SSH service is running\n")
            text_widget.insert(tk.END, "   4. Check firewall settings\n")
            
            text_widget.see(tk.END)
        
        run_diagnosis()
    
    def reset_status(self):
        """Reset all status variables"""
        if messagebox.askyesno("Reset Status", "Are you sure you want to reset all status variables?"):
            self.connection_status = False
            self.scan_status = False
            self.scanning_active = False
            self.download_status = False
            self.model_status = False
            self.model_active = False
            
            if self.ssh_client:
                self.ssh_client.close()
                self.ssh_client = None
            
            self.update_status_display()
            self.progress_var.set("Status reset - Ready to start")
            self.log_message("✅ All status variables reset!")
    
    def check_local_folders(self):
        """Check and display local folder status"""
        folder_window = tk.Toplevel(self.root)
        folder_window.title("Local Folders Status")
        folder_window.geometry("600x500")
        
        text_widget = scrolledtext.ScrolledText(folder_window, font=("Consolas", 10))
        text_widget.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        text_widget.insert(tk.END, "📁 LOCAL FOLDER STATUS\n")
        text_widget.insert(tk.END, "=" * 50 + "\n\n")
        
        # Check 3D Creator executable
        if os.path.exists(self.config['local_3d_creator']):
            text_widget.insert(tk.END, f"✅ 3DModelCreator: {self.config['local_3d_creator']}\n")
        else:
            text_widget.insert(tk.END, f"⚠️ 3DModelCreator: NOT FOUND (will be built)\n")
        
        # Check scan directories
        laser_dir = self.config['local_laser_dir']
        if os.path.exists(laser_dir):
            laser_count = len([f for f in os.listdir(laser_dir) 
                             if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
            text_widget.insert(tk.END, f"✅ Laser folder: {laser_count} photos\n")
        else:
            text_widget.insert(tk.END, f"⚠️ Laser folder: DOES NOT EXIST\n")
        
        color_dir = self.config['local_color_dir']
        if os.path.exists(color_dir):
            color_count = len([f for f in os.listdir(color_dir) 
                             if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
            text_widget.insert(tk.END, f"✅ Color folder: {color_count} photos\n")
        else:
            text_widget.insert(tk.END, f"⚠️ Color folder: DOES NOT EXIST\n")
        
        # Check output directory
        output_dir = self.config['local_output_dir']
        if os.path.exists(output_dir):
            obj_count = len([f for f in os.listdir(output_dir) 
                           if f.endswith('.obj')])
            text_widget.insert(tk.END, f"✅ Output folder: {obj_count} .obj files\n")
            
            if obj_count > 0:
                text_widget.insert(tk.END, "\n   Model files:\n")
                for file in os.listdir(output_dir):
                    if file.endswith(('.obj', '.mtl', '.png', '.ply')):
                        file_path = os.path.join(output_dir, file)
                        size = os.path.getsize(file_path)
                        text_widget.insert(tk.END, f"   • {file} ({size} bytes)\n")
        else:
            text_widget.insert(tk.END, f"⚠️ Output folder: DOES NOT EXIST\n")
        
        text_widget.see(tk.END)
    
    def show_settings(self):
        """Show settings configuration window"""
        settings_window = tk.Toplevel(self.root)
        settings_window.title("Configuration Settings")
        settings_window.geometry("700x600")
        
        notebook = ttk.Notebook(settings_window)
        notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Connection settings
        conn_frame = ttk.Frame(notebook)
        notebook.add(conn_frame, text="Connection")
        
        # Add padding frame
        conn_inner = ttk.LabelFrame(conn_frame, text="SSH Connection Settings", padding=15)
        conn_inner.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        ttk.Label(conn_inner, text="SSH Host:", font=("Arial", 10, "bold")).grid(row=0, column=0, sticky=tk.W, padx=5, pady=8)
        host_var = tk.StringVar(value=self.config['ssh_host'])
        host_entry = ttk.Entry(conn_inner, textvariable=host_var, width=35, font=("Arial", 10))
        host_entry.grid(row=0, column=1, padx=5, pady=8, sticky=tk.W)
        
        ttk.Label(conn_inner, text="SSH User:", font=("Arial", 10, "bold")).grid(row=1, column=0, sticky=tk.W, padx=5, pady=8)
        user_var = tk.StringVar(value=self.config['ssh_user'])
        user_entry = ttk.Entry(conn_inner, textvariable=user_var, width=35, font=("Arial", 10))
        user_entry.grid(row=1, column=1, padx=5, pady=8, sticky=tk.W)
        
        ttk.Label(conn_inner, text="SSH Password:", font=("Arial", 10, "bold")).grid(row=2, column=0, sticky=tk.W, padx=5, pady=8)
        pass_var = tk.StringVar(value=self.config['ssh_pass'])
        pass_entry = ttk.Entry(conn_inner, textvariable=pass_var, show="*", width=35, font=("Arial", 10))
        pass_entry.grid(row=2, column=1, padx=5, pady=8, sticky=tk.W)
        
        # Show/hide password button
        show_pass_var = tk.BooleanVar()
        def toggle_password():
            if show_pass_var.get():
                pass_entry.config(show="")
            else:
                pass_entry.config(show="*")
        
        ttk.Checkbutton(conn_inner, text="Show password", variable=show_pass_var, command=toggle_password).grid(row=3, column=1, sticky=tk.W, padx=5, pady=5)
        
        # Connection info
        info_frame = ttk.LabelFrame(conn_frame, text="Connection Info", padding=10)
        info_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
        
        ttk.Label(info_frame, text="Current target:", font=("Arial", 9)).pack(anchor=tk.W)
        current_target = ttk.Label(info_frame, text=f"{self.config['ssh_user']}@{self.config['ssh_host']}", 
                                  font=("Arial", 9, "bold"), foreground="blue")
        current_target.pack(anchor=tk.W)
        
        # Path settings
        paths_frame = ttk.Frame(notebook)
        notebook.add(paths_frame, text="Paths")
        
        # Remote paths
        remote_frame = ttk.LabelFrame(paths_frame, text="Raspberry Pi Paths", padding=15)
        remote_frame.pack(fill=tk.X, padx=10, pady=10)
        
        # Local paths
        local_frame = ttk.LabelFrame(paths_frame, text="Windows Local Paths", padding=15)
        local_frame.pack(fill=tk.X, padx=10, pady=10)
        
        path_vars = {}
        
        # Remote path labels
        remote_labels = [
            ("Scanner Project:", "remote_scanner_project"),
            ("Laser Photos:", "remote_lazer_path"),
            ("LED Photos:", "remote_leds_path")
        ]
        
        for i, (label, key) in enumerate(remote_labels):
            ttk.Label(remote_frame, text=label, font=("Arial", 10, "bold")).grid(row=i, column=0, sticky=tk.W, padx=5, pady=5)
            var = tk.StringVar(value=self.config[key])
            path_vars[key] = var
            ttk.Entry(remote_frame, textvariable=var, width=55, font=("Arial", 9)).grid(row=i, column=1, padx=5, pady=5, sticky=tk.W)
        
        # Local path labels
        local_labels = [
            ("3D Creator Executable:", "local_3d_creator"),
            ("Build Directory:", "local_build_dir"),
            ("Output Directory:", "local_output_dir")
        ]
        
        for i, (label, key) in enumerate(local_labels):
            ttk.Label(local_frame, text=label, font=("Arial", 10, "bold")).grid(row=i, column=0, sticky=tk.W, padx=5, pady=5)
            var = tk.StringVar(value=self.config[key])
            path_vars[key] = var
            ttk.Entry(local_frame, textvariable=var, width=55, font=("Arial", 9)).grid(row=i, column=1, padx=5, pady=5, sticky=tk.W)
        
        # Save button frame
        button_frame = ttk.Frame(settings_window)
        button_frame.pack(fill=tk.X, padx=10, pady=10)
        
        def save_settings():
            try:
                self.config['ssh_host'] = host_var.get()
                self.config['ssh_user'] = user_var.get()
                self.config['ssh_pass'] = pass_var.get()
                
                for key, var in path_vars.items():
                    self.config[key] = var.get()
                
                self.save_config()
                self.update_status_display()
                messagebox.showinfo("Settings Saved", "Configuration saved successfully!")
                settings_window.destroy()
            except Exception as e:
                messagebox.showerror("Save Error", f"Failed to save settings:\n{str(e)}")
        
        def test_connection():
            try:
                import socket
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                result = sock.connect_ex((host_var.get(), 22))
                sock.close()
                
                if result == 0:
                    messagebox.showinfo("Connection Test", "✅ SSH port is reachable!")
                else:
                    messagebox.showwarning("Connection Test", "❌ SSH port is not reachable")
            except Exception as e:
                messagebox.showerror("Connection Test", f"Test failed: {str(e)}")
        
        ttk.Button(button_frame, text="💾 Save Settings", command=save_settings, width=20).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="🔌 Test Connection", command=test_connection, width=20).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="❌ Cancel", command=settings_window.destroy, width=15).pack(side=tk.RIGHT, padx=5)
    
    def save_config(self):
        """Save configuration to file"""
        try:
            config_to_save = self.config.copy()
            # Save password as well (user requested this functionality)
            
            with open("scanner_config.json", "w") as f:
                json.dump(config_to_save, f, indent=4)
            self.log_message("✅ Configuration saved successfully")
        except Exception as e:
            self.log_message(f"⚠️ Could not save config: {str(e)}")
    
    def load_config(self):
        """Load configuration from file"""
        try:
            if os.path.exists("scanner_config.json"):
                with open("scanner_config.json", "r") as f:
                    loaded_config = json.load(f)
                    self.config.update(loaded_config)
                self.log_message("✅ Configuration loaded")
        except Exception as e:
            self.log_message(f"⚠️ Could not load config: {str(e)}")
    
    def disable_buttons_during_scan(self):
        """Disable all buttons except FOLDERS and STATUS during scanning"""
        self.connect_btn.config(state=tk.DISABLED)
        self.scan_btn.config(state=tk.DISABLED)
        self.model_btn.config(state=tk.DISABLED)
        
        # Find and disable other buttons except FOLDERS and STATUS
        for widget in self.root.winfo_children():
            self._disable_buttons_recursive(widget, ["FOLDERS", "STATUS"])
    
    def enable_buttons_after_scan(self):
        """Re-enable buttons after scanning completes"""
        # Re-enable main workflow buttons
        self.connect_btn.config(state=tk.NORMAL)
        self.scan_btn.config(state=tk.NORMAL if self.connection_status else tk.DISABLED)
        self.model_btn.config(state=tk.NORMAL)  # Will be properly set by update_status_display
        
        # Re-enable all other buttons that were disabled
        for widget in self.root.winfo_children():
            self._enable_buttons_recursive(widget, ["FOLDERS", "STATUS"])
        
        # Update status display to set proper button states
        self.update_status_display()
    
    def _disable_buttons_recursive(self, widget, exceptions):
        """Recursively disable buttons except those in exceptions list"""
        try:
            if isinstance(widget, ttk.Button):
                button_text = widget.cget('text')
                if not any(exception in button_text for exception in exceptions):
                    widget.config(state=tk.DISABLED)
            
            # Recursively check children
            for child in widget.winfo_children():
                self._disable_buttons_recursive(child, exceptions)
        except:
            pass  # Ignore any errors during recursive traversal
    
    def _enable_buttons_recursive(self, widget, exceptions):
        """Recursively re-enable buttons except those in exceptions list"""
        try:
            if isinstance(widget, ttk.Button):
                button_text = widget.cget('text')
                if not any(exception in button_text for exception in exceptions):
                    widget.config(state=tk.NORMAL)
            
            # Recursively check children
            for child in widget.winfo_children():
                self._enable_buttons_recursive(child, exceptions)
        except:
            pass  # Ignore any errors during recursive traversal

def main():
    root = tk.Tk()
    app = ScannerGUI(root)
    
    # Handle window closing
    def on_closing():
        if app.ssh_client:
            app.ssh_client.close()
        root.destroy()
    
    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()

if __name__ == "__main__":
    main() 