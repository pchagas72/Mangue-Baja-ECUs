#ifndef HARD_DEFS_H
#define HARD_DEFS_H

#include <Arduino.h>
#include <driver/gpio.h>

#define EMBEDDED_LED    LED_BUILTIN

/* LoRa definitions */
#define LORA_RX         GPIO_NUM_17   // Serial2 RX (connect this to the EBYTE Tx pin)
#define LORA_TX         GPIO_NUM_16   // Serial2 TX pin (connect this to the EBYTE Rx pin)
#define PIN_M0          GPIO_NUM_12
#define PIN_M1          GPIO_NUM_14
#define PIN_AUX         GPIO_NUM_13
#define LoRaUART        Serial2
#define LoRa_Baud_Rate  9600

/* SD */ 
#define SD_CS           GPIO_NUM_5 
// SPI BUS
// VSPI
#define MISO            GPIO_NUM_19
#define MOSI            GPIO_NUM_23
#define SCK             GPIO_NUM_18

#endif
