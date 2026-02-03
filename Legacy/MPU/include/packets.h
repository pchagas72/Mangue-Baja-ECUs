// MPU_NACIONAL_2025-Transmitter/include/packets.h
#ifndef PACKETS_H
#define PACKETS_H

#include <stdio.h>
#include <string.h>

#define SUCESS_RESPONSE 2
#define FAIL_RESPONSE   1

#pragma pack(push,1)

typedef struct
{
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
} imu_acc_t;

typedef struct
{
    int16_t dps_x;
    int16_t dps_y;
    int16_t dps_z;
} imu_dps_t;

typedef struct
{
    int16_t Roll;
    int16_t Pitch;
} Angle_t;

// This struct is now aligned with the espidf_ecu's mqtt_packet_t
typedef struct
{
    /* REAR DATAS */
    float volt;
    uint8_t SOC;
    uint8_t cvt;
    float current;
    uint8_t temperature;
    uint16_t speed;

    /* FRONT DATAS */
    imu_acc_t imu_acc;
    imu_dps_t imu_dps;
    Angle_t Angle;
    uint16_t rpm;
    uint8_t flags;

    /* MPU DATAS */
    double latitude;
    double longitude;

    /* DEBUG DATA */
    uint32_t timestamp;

} radio_packet_t; // We keep the name radio_packet_t for consistency in this project

#pragma pack(pop)

#endif
