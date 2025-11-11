# ECUs Modified for the 2026 Nationals

For some reason, previous team members standardized English as the main documentation language, so we will continue that for now.

This firmware is outdated and will be revamped as soon as the current hardware issues are resolved.

## MPU

The Mapping and Positioning Unit (MPU), responsible for GPS location and LoRa 
communications, now implements the OTA protocol for wireless firmware updates.

## SCU

The Storage Control Unit (SCU), now fixed, implements SD storage of CAN data, 
an MQTT connection to the database, and the OTA protocol for firmware updates.

## /'Compiled Binaries'

The folder name is self-explanatory. It contains all the compiled binaries for 
easier access when sending via OTA.

## TODO's

- [ ] Write documentation for each ECU.
