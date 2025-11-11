#ifndef HARDWARE_DEFS_H
#define HARDWARE_DEFS_H

#include "driver/gpio.h"

/* pinout definitions */
// LEDS
#define EMBEDDED_LED  GPIO_NUM_2 // Turned on on failiure
#define DEBUG_LED     GPIO_NUM_25

/* SD */ 
#define SD_CS         GPIO_NUM_5 
// SPI BUS
// VSPI
#define MISO          GPIO_NUM_19
#define MOSI          GPIO_NUM_23
#define SCK           GPIO_NUM_18

/* CAN PINS */ 
#define CAN_RX_id     GPIO_NUM_21  
#define CAN_TX_id     GPIO_NUM_22

/* GPRS */
#define MODEM_RST
#define MODEM_TX      GPIO_NUM_17
#define MODEM_RX      GPIO_NUM_16

#endif