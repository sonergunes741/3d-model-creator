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
import shutil
import tempfile
import webbrowser
import sys
import requests
from urllib.parse import quote

# Try to import 3D visualization libraries
try:
    import matplotlib
    matplotlib.use('TkAgg')  # Use TkAgg backend for better tkinter integration
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D
    import numpy as np
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    MATPLOTLIB_AVAILABLE = False

# Try to import Plotly for hardware-accelerated 3D rendering
try:
    import plotly.graph_objects as go
    import plotly.offline as pyo
    PLOTLY_AVAILABLE = True
except ImportError:
    PLOTLY_AVAILABLE = False

# Try to import requests for file upload
try:
    import requests
    REQUESTS_AVAILABLE = True
except ImportError:
    REQUESTS_AVAILABLE = False

class ScannerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Morphix - Laser Scanner & 3D Model Creator")
        self.root.geometry("900x700")
        self.root.resizable(True, True)
        
        # SSH connection variables
        self.ssh_client = None
        
        # Shutdown flag to prevent operations during shutdown
        self.shutting_down = False
        
        # Status variables (matching bash script)
        self.connection_status = False
        self.scan_status = False
        self.scanning_active = False  # New: indicates scanning is in progress
        self.download_status = False
        self.model_status = False
        self.model_active = False  # New: indicates model creation is in progress
        self.laser_status = False
        self.led_status = False
        self.user_stopped_scan = False
        self.viewer_active = False  # New: indicates 3D viewer is open
        
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
            "local_output_dir": "./output",
            "remote_laser_control": "/home/realityshapers/Desktop/scanner_project/simple_laser_control",
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

        self.setup_laser_control(main_frame)
        
        # Main workflow buttons
        self.setup_workflow_buttons(main_frame)
        
        # Log and additional controls
        self.setup_log_and_controls(main_frame)
        
    def setup_banner(self, parent):
        banner_frame = ttk.LabelFrame(parent, text="", padding=10)
        banner_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Main project name
        title_label = ttk.Label(banner_frame, text="Morphix", 
                               font=("Arial", 20, "bold"), foreground="purple")
        title_label.pack()
        
        # Subtitle
        subtitle_label = ttk.Label(banner_frame, text="Laser Scanner & 3D Model Creator", 
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
            ("3. 3D MODEL", "model_status"),
            ("4. VIEW 3D", "view_status")
        ]
        
        for i, (label, status_key) in enumerate(status_items):
            row = i // 2
            col = (i % 2) * 2
            
            ttk.Label(status_grid, text=label, font=("Arial", 10, "bold")).grid(row=row, column=col, sticky=tk.W, padx=5, pady=2)
            
            status_var = tk.StringVar()
            self.status_labels[status_key] = status_var
            status_label = ttk.Label(status_grid, textvariable=status_var, font=("Arial", 10))
            status_label.grid(row=row, column=col+1, sticky=tk.W, padx=5, pady=2)

    def setup_laser_control(self, parent):
        """Setup laser control panel"""
        laser_frame = ttk.LabelFrame(parent, text="TEST COMPONENTS", padding=10)
        laser_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Status indicators
        status_frame = ttk.Frame(laser_frame)
        status_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Laser status
        self.laser_status_var = tk.StringVar(value="❌ KAPALI")
        ttk.Label(status_frame, text="Laser:", font=("Arial", 10, "bold")).pack(side=tk.LEFT, padx=(0, 5))
        self.laser_status_label = ttk.Label(status_frame, textvariable=self.laser_status_var, 
                                        font=("Arial", 10, "bold"), foreground="red")
        self.laser_status_label.pack(side=tk.LEFT, padx=(0, 20))
        
        # LED status
        self.led_status_var = tk.StringVar(value="❌ KAPALI")
        ttk.Label(status_frame, text="LED:", font=("Arial", 10, "bold")).pack(side=tk.LEFT, padx=(0, 5))
        self.led_status_label = ttk.Label(status_frame, textvariable=self.led_status_var, 
                                        font=("Arial", 10, "bold"), foreground="red")
        self.led_status_label.pack(side=tk.LEFT, padx=(0, 20))
        
        # Control buttons
        control_frame = ttk.Frame(laser_frame)
        control_frame.pack(fill=tk.X)
        
        # Laser buttons
        laser_btn_frame = ttk.Frame(control_frame)
        laser_btn_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        self.laser_on_btn = ttk.Button(laser_btn_frame, text="LASER AÇ", 
                                    command=self.laser_on, width=15)
        self.laser_on_btn.pack(side=tk.LEFT, padx=2)
        
        self.laser_off_btn = ttk.Button(laser_btn_frame, text="LASER KAPAT", 
                                    command=self.laser_off, width=15)
        self.laser_off_btn.pack(side=tk.LEFT, padx=2)
        
        # LED buttons
        led_btn_frame = ttk.Frame(control_frame)
        led_btn_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        self.led_on_btn = ttk.Button(led_btn_frame, text="LED AÇ", 
                                    command=self.led_on, width=15)
        self.led_on_btn.pack(side=tk.LEFT, padx=2)
        
        self.led_off_btn = ttk.Button(led_btn_frame, text="LED KAPAT", 
                                    command=self.led_off, width=15)
        self.led_off_btn.pack(side=tk.LEFT, padx=2)

        motor_btn_frame = ttk.Frame(control_frame)
        motor_btn_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        self.motor_rotation_btn = ttk.Button(motor_btn_frame, text="TEST MOTOR", 
                                        command=self.motor_full_rotation, width=12)
        self.motor_rotation_btn.pack(side=tk.LEFT, padx=2)

        self.motor_off_btn = ttk.Button(motor_btn_frame, text="MOTOR OFF", 
                                    command=self.motor_off, width=12)
        self.motor_off_btn.pack(side=tk.LEFT, padx=2)

    def motor_full_rotation(self):
        """Full 360 degree rotation"""
        if self.execute_laser_command("motor_rotation"):
            self.log_message("🔄 Motor tam dönüş tamamlandı")

    def motor_off(self):
        """Turn motor off"""
        if self.execute_laser_command("motor_off"):
            self.log_message("⏹️ Motor kapatıldı")

