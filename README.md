# ECUs Modified for the 2026 Nationals

For some reason, previous team members standardized English as the main documentation language, so we will continue that for now.

This firmware is outdated and will be revamped as soon as the current hardware issues are resolved.

## How to use the OTA library

After turning the esp32 chip on, you can connect to the "<BOARD_NAME> Mangue_Baja" network, and then the OTA interface will be found at 192.168.34.1:1880

After using a browser to access it, you'll be able to send the compiled binary to the board.

Always remember to keep the OTA.h implementation on the new firmware so it's always accessible.

## MPU

The Mapping and Positioning Unit (MPU), responsible for GPS location and LoRa 
communications, now implements the OTA protocol for wireless firmware updates.

## SCU

The Storage Control Unit (SCU), now fixed, implements SD storage of CAN data, 
an MQTT connection to the database, and the OTA protocol for firmware updates.

## /'Compiled Binaries'

The folder name is self-explanatory. It contains all the compiled binaries for 
easier access when sending via OTA.

## About the datalogging

This is the first code in this repo writen 100% by be, it is ultra simple and I'll be adding more sensors to it later.

## TODO's

- [ ] Write documentation for each ECU.
- [ ] Add real timestamp to datalogging.
- [ ] Send datalogging data through mqtt.
