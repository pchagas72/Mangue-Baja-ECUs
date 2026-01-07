#ifndef CON_STATE_MACHINE_H
#define CON_STATE_MACHINE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
//#include <ArduinoJson.h>
#include <Ticker.h>
#include "CAN.h"
#include "gprs_defs.h"
#include "hardware_defs.h"
#include "packets.h" 

/* Credentials Variables */
//#define TIM     // Uncomment this line and comment the others if this is your chip
//#define CLARO   // Uncomment this line and comment the others if this is your chip
//#define VIVO    // Uncomment this line and comment the others if this is your chip

/* State Machines */
typedef enum {
    DISCONNECTED    = (0 << 1), 
    CONNECTED       = (1 << 0), 
    RESERVED_ID_4x4 = (1 << 1), // this ID is set in front ECU
    ERROR_CONECTION = (1 << 2)
} connectivity_states;

uint8_t Initialize_GSM(void);
void gsmCallback(char *topic, byte *payload, unsigned int length);
boolean Check_mqtt_client_conection(void);
void gsmReconnect(uint8_t &_try_reconect);
void Send_msg_MQTT(void);
void publishPacket(void *T, uint32_t len);

/* Ticker functions */
void setup_GSM_tic(void);

/* Interrupts routine */
void ticker1HzISR(void);
void ticker20HzISR(void);

#endif