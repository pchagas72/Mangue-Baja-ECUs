This repository contains the legacy firmware for the Mapping and Positioning Unit (MPU). The codebase is currently being refactored from the Arduino framework to the native ESP-IDF to improve real-time performance and reliability.

Key refactoring objectives:

CAN Bus: We are replacing the generic Arduino library with the Two-Wire Automotive Interface (TWAI) driver. This native implementation provides finer control over bus timing and error handling, bringing the system closer to automotive industry standards.

Telemetry (LoRa): We are deprecating the blocking EBYTE wrapper in favor of a custom UART driver using DMA (Direct Memory Access). This allows for non-blocking communication with the transceiver, ensuring that the main control loop remains deterministic even during heavy data transmission.
