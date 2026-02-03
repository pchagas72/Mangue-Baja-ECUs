#ifndef BLUETOOTH_DEFS_H
#define BLUETOOTH_DEFS_H

#include <stdint.h>

/* This struct is used to hold debug data that is sent over Bluetooth */
typedef struct
{
    uint8_t lora_init;
    uint8_t accel_begin;
    uint8_t termistor;
    uint8_t cvt_temperature;
    uint8_t measure_volt;
    uint8_t speed_pulse_counter;
    uint8_t servo_state;
    uint8_t internet_modem;
    uint8_t mqtt_client_connection;
    uint8_t sd_start;
    uint8_t check_sd;
} bluetooth;

#endif
