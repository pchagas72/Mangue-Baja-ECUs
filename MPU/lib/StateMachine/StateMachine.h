#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <CircularBuffer.hpp>
#include <TinyGPSPlus.h>
#include <Ticker.h>
#include "LORA.h"
#include "CAN.h"
#include "MPU_defs.h"

void CircularBuffer_Ticker_Init(void);
void CircularBuffer_CurrentState(void);

bool get_GPS_data(void);

/* Tickers */
void ticker1HzISR(void);
void ticker250mHzISR(void);

// DEBUG FUNCTIONS
void PRINT_GPS_TIME(void);
void PRINT_GPS_DATE(void);

#endif
