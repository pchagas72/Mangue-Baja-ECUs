#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <CANmsg.h>
#include "can_defs.h"
#include "hardware_defs.h"
#include "packets.h"

bool CAN_start_device(bool debug_mode = false);
void Send_SOT_msg(uint8_t _msg);
void Send_SCU_FLAGS(bluetooth ble);

mqtt_packet_t update_packet(void);
bool MPU_request_Debug_data(void);

/* Interrupt */
void canISR(CAN_FRAME *rxMsg);

#endif
