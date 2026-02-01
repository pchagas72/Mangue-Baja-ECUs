# Baja Steering Firmware

This is the main interface between the driver and the car electronic system.

### Pinout (ESP32)
| Pin  | Function | Connection |
| :--- | :--- | :--- |
| **IO 5** | CAN TX | Connect to TJA1050 **TX** |
| **IO 18** | CAN RX | Connect to TJA1050 **RX** |
| **IO 21** | I2C SDA | Connect to OLED SDA |
| **IO 22** | I2C SCL | Connect to OLED SCL |
| **IO 4** | OLED RST | Connect to OLED RES |
| **IO 0** | Mode Button | Button to GND (Internal Pull-up) |

This project uses the **Espressif IoT Development Framework (ESP-IDF)**.

1.  **Install ESP-IDF:**
    Follow the [official guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) to set up the environment.

2.  **Build:**
    ```bash
    idf.py build
    ```

3.  **Flash:**
    Connect the steering wheel via FTDI/USB and run:
    ```bash
    idf.py -p (PORT) flash monitor
    ```
---
*Mangue Baja - Pernambuco, Brazil* 🦀

## Todo:

- [ ] Add variables to the ESP bios
- Add more logs
    - [ ] CAN logging
- Receive flags from COM ecus [ ]
    - [ ] BOX to PILOT alert flags
- Send data to the CAN network [ ]
    - Pilot to box alert [ ]
    - Emergency alerts [ ]
        - [ ] No Radio - interface shows "Radio down"