# 4. Laser control metodlarını ekleyin:
    def execute_laser_command(self, command):
        """Execute laser control command on Raspberry Pi"""
        if not self.connection_status:
            messagebox.showwarning("Not Connected", "Please connect to Raspberry Pi first!")
            return False
        
        try:
            cmd = f"cd {self.config['remote_scanner_project']} && sudo ./{self.config['remote_laser_control'].split('/')[-1]} {command}"
            stdin, stdout, stderr = self.ssh_client.exec_command(cmd)
            result = stdout.read().decode().strip()
            error = stderr.read().decode().strip()
            
            if error:
                self.log_message(f"❌ Laser command error: {error}")
                return False
            
            self.log_message(f"🔧 Laser command '{command}': {result}")
            return True
            
        except Exception as e:
            self.log_message(f"❌ Laser command failed: {str(e)}")
            return False

    def laser_on(self):
        """Turn laser ON"""
        if self.execute_laser_command("laser_on"):
            self.laser_status = True
            self.laser_status_var.set("✅ AÇIK")
            self.laser_status_label.config(foreground="green")
            self.log_message("🔴 LASER AÇILDI")
            # Disable led_on_btn while laser is on
            self.led_on_btn.config(state=tk.DISABLED)

    def laser_off(self):
        """Turn laser OFF"""
        if self.execute_laser_command("laser_off"):
            self.laser_status = False
            self.laser_status_var.set("❌ KAPALI")
            self.laser_status_label.config(foreground="red")
            self.log_message("⚫ LASER KAPATILDI")
            # Enable led_on_btn when laser is off
            self.led_on_btn.config(state=tk.NORMAL)

    def led_on(self):
        """Turn LED ON"""
        if self.execute_laser_command("led_on"):
            self.led_status = True
            self.led_status_var.set("✅ AÇIK")
            self.led_status_label.config(foreground="green")
            self.log_message("💡 LED AÇILDI")
            # Disable laser_on_btn while LED is on
            self.laser_on_btn.config(state=tk.DISABLED)

    def led_off(self):
        """Turn LED OFF"""
        if self.execute_laser_command("led_off"):
            self.led_status = False
            self.led_status_var.set("❌ KAPALI")
            self.led_status_label.config(foreground="red")
            self.log_message("🔲 LED KAPATILDI")
            # Enable laser_on_btn when LED is off
            self.laser_on_btn.config(state=tk.NORMAL)

    def emergency_stop(self):
        """Emergency stop - turn everything OFF"""
        if self.execute_laser_command("all_off"):
            self.laser_status = False
            self.led_status = False
            self.laser_status_var.set("❌ KAPALI")
            self.led_status_var.set("❌ KAPALI")
            self.laser_status_label.config(foreground="red")
            self.led_status_label.config(foreground="red")
            self.log_message("🚨 ACİL DURDURMA - HEPSİ KAPATILDI")

    def update_laser_status(self):
        """Update laser and LED status from Raspberry Pi"""
        if not self.connection_status:
            messagebox.showwarning("Not Connected", "Please connect to Raspberry Pi first!")
            return
        
        try:
            cmd = f"cd {self.config['remote_scanner_project']} && sudo ./{self.config['remote_laser_control'].split('/')[-1]} status"
            stdin, stdout, stderr = self.ssh_client.exec_command(cmd)
            result = stdout.read().decode().strip()
            
            if "LASER:" in result and "LED:" in result:
                # Parse result: "LASER:1,LED:0"
                parts = result.split(",")
                laser_state = parts[0].split(":")[1] == "1"
                led_state = parts[1].split(":")[1] == "1"
                
                # Update laser status
                self.laser_status = laser_state
                if laser_state:
                    self.laser_status_var.set("✅ AÇIK")
                    self.laser_status_label.config(foreground="green")
                else:
                    self.laser_status_var.set("❌ KAPALI")
                    self.laser_status_label.config(foreground="red")
                
                # Update LED status
                self.led_status = led_state
                if led_state:
                    self.led_status_var.set("✅ AÇIK")
                    self.led_status_label.config(foreground="green")
                else:
                    self.led_status_var.set("❌ KAPALI")
                    self.led_status_label.config(foreground="red")
                
                self.log_message(f"🔄 Durum güncellendi - Laser: {'AÇIK' if laser_state else 'KAPALI'}, LED: {'AÇIK' if led_state else 'KAPALI'}")
            else:
                self.log_message("⚠️ Durum okunamadı")
                
        except Exception as e:
            self.log_message(f"❌ Durum okuma hatası: {str(e)}")
        
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
        
        # VIEW 3D button with no action for now
        self.view_btn = ttk.Button(button_grid, text="4. VIEW 3D", command=lambda: None, width=15, state=tk.DISABLED)
        self.view_btn.grid(row=0, column=3, padx=5, pady=5)

        self.stop_btn = ttk.Button(button_grid, text="🛑 STOP SCAN", command=self.stop_scanner, width=15, state=tk.DISABLED)
        self.stop_btn.grid(row=0, column=4, padx=5, pady=5)
        
        # Additional control buttons
        control_frame = ttk.Frame(workflow_frame)
        control_frame.pack(pady=(10, 0))
        
        ttk.Button(control_frame, text="STATUS", command=self.show_detailed_status, width=12).pack(side=tk.LEFT, padx=2)
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

        

    def stop_scanner(self):
        """Send SIGINT (Ctrl+C) to running scanner process"""
        if not self.connection_status:
            messagebox.showwarning("Not Connected", "Please connect to Raspberry Pi first!")
            return
        
        try:
            # Set flag to prevent error messages
            self.user_stopped_scan = True
            
            self.log_message("🛑 SCANNER DURDURMA KOMUTU GÖNDERİLİYOR...")
            
            # 1. First try graceful stop
            kill_cmd = "sudo pkill -SIGINT scanner_control"
            stdin, stdout, stderr = self.ssh_client.exec_command(kill_cmd)
            stdout.read()
            
            # 2. Wait a moment for graceful shutdown
            time.sleep(1)
            
            # 3. Check if still running and force kill if necessary
            check_cmd = "pgrep scanner_control"
            stdin, stdout, stderr = self.ssh_client.exec_command(check_cmd)
            still_running = stdout.read().decode().strip()
            
            if still_running:
                self.log_message("   Zorla durdurma...")
                force_kill_cmd = "sudo pkill -SIGKILL scanner_control"
                stdin, stdout, stderr = self.ssh_client.exec_command(force_kill_cmd)
                stdout.read()
            
            # 4. Clean up camera processes
            cleanup_cmd = "sudo pkill -9 libcamera 2>/dev/null; sudo pkill -9 timeout 2>/dev/null"
            stdin, stdout, stderr = self.ssh_client.exec_command(cleanup_cmd)
            stdout.read()

            # 5. SAFETY: Turn off motor and lights (YENİ)
            self.log_message("   Güvenlik: Motor ve ışıklar kapatılıyor...")
            safety_cmd = f"cd {self.config['remote_scanner_project']} && sudo ./simple_laser_control motor_off"
            stdin, stdout, stderr = self.ssh_client.exec_command(safety_cmd)
            stdout.read()
            
            self.log_message("✅ SCANNER DURDURULDU")
            
            # Update UI immediately
            self.scanning_active = False
            self.progress_var.set("Tarama durduruldu")
            self.progress_bar.stop()
            self.update_status_display()
            
            # Don't show success message popup - just log it
            
        except Exception as e:
            self.log_message(f"❌ Stop signal failed: {str(e)}")
        
        
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
                            ("blue" if self.model_active else "orange")),
            "view_status": ("🔄 Viewer Open" if self.viewer_active else 
                           ("✅ Available" if self.check_model_files_exist() else "❌ No Model"), 
                           "blue" if self.viewer_active else 
                           ("green" if self.check_model_files_exist() else "red"))
        }
        
        for status_key, (text, color) in statuses.items():
            if status_key in self.status_labels:
                self.status_labels[status_key].set(text)
        
        # Update connect button text
        self.connect_btn.config(text="✅ CONNECTED" if self.connection_status else "1. CONNECT")
        
        # Update VIEW 3D button text based on viewer status
        if self.viewer_active:
            self.view_btn.config(text="🌐 CLOSE VIEWER")
        else:
            self.view_btn.config(text="4. VIEW 3D")
        
        # Update button states using the new centralized method
        self.update_button_states()
    
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
    
    def check_model_files_exist(self):
        """Check if 3D model files exist for viewing"""
        try:
            # Only check GUI's configured output directory
            output_dir = self.config['local_output_dir']
            
            if os.path.exists(output_dir):
                # Look for OBJ files (primary format)
                obj_files = [f for f in os.listdir(output_dir) 
                           if f.lower().endswith('.obj')]
                return len(obj_files) > 0
            
            return False
            
        except Exception:
            return False
    
    def log_message(self, message, level="INFO"):
        """Add message to log with timestamp and color coding"""
        # Don't log messages if we're shutting down
        if self.shutting_down:
            return
            
        try:
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
            
            # Check if log_text widget still exists and is valid
            if hasattr(self, 'log_text') and self.log_text.winfo_exists():
                self.log_text.insert(tk.END, formatted_message)
                self.log_text.see(tk.END)
                self.root.update_idletasks()
        except (tk.TclError, AttributeError):
            # GUI has been destroyed or widget is invalid, just print to console
            print(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
        except Exception:
            # Any other error, silently ignore during shutdown
            pass
    
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
        # Reset scan and download status at the start of a new scan
        self.scan_status = False
        self.download_status = False
        self.scanning_active = True
        self.update_status_display()
        # Set laser and led status to off
        self.laser_status = False
        self.led_status = False
        self.laser_status_var.set("❌ KAPALI")
        self.led_status_var.set("❌ KAPALI")
        self.laser_status_label.config(foreground="red")
        self.led_status_label.config(foreground="red")
        
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
                self.disable_buttons_during_operation("scan")
                
                # Set scanning as active
                self.scanning_active = True
                self.update_status_display()
                self.stop_btn.config(state=tk.NORMAL)
                
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
                
                scan_cmd = f"cd {self.config['remote_scanner_project']} && sudo ./scanner_control"
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
                if getattr(self, 'user_stopped_scan', False):
                    # User requested stop - not an error
                    self.log_message("⏹️ TARAMA KULLANICI TARAFINDAN DURDURULDU")
                    self.progress_var.set("Tarama durduruldu")
                    self.user_stopped_scan = False  # Reset flag
                else:
                    # Real error
                    self.log_message(f"❌ Scanning failed: {str(e)}")
                    self.progress_var.set("Scanning failed")
                    messagebox.showerror("Scan Error", f"Laser scanning failed:\n{str(e)}")
            finally:
                 # Set scanning as inactive
                 self.scanning_active = False
                 self.progress_bar.stop()
                 # Reset stop flag
                 self.user_stopped_scan = False  # EKLE
                 # Re-enable buttons after scanning completes
                 self.enable_buttons_after_operation("scan")
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
        # Reset model status at the start of a new model creation
        self.model_status = False
        self.model_active = True
        self.update_status_display()
        
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
                self.disable_buttons_during_operation("model")
                
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
                output_dir_used = self.config['local_output_dir']
                if os.path.exists(output_dir_used):
                    for file in os.listdir(output_dir_used):
                        if file.endswith('.obj'):
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
                self.enable_buttons_after_operation("model")
                self.update_status_display()
        
        threading.Thread(target=model_thread, daemon=True).start()
    
    def view_3d_model(self):
        """View the created 3D model using 3dviewer.net with automatic upload and fallback to transfer.sh if file.io fails"""
        if self.viewer_active:
            self.close_web_viewer()
            return

        if not self.check_model_files_exist():
            messagebox.showwarning("No Model Found", "Please create a 3D model first!")
            return

        def open_web_viewer_thread():
            try:
                self.viewer_active = True
                self.root.after(0, self.update_status_display)
                self.log_message("🌐 3DVIEWER.NET İÇİN MODEL YÜKLENİYOR")
                obj_file_path = None
                output_dir = self.config['local_output_dir']
                if os.path.exists(output_dir):
                    obj_files = [f for f in os.listdir(output_dir) if f.lower().endswith('.obj')]
                    if obj_files:
                        obj_file_path = os.path.join(output_dir, obj_files[0])
                if not obj_file_path:
                    self.viewer_active = False
                    self.root.after(0, self.update_status_display)
                    self.root.after(0, lambda: messagebox.showerror("Model Error", f"No OBJ file found in GUI output directory: {output_dir}"))
                    return

                from urllib.parse import quote
                # Önce file.io ile yüklemeyi dene
                try:
                    with open(obj_file_path, 'rb') as f:
                        files = {'file': f}
                        response = requests.post('https://file.io/', files=files)
                    self.log_message(f"file.io yanıtı: {response.text}")
                    try:
                        result = response.json()
                    except Exception as e:
                        raise Exception("file.io'dan geçersiz yanıt alındı veya servis çalışmıyor.")
                    if response.status_code == 200 and result.get('success'):
                        file_url = result.get('link')
                        self.log_message(f"   Model başarıyla yüklendi: {file_url}")
                        viewer_url = f"https://3dviewer.net/#model={quote(file_url)}"
                        webbrowser.open(viewer_url)
                        self.log_message("✅ 3D MODEL 3dviewer.net'te otomatik açıldı (file.io)")
                        return
                    else:
                        raise Exception("file.io upload failed: " + str(result))
                except Exception as e:
                    self.log_message(f"file.io başarısız: {str(e)}. transfer.sh ile tekrar deneniyor...")
                    # transfer.sh ile yükle
                    try:
                        with open(obj_file_path, 'rb') as f:
                            response = requests.put(f'https://transfer.sh/{os.path.basename(obj_file_path)}', data=f)
                        self.log_message(f"transfer.sh yanıtı: {response.text}")
                        if response.status_code == 200:
                            file_url = response.text.strip()
                            self.log_message(f"   Model transfer.sh ile yüklendi: {file_url}")
                            viewer_url = f"https://3dviewer.net/#model={quote(file_url)}"
                            webbrowser.open(viewer_url)
                            self.log_message("✅ 3D MODEL 3dviewer.net'te otomatik açıldı (transfer.sh)")
                            return
                        else:
                            raise Exception(f"transfer.sh upload failed: {response.text}")
                    except Exception as e2:
                        raise Exception(f"Hem file.io hem transfer.sh başarısız: {str(e2)}")
            except Exception as e:
                self.viewer_active = False
                self.root.after(0, self.update_status_display)
                error_msg = f"Failed to open 3dviewer.net:\n{str(e)}"
                self.log_message(f"❌ 3dviewer.net açma hatası: {str(e)}")
                self.root.after(0, lambda: messagebox.showerror("Viewer Error", error_msg))
        threading.Thread(target=open_web_viewer_thread, daemon=True).start()
        self.log_message("🔄 3dviewer.net için hazırlık yapılıyor...")
    
    def close_web_viewer(self):
        """Close the web-based 3D viewer and clean up temporary files"""
        try:
            self.log_message("🔲 Closing web-based 3D viewer...")
            
            # Clean up temporary HTML file if it exists
            if hasattr(self, 'temp_html_path') and os.path.exists(self.temp_html_path):
                try:
                    os.remove(self.temp_html_path)
                    self.log_message("   Temporary HTML file cleaned up")
                except:
                    pass
                delattr(self, 'temp_html_path')
            
            # Reset viewer state
            self.viewer_active = False
            self.update_status_display()
            
            self.log_message("✅ Web viewer closed successfully")
            
        except Exception as e:
            self.log_message(f"⚠️ Error closing web viewer: {str(e)}")
            # Force reset viewer state even if there's an error
            self.viewer_active = False
            self.update_status_display()
    
    def parse_obj_file(self, obj_file_path):
        """Parse OBJ file and extract vertices and faces"""
        vertices = []
        faces = []
        
        try:
            with open(obj_file_path, 'r') as file:
                for line in file:
                    line = line.strip()
                    if line.startswith('v '):  # Vertex
                        parts = line.split()
                        if len(parts) >= 4:
                            x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                            vertices.append([x, y, z])
                    elif line.startswith('f '):  # Face
                        parts = line.split()
                        if len(parts) >= 4:
                            # Handle faces with texture/normal indices (v/vt/vn format)
                            face_vertices = []
                            for part in parts[1:]:
                                vertex_index = int(part.split('/')[0]) - 1  # OBJ indices start at 1
                                face_vertices.append(vertex_index)
                            faces.append(face_vertices)
            
            return np.array(vertices), faces
            
        except Exception as e:
            raise Exception(f"Failed to parse OBJ file: {str(e)}")
    
    def create_3d_visualization(self, vertices, faces, obj_file_path):
        """Create and display 3D visualization using matplotlib"""
        try:
            self.log_message("🎨 Creating 3D visualization in main thread...")
            
            # Disable matplotlib toolbar
            import matplotlib
            matplotlib.rcParams['toolbar'] = 'None'
            
            # Create figure with dark background
            fig = plt.figure(figsize=(12, 9), facecolor='black')
            ax = fig.add_subplot(111, projection='3d', facecolor='black')
            
            # Set title with white text
            model_name = os.path.basename(obj_file_path)
            fig.suptitle(f'3D Model Viewer - {model_name}', fontsize=14, fontweight='bold', color='white')
            
            # Hide axes, grid, and labels for clean look
            ax.set_axis_off()
            
            # Set dark background
            ax.xaxis.pane.fill = False
            ax.yaxis.pane.fill = False
            ax.zaxis.pane.fill = False
            ax.xaxis.pane.set_edgecolor('black')
            ax.yaxis.pane.set_edgecolor('black')
            ax.zaxis.pane.set_edgecolor('black')
            ax.grid(False)
            
            # SIMPLE ROTATION FIX: Rotate 90 degrees to make object stand vertically
            # Rotate around X-axis to make the object stand upright (Y becomes Z)
            rotation_matrix = np.array([
                [1, 0, 0],
                [0, 0, -1],
                [0, 1, 0]
            ])
            vertices = np.dot(vertices, rotation_matrix.T)
            self.log_message("   Applied 90° rotation to make object stand vertically")
            
            # Parse vertex colors from MTL file if available
            vertex_colors = self.parse_vertex_colors(obj_file_path)
            
            # Plot vertices as points with colors
            if len(vertices) > 0:
                if vertex_colors is not None and len(vertex_colors) == len(vertices):
                    # Use actual vertex colors
                    ax.scatter(vertices[:, 0], vertices[:, 1], vertices[:, 2], 
                              c=vertex_colors, s=0.5, alpha=0.8)
                else:
                    # Use default blue color for vertices
                    ax.scatter(vertices[:, 0], vertices[:, 1], vertices[:, 2], 
                              c='lightblue', s=0.5, alpha=0.6)
            
            # Plot faces with proper coloring and wireframe
            if len(faces) > 0:
                self.log_message(f"   Rendering {len(faces)} faces with wireframe...")
                
                # Limit faces for performance but use more than before
                max_faces = min(5000, len(faces))
                
                # First pass: Draw filled faces
                for i, face in enumerate(faces[:max_faces]):
                    if len(face) >= 3:
                        # Create face polygon
                        face_vertices = vertices[face]
                        
                        # Use face colors if available, otherwise use a subtle color
                        if vertex_colors is not None and len(vertex_colors) > max(face):
                            # Average color of face vertices
                            face_color = np.mean([vertex_colors[j] for j in face if j < len(vertex_colors)], axis=0)
                            color = face_color[:3] if len(face_color) >= 3 else [0.7, 0.7, 0.9]
                        else:
                            # Use a subtle blue-gray color
                            color = [0.6, 0.7, 0.9]
                        
                        # Draw filled triangular face if it's a triangle
                        if len(face) == 3:
                            # Create a triangular surface
                            from mpl_toolkits.mplot3d.art3d import Poly3DCollection
                            triangle = [face_vertices]
                            poly = Poly3DCollection(triangle, alpha=0.6, facecolor=color, edgecolor='none')
                            ax.add_collection3d(poly)
                
                # Second pass: Draw wireframe edges
                for i, face in enumerate(faces[:max_faces]):
                    if len(face) >= 3:
                        # Create face polygon for wireframe
                        face_vertices = vertices[face]
                        
                        # Close the polygon by adding first vertex at the end
                        face_vertices = np.vstack([face_vertices, face_vertices[0]])
                        
                        # Draw wireframe with subtle white/gray lines
                        ax.plot(face_vertices[:, 0], face_vertices[:, 1], face_vertices[:, 2], 
                               color='white', alpha=0.3, linewidth=0.2)
                
                if len(faces) > max_faces:
                    self.log_message(f"   (Showing first {max_faces} of {len(faces)} faces for performance)")
            
            # Set equal aspect ratio and center the model
            max_range = np.array([vertices[:, 0].max() - vertices[:, 0].min(),
                                vertices[:, 1].max() - vertices[:, 1].min(),
                                vertices[:, 2].max() - vertices[:, 2].min()]).max() / 2.0
            
            mid_x = (vertices[:, 0].max() + vertices[:, 0].min()) * 0.5
            mid_y = (vertices[:, 1].max() + vertices[:, 1].min()) * 0.5
            mid_z = (vertices[:, 2].max() + vertices[:, 2].min()) * 0.5
            
            ax.set_xlim(mid_x - max_range, mid_x + max_range)
            ax.set_ylim(mid_y - max_range, mid_y + max_range)
            ax.set_zlim(mid_z - max_range, mid_z + max_range)
            
            # Add model info text in top-left corner with white text
            info_text = f"Vertices: {len(vertices):,}\nFaces: {len(faces):,}\nFile: {model_name}"
            ax.text2D(0.02, 0.98, info_text, transform=ax.transAxes, fontsize=10,
                     verticalalignment='top', color='white',
                     bbox=dict(boxstyle='round', facecolor='black', alpha=0.7, edgecolor='white'))
            
            # Add interaction instructions in bottom-left corner with white text
            instructions = "Mouse: Rotate view (constrained)\nScroll: Zoom\nRight-click: Pan"
            ax.text2D(0.02, 0.02, instructions, transform=ax.transAxes, fontsize=9,
                     verticalalignment='bottom', color='white',
                     bbox=dict(boxstyle='round', facecolor='black', alpha=0.7, edgecolor='white'))
            
            # Set initial viewing angle for better presentation of vertical object
            # Elevation: 15 degrees (slightly looking up)
            # Azimuth: 30 degrees (rotated view for better perspective)
            ax.view_init(elev=15, azim=30)
            
            # ROTATION CONSTRAINTS: Limit rotation angles
            # Azimuth: 360° (full rotation around vertical axis)
            # Elevation: 0° to 180° (prevent upside-down viewing)
            
            def constrain_rotation():
                """Constrain the viewing angles to realistic limits"""
                elev, azim = ax.elev, ax.azim
                
                # Constrain elevation to 0-180 degrees (prevent upside-down)
                constrained = False
                if elev < 0:
                    elev = 0
                    constrained = True
                elif elev > 180:
                    elev = 180
                    constrained = True
                
                # Azimuth can be 0-360 degrees (full rotation around vertical axis)
                # Normalize azimuth to 0-360 range
                azim = azim % 360
                
                # Only apply constraints if needed (to avoid unnecessary redraws)
                if constrained:
                    ax.view_init(elev=elev, azim=azim)
                    return True
                return False
            
            # Use a timer-based approach for smooth, live rotation with constraints
            constraint_timer = None
            
            def on_motion(event):
                """Handle mouse motion - let matplotlib handle rotation naturally"""
                nonlocal constraint_timer
                
                # Cancel previous timer if it exists
                if constraint_timer is not None:
                    constraint_timer.stop()
                
                # Set a short timer to check constraints after motion stops
                # This allows smooth rotation while still enforcing limits
                def check_constraints():
                    if constrain_rotation():
                        fig.canvas.draw_idle()
                
                # Use matplotlib's timer for non-blocking constraint checking
                constraint_timer = fig.canvas.new_timer(interval=50)  # 50ms delay
                constraint_timer.single_shot = True
                constraint_timer.add_callback(check_constraints)
                constraint_timer.start()
            
            # Connect the motion event
            fig.canvas.mpl_connect('motion_notify_event', on_motion)
            
            # Apply constraints immediately when user finishes dragging
            def on_button_release(event):
                """Handle button release with immediate constraint check"""
                if event.inaxes == ax:
                    if constrain_rotation():
                        fig.canvas.draw_idle()
            
            fig.canvas.mpl_connect('button_release_event', on_button_release)
            
            # Also add a scroll event handler to maintain constraints during zoom
            def on_scroll(event):
                """Handle scroll events and maintain constraints"""
                if event.inaxes == ax:
                    # Small delay to let zoom complete, then check constraints
                    def check_constraints_after_zoom():
                        if constrain_rotation():
                            fig.canvas.draw_idle()
                    
                    zoom_timer = fig.canvas.new_timer(interval=100)
                    zoom_timer.single_shot = True
                    zoom_timer.add_callback(check_constraints_after_zoom)
                    zoom_timer.start()
            
            fig.canvas.mpl_connect('scroll_event', on_scroll)
            
            # Set up window close event handler to reset viewer flag
            def on_viewer_close(event):
                self.viewer_active = False
                if not self.shutting_down:
                    try:
                        self.update_status_display()
                        self.log_message("🔲 3D viewer window closed")
                    except:
                        pass
            
            # Connect the close event
            fig.canvas.mpl_connect('close_event', on_viewer_close)
            
            # Show the plot with tight layout
            plt.tight_layout()
            plt.show()
            
            self.log_message("✅ 3D MODEL VIEWER OPENED SUCCESSFULLY")
            self.log_message("   Rotation constraints: Vertical 360°, Horizontal 0-180°")
            
        except Exception as e:
            # Reset viewer flag on error
            self.viewer_active = False
            self.update_status_display()
            self.log_message(f"❌ 3D visualization failed: {str(e)}")
            raise Exception(f"Failed to create 3D visualization: {str(e)}")
    
    def parse_vertex_colors(self, obj_file_path):
        """Parse vertex colors from OBJ file or associated texture"""
        try:
            # Look for vertex colors in OBJ file (some OBJ files have vertex colors)
            vertex_colors = []
            
            with open(obj_file_path, 'r') as file:
                for line in file:
                    line = line.strip()
                    if line.startswith('v '):  # Vertex with possible color
                        parts = line.split()
                        if len(parts) >= 7:  # x y z r g b format
                            r, g, b = float(parts[4]), float(parts[5]), float(parts[6])
                            vertex_colors.append([r, g, b])
                        elif len(parts) >= 4:  # Just x y z, no color
                            vertex_colors.append([0.7, 0.7, 0.9])  # Default blue-gray
            
            if len(vertex_colors) > 0:
                return np.array(vertex_colors)
            
            # If no vertex colors found, try to load from texture/MTL file
            mtl_file = obj_file_path.replace('.obj', '.mtl')
            if os.path.exists(mtl_file):
                return self.parse_mtl_colors(mtl_file)
            
            return None
            
        except Exception as e:
            self.log_message(f"   Could not parse vertex colors: {str(e)}")
            return None
    
    def parse_mtl_colors(self, mtl_file_path):
        """Parse colors from MTL material file"""
        try:
            default_color = [0.7, 0.7, 0.9]  # Default blue-gray
            
            with open(mtl_file_path, 'r') as file:
                for line in file:
                    line = line.strip()
                    if line.startswith('Kd '):  # Diffuse color
                        parts = line.split()
                        if len(parts) >= 4:
                            r, g, b = float(parts[1]), float(parts[2]), float(parts[3])
                            return np.array([[r, g, b]])  # Single color for all vertices
            
            return np.array([default_color])
            
        except Exception:
            return None
    
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
    
    def disable_buttons_during_operation(self, operation_type="scan"):
        """Disable buttons during operations, keeping only STATUS and FOLDERS active"""
        # Always disable main workflow buttons during any operation
        self.connect_btn.config(state=tk.DISABLED)
        self.scan_btn.config(state=tk.DISABLED)
        self.model_btn.config(state=tk.DISABLED)
        self.view_btn.config(state=tk.DISABLED)
        
        # Find and disable other buttons except FOLDERS and STATUS
        for widget in self.root.winfo_children():
            self._disable_buttons_recursive(widget, ["FOLDERS", "STATUS"])
    
    def enable_buttons_after_operation(self, operation_type="scan"):
        """Re-enable buttons after operations complete, with proper state logic"""
        # Re-enable buttons based on current state
        self.connect_btn.config(state=tk.NORMAL)
        
        # SCAN button: only enabled if connected
        self.scan_btn.config(state=tk.NORMAL if self.connection_status else tk.DISABLED)
        
        # MODEL button: only enabled if photos exist (either downloaded or local)
        photos_available = self.check_local_photos_exist()
        self.model_btn.config(state=tk.NORMAL if photos_available else tk.DISABLED)
        
        # VIEW 3D button: only enabled if model files exist
        model_files_exist = self.check_model_files_exist()
        self.view_btn.config(state=tk.NORMAL if model_files_exist else tk.DISABLED)
        
        # Re-enable all other buttons that were disabled
        for widget in self.root.winfo_children():
            self._enable_buttons_recursive(widget, ["FOLDERS", "STATUS"])
        
        # Update status display to ensure consistency
        self.update_status_display()
    
    def update_button_states(self):
        """Update button states based on current workflow status"""
        # Only update if no operation is currently active
        if not self.scanning_active and not self.model_active:
            # CONNECT button: always enabled
            self.connect_btn.config(state=tk.NORMAL)
            
            # SCAN button: only enabled if connected and not currently scanning
            scan_enabled = self.connection_status
            self.scan_btn.config(state=tk.NORMAL if scan_enabled else tk.DISABLED)
            
            # MODEL button: only enabled if photos exist and not currently creating model
            photos_available = self.check_local_photos_exist()
            self.model_btn.config(state=tk.NORMAL if photos_available else tk.DISABLED)
            
            # VIEW 3D button: only enabled if model files exist AND viewer is not already open
            model_files_exist = self.check_model_files_exist()
            view_enabled = model_files_exist and not self.viewer_active
            self.view_btn.config(state=tk.NORMAL if view_enabled else tk.DISABLED)

            self.stop_btn.config(state=tk.NORMAL if self.scanning_active else tk.DISABLED)

            laser_enabled = self.connection_status and not self.scanning_active and not self.model_active
    
            self.laser_on_btn.config(state=tk.NORMAL if laser_enabled else tk.DISABLED)
            self.laser_off_btn.config(state=tk.NORMAL if laser_enabled else tk.DISABLED)
            self.led_on_btn.config(state=tk.NORMAL if laser_enabled else tk.DISABLED)
            self.led_off_btn.config(state=tk.NORMAL if laser_enabled else tk.DISABLED)
            
            # Debug logging (only when state changes would be visible)
            # self.log_message(f"🔧 Button states updated - Scan: {'✅' if scan_enabled else '❌'}, Model: {'✅' if photos_available else '❌'}")
        else:
            # During operations, buttons should remain disabled
            # Debug logging for operations in progress
            operation = "scanning" if self.scanning_active else "model creation"
            # self.log_message(f"🔧 Buttons remain disabled during {operation}")
            pass
    
    def disable_buttons_during_scan(self):
        """Legacy method - redirect to new method"""
        self.disable_buttons_during_operation("scan")
    
    def enable_buttons_after_scan(self):
        """Legacy method - redirect to new method"""
        self.enable_buttons_after_operation("scan")
    
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
    
    # Handle window closing properly
    def on_closing():
        try:
            # Set shutdown flag to prevent further operations
            app.shutting_down = True
            
            print("Shutting down application...")
            
            # Close web viewer if active
            if app.viewer_active:
                print("Closing web viewer...")
                try:
                    app.close_web_viewer()
                except:
                    pass
            
            # Close SSH connection if open
            if app.ssh_client:
                print("Closing SSH connection...")
                try:
                    app.ssh_client.close()
                except:
                    pass
            
            # Close any matplotlib windows
            try:
                import matplotlib.pyplot as plt
                plt.close('all')
                print("Closed matplotlib windows")
            except:
                pass
            
            print("Terminating application...")
            
            # Destroy the root window
            root.quit()  # Exit mainloop
            root.destroy()  # Destroy window
            
            print("Application closed successfully")
            
        except Exception as e:
            print(f"Error during shutdown: {e}")
            # Force exit if normal shutdown fails
            try:
                import sys
                sys.exit(0)
            except:
                pass
    
    # Set window close protocol
    root.protocol("WM_DELETE_WINDOW", on_closing)
    
    try:
        root.mainloop()
    except KeyboardInterrupt:
        print("\nKeyboard interrupt received, shutting down...")
        on_closing()

if __name__ == "__main__":
    main() 