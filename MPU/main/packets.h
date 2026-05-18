#ifndef PACKETS_H
#define PACKETS_H

#include <stdint.h>

// Instruct the compiler to NOT add padding bytes
#pragma pack(push, 1)
typedef struct {
    float volt;           // 4 bytes
    uint8_t SOC;          // 1 byte
    uint8_t cvt;          // 1 byte
    float current;        // 4 bytes
    uint8_t temperature;  // 1 byte
    uint8_t speed;        // 1 byte
    struct { int16_t x; int16_t y; int16_t z; } imu_acc; // 6 bytes
    struct { int16_t x; int16_t y; int16_t z; } imu_dps; // 6 bytes
    struct { int16_t Roll; int16_t Pitch; } Angle;       // 4 bytes
    uint16_t rpm;         // 2 bytes
    uint8_t flags;        // 1 byte
    double latitude;      // 8 bytes
    double longitude;     // 8 bytes
    uint32_t timestamp;   // 4 bytes
} __attribute__((packed)) radio_packet_t;
#pragma pack(pop)

#endif
