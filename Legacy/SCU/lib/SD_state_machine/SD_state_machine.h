#ifndef SD_STATE_MACHINE_H
#define SD_STATE_MACHINE_H

#include <Arduino.h>
#include <SD.h>
#include <Ticker.h>
#include "CAN.h"
#include "hardware_defs.h"
#include "can_defs.h"
#include "packets.h" 

/* SD definitions */
uint8_t start_SD_device(void);
bool sdConfig(void);
int countFiles(File dir);
uint8_t sdSave(bool set); 
String packetToString(bool err);
uint8_t Check_SD_for_storage(void);

/* Ticker definitions */
void setup_SD_ticker(void);

/* Ticker interrupts */
void ticker40HzISR(void);

#endif
