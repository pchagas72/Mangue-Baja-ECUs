#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PACKET_TYPE_PING    0x01
#define PACKET_TYPE_CMD     0x02

// This needs to be the exact same structure as the Base Station
typedef struct __attribute__((packed)) {
    uint8_t type;           // 1 byte
    uint32_t packet_id;     // 4 bytes
    uint64_t timestamp;     // 8 bytes (for ms precision)
    char payload[32];       // 32 bytes (pattern for integrity check)
} Message_Struct;

#endif
