#!/usr/bin/env python3
"""
esp32BLEupdater.py

A simple GUI application to update ESP32 firmware via BLE.
The application:
  - Scans for BLE devices and filters by known ESP32 device name.
  - Lets the user select a .bin file.
  - Once a device and file are selected, enables the Update button.
  - Performs a handshake and sends the file in packets via BLE.
  - Displays progress and notifications from the ESP32 in a console.
  - Saves the log in the same folder as the .bin file with a timestamp.
  
This code is organized into distinct sections to follow SOLID principles and clean architecture.
"""

import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext
import asyncio
from bleak import BleakScanner, BleakClient
import threading
import time
import datetime
import os
import math

# ---------------------- CONSTANTS ----------------------
SERVICE_UUID   = "12345678-1234-1234-1234-1234567890ab"
HANDSHAKE_UUID = "12345678-1234-1234-1234-1234567890ac"
DATA_UUID      = "12345678-1234-1234-1234-1234567890ad"
ACK_UUID       = "12345678-1234-1234-1234-1234567890ae"
PACKET_SIZE    = 512
ESP32_NAME_FILTER = "ESP32FirmwareUpdate"  # Known device name used by ESP32

# ---------------------- LOGGER CLASS ----------------------
class Logger:
    def __init__(self, text_widget):
        self.text_widget = text_widget
        self.log_messages = []
        self.lock = threading.Lock()

    def log(self, message):
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
        full_message = f"[{timestamp}] {message}"
        with self.lock:
            self.log_messages.append(full_message)
        # Update the text widget from the main thread
        self.text_widget.after(0, lambda: self._append_message(full_message))

    def _append_message(self, message):
        self.text_widget.insert(tk.END, message + "\n")
        self.text_widget.see(tk.END)

    def save(self, bin_path, start_time):
        base = os.path.splitext(bin_path)[0]
        timestamp = start_time.strftime("%Y%m%d_%H%M")
        log_filename = f"{base}_{timestamp}.log"
        try:
            with open(log_filename, "w") as f:
                for msg in self.log_messages:
                    f.write(msg + "\n")
            self.log(f"Log saved to {log_filename}")
        except Exception as e:
            self.log(f"Error saving log: {e}")

# ---------------------- FILE MANAGER ----------------------
class FileManager:
    def __init__(self, packet_size):
        self.packet_size = packet_size

    def load_file(self, path):
        with open(path, "rb") as f:
            data = f.read()
        return data

    def split_file(self, data):
        total_size = len(data)
        total_packets = math.ceil(total_size / self.packet_size)
        packets = []
        for i in range(total_packets):
            start = i * self.packet_size
            end = start + self.packet_size
            packets.append(data[start:end])
        return total_size, total_packets, packets

# ---------------------- BLE MANAGER ----------------------
class BLEManager:
    def __init__(self, logger):
        self.logger = logger
        self.client = None

    async def scan_for_device(self):
        self.logger.log("Scanning for BLE devices...")
        devices = await BleakScanner.discover(timeout=5.0)
        filtered = [d for d in devices if d.name and ESP32_NAME_FILTER in d.name]
        if filtered:
            self.logger.log(f"Found {len(filtered)} device(s) matching filter.")
            for dev in filtered:
                self.logger.log(f"Device: {dev.name}, Address: {dev.address}")
            return filtered[0]  # Selecciona el primero encontrado
        else:
            self.logger.log("No devices found matching filter.")
            return None

    async def connect(self, address):
        self.logger.log(f"Connecting to device at {address}...")
        self.client = BleakClient(address)
        try:
            await self.client.connect()
            self.logger.log("Connected successfully.")
            await self.client.start_notify(ACK_UUID, self.notification_handler)
            return True
        except Exception as e:
            self.logger.log(f"Connection failed: {e}")
            return False

    async def disconnect(self):
        if self.client:
            self.logger.log("Disconnecting device...")
            try:
                await self.client.disconnect()
                self.logger.log("Disconnected successfully.")
            except Exception as e:
                self.logger.log(f"Disconnection error: {e}")

    def notification_handler(self, sender, data):
        if len(data) >= 3:
            packet_number = int.from_bytes(data[0:2], byteorder='little')
            status = "OK" if data[2] == 1 else "ERROR"
            self.logger.log(f"ESP32 Ack: Packet {packet_number} - {status}")

    async def send_handshake(self, total_size, total_packets):
        handshake = total_size.to_bytes(4, byteorder='little') + total_packets.to_bytes(2, byteorder='little')
        self.logger.log("Sending handshake...")
        try:
            await self.client.write_gatt_char(HANDSHAKE_UUID, handshake, response=True)
            self.logger.log("Handshake sent.")
            return True
        except Exception as e:
            self.logger.log(f"Handshake failed: {e}")
            return False

    async def send_packet(self, packet):
        try:
            await self.client.write_gatt_char(DATA_UUID, packet, response=True)
            return True
        except Exception as e:
            self.logger.log(f"Failed to send packet: {e}")
            return False

