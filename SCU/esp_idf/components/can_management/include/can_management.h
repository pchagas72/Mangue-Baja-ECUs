#pragma once

#include <stdint.h>

// The default pins are:
//      TX -> NUM_21
//      TX -> NUM_22

#define CAN_TX_GPIO GPIO_NUM_21
#define CAN_RX_GPIO GPIO_NUM_22

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

typedef struct
{
    // Data sent through the Temperature Control Unit
    float volt;
    uint8_t SOC;
    uint8_t cvt;
    float current;
    uint8_t temperature;
    uint16_t speed;

    // Data sent through the Man Machine Interface
    imu_acc_t imu_acc;
    imu_dps_t imu_dps;
    Angle_t Angle;
    uint16_t rpm;
    uint8_t flags; // No usage as of now

    // Data send through the Mapping and Positioning Unit
    double latitude;
    double longitude;

    // Real Time Clock timestamp
    uint32_t timestamp;

} can_packet;

/* IDs */
#define BUFFER_SIZE     50
#define THROTTLE_MID    0x00
#define THROTTLE_RUN    0x01
#define THROTTLE_CHOKE  0x02

#define SYNC_ID         0x001       // message for bus sync
#define THROTTLE_ID     0x100       // 1by = throttle state (0x00, 0x01 or 0x02)
#define FLAGS_ID        0x101       // 1by
#define IMU_ACC_ID      0x200       // 6by           
#define IMU_DPS_ID      0x201       // 6by          
#define ANGLE_ID        0X205       // 4by
#define SPEED_ID        0x300       // 2by          
#define SOC_ID          0x302       // 1by
#define RPM_ID          0x304       // 2by           
#define SOT_ID          0x305       // 1by
#define TEMPERATURE_ID  0x400       // 1by     
#define CVT_ID          0x401       // 1by
#define FUEL_ID         0x500       // 2by           
#define VOLTAGE_ID      0x502       // 4by
#define CURRENT_ID      0x505       // 4by
#define LAT_ID          0x600       // 8by
#define LNG_ID          0x700       // 8by

#define MMI_ID          0x306
#define TCU_ID          0x307
#define SCU_ID          0x308
#define MPU_ID          0x309

void initialize_CAN(const char *TAG);
int read_packet(const char *TAG, can_packet *out_packet);
void get_fixed_packet(can_packet *out_packet);
