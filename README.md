# Mangue Baja - ECU Firmware (2026 Season)

This repository contains the embedded firmware for the Electronic Control Units (ECUs) of the Mangue Baja off-road vehicle. The system is built on the **ESP32** architecture, currently undergoing a refactoring process from the Arduino framework to **native ESP-IDF** to ensure deterministic behavior, real-time performance, and automotive-grade reliability.

## System Architecture

The ESP based vehicle electronics are distributed across three primary modules connected via **CAN Bus**:

### 1. Mapping and Positioning Unit (MPU)
Responsible for vehicle localization and long-range telemetry.
* **Functions:** GPS data acquisition and LoRa communication.
* **Development Status:** Refactoring to ESP-IDF.
    * **CAN:** Migrating to the native **TWAI (Two-Wire Automotive Interface)** driver for precise timing and error handling.
    * **LoRa:** Implementing a custom UART driver with **DMA (Direct Memory Access)** to replace blocking calls, ensuring the main control loop remains non-blocking during data transmission.

### 2. Storage Control Unit (SCU)
The central data acquisition and cloud telemetry node.
* **Functions:** Local data logging (SD Card) and MQTT telemetry via GSM.
* **Development Status:** Refactoring to ESP-IDF.
    * **Storage:** Implementing **Virtual File System (VFS)** with SPI for robust file operations.
    * **Connectivity:** Replacing generic libraries with native `esp_mqtt` and `esp_modem` components for efficient cellular communication.

### 3. Steering Wheel Interface
The primary Human-Machine Interface (HMI) for the driver.
* **Functions:** Visualizing critical ECU data (Fuel, Voltage, Engine Temp, Roll/Pitch) via an OLED display.
* **Framework:** Built entirely on **ESP-IDF**.

## Key Features

* **OTA Updates:** Supports Over-The-Air firmware updates via WiFi.
    * *Default Access Point:* `<BOARD_NAME> Mangue_Baja`
    * *Update Gateway:* `192.168.34.1:1880`
* **Automotive Protocol:** Utilizes standard CAN bus communication (0x500 series IDs) for inter-module data exchange.
* **Data Logging:** Local persistency on SD cards for post-run engineering analysis.

## Development Environment

The project utilizes the **Espressif IoT Development Framework (ESP-IDF)**.

**Build & Flash:**
```bash
# Setup environment (if not already done)
. $HOME/esp/esp-idf/export.sh

# Build the project
idf.py build

# Flash and Monitor
idf.py -p (PORT) flash monitor
```

## Repository Structure

* `/MPU` - Source code for GPS and LoRa telemetry.
* `/SCU` - Source code for SD logging and GSM/MQTT.
* `/Steering Wheel` - Source code for the driver display interface.
* `/Compiled` - Pre-compiled binaries for quick OTA deployment.
* `/Datalogging` - Firmware for tests that require sensors not present in the main harness

---
**Mangue Baja** | *Pernambuco, Brazil*