# ---------------------- UPDATE CONTROLLER ----------------------
class UpdateController:
    def __init__(self, ble_manager, file_manager, logger):
        self.ble_manager = ble_manager
        self.file_manager = file_manager
        self.logger = logger

    async def perform_update(self, device, bin_path):
        start_time = datetime.datetime.now()
        connected = await self.ble_manager.connect(device.address)
        if not connected:
            self.logger.log("Unable to connect to device.")
            return
        try:
            data = self.file_manager.load_file(bin_path)
        except Exception as e:
            self.logger.log(f"Failed to load file: {e}")
            await self.ble_manager.disconnect()
            return
        total_size, total_packets, packets = self.file_manager.split_file(data)
        self.logger.log(f"File loaded: {total_size} bytes, {total_packets} packets.")

        handshake_ok = await self.ble_manager.send_handshake(total_size, total_packets)
        if not handshake_ok:
            self.logger.log("Handshake failed, aborting update.")
            await self.ble_manager.disconnect()
            return

        for i, packet in enumerate(packets, start=1):
            self.logger.log(f"Sending packet {i}/{total_packets}...")
            success = await self.ble_manager.send_packet(packet)
            if not success:
                self.logger.log(f"Error sending packet {i}, aborting update.")
                await self.ble_manager.disconnect()
                return
            await asyncio.sleep(0.05)  # Breve retardo entre paquetes

        self.logger.log("All packets sent. Update process complete.")
        await self.ble_manager.disconnect()
        self.logger.save(bin_path, start_time)

# ---------------------- APP GUI ----------------------
class AppGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 BLE Updater")
        self.ble_manager = BLEManager(logger=None)  # Logger se asigna luego
        self.file_manager = FileManager(PACKET_SIZE)
        self.update_controller = None
        self.selected_device = None
        self.bin_path = None

        self.create_widgets()
        self.logger = Logger(self.console)
        self.ble_manager.logger = self.logger
        self.update_controller = UpdateController(self.ble_manager, self.file_manager, self.logger)

    def create_widgets(self):
        main_frame = tk.Frame(self.root)
        main_frame.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)

        # Sección de dispositivo BLE
        device_frame = tk.LabelFrame(main_frame, text="BLE Device")
        device_frame.pack(fill=tk.X, padx=5, pady=5)
        self.scan_button = tk.Button(device_frame, text="Scan Devices", command=self.scan_devices)
        self.scan_button.pack(side=tk.LEFT, padx=5, pady=5)
        self.device_label = tk.Label(device_frame, text="No device selected")
        self.device_label.pack(side=tk.LEFT, padx=5, pady=5)

        # Sección de selección de archivo
        file_frame = tk.LabelFrame(main_frame, text="Firmware File")
        file_frame.pack(fill=tk.X, padx=5, pady=5)
        self.file_button = tk.Button(file_frame, text="Select .bin File", command=self.select_file)
        self.file_button.pack(side=tk.LEFT, padx=5, pady=5)
        self.file_label = tk.Label(file_frame, text="No file selected")
        self.file_label.pack(side=tk.LEFT, padx=5, pady=5)

        # Botón de update
        self.update_button = tk.Button(main_frame, text="Update", command=self.start_update, state=tk.DISABLED)
        self.update_button.pack(padx=5, pady=5)

        # Consola de feedback
        console_frame = tk.LabelFrame(main_frame, text="Console")
        console_frame.pack(fill=tk.BOTH, padx=5, pady=5, expand=True)
        self.console = scrolledtext.ScrolledText(console_frame, height=10, state=tk.NORMAL)
        self.console.pack(fill=tk.BOTH, padx=5, pady=5, expand=True)

    def scan_devices(self):
        self.update_button.config(state=tk.DISABLED)
        self.device_label.config(text="Scanning...")
        threading.Thread(target=self._scan_devices_thread, daemon=True).start()

    def _scan_devices_thread(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        device = loop.run_until_complete(self.ble_manager.scan_for_device())
        if device:
            self.selected_device = device
            self.device_label.config(text=f"{device.name} ({device.address})")
        else:
            self.device_label.config(text="No device found")
        if self.selected_device and self.bin_path:
            self.update_button.config(state=tk.NORMAL)
        loop.close()

    def select_file(self):
        path = filedialog.askopenfilename(filetypes=[("Binary Files", "*.bin")])
        if path:
            self.bin_path = path
            self.file_label.config(text=os.path.basename(path))
            if self.selected_device and self.bin_path:
                self.update_button.config(state=tk.NORMAL)

    def start_update(self):
        if not (self.selected_device and self.bin_path):
            messagebox.showerror("Error", "Device and file must be selected.")
            return
        self.update_button.config(state=tk.DISABLED)
        threading.Thread(target=self._update_thread, daemon=True).start()

    def _update_thread(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        loop.run_until_complete(self.update_controller.perform_update(self.selected_device, self.bin_path))
        loop.close()
        self.update_button.config(state=tk.NORMAL)

# ---------------------- MAIN ----------------------
if __name__ == "__main__":
    root = tk.Tk()
    app = AppGUI(root)
    root.mainloop()
