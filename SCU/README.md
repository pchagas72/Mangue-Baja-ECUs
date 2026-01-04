This is the legacy code for the Storage Control Unit. It will be refactored to move the codebase from the Arduino framework to native ESP-IDF.

These are the necessary changes for this transition:

The current CAN-bus driver is limited. To align with industry standards, the Two-Wire Automotive Interface (TWAI) driver, native to the ESP-IDF framework, will be implemented.

For SD logging, a Virtual File System (VFS) will be used alongside the SPI protocol. This approach is more robust and standardizes file operations.

The most significant transition is replacing the TinyGSM Arduino library with the native esp_mqtt and esp_modem components. They are significantly more efficient, offer advanced features, and provide better debugging capabilities.
