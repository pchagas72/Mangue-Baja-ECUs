#ifndef MPU_DEFS_H
#define MPU_DEFS_H

#include <Arduino.h>
#include "packets.h"
#include "hard_defs.h"
#include "can_defs.h"

#define MB_ID  11

/* State Machines */
typedef enum {
    IDLE_ST, 
    RADIO_ST, 
    GPS_ST, 
    DEBUG_ST
} state_t;

//typedef enum {SENDING, LISTENING} connectivity_states;

//unsigned long timer;

#endif