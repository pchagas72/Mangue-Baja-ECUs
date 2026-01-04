This is the "old" code for the Storage Control Unit. It will be refactored in order to move the codebase from platformIO to ESP-idf.

These are the necessary changes for this transition:

The current CAN-bus driver is very limited and not compatible with ESP-idf. In order to get closer to the industry standards the Two Wire Automotive Interface (TWAI) driver, native to the ESP-idf framework will be implemented.
