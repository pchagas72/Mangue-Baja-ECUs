#pragma once
#include <stdint.h>
#include <stdbool.h>

// --- LEGACY IDs (From your defs.h) ---
#define ID_RPM          0x304
#define ID_SPEED        0x300
#define ID_ANGLE        0x205  // Assuming this contains Roll/Pitch
#define ID_CVT_TEMP     0x401
#define ID_ENG_TEMP     0x400
#define ID_VOLTAGE      0x502
#define ID_FUEL         0x500

typedef enum {
    CVT,
    BAT,
    FUEL,
} box_message;

// --- Data Structure for the Dashboard ---
typedef struct {
    uint16_t rpm;
    uint16_t speed;
    int16_t roll;       // Angle X
    int16_t pitch;      // Angle Y
    uint8_t cvt_temp;
    uint8_t eng_temp;
    float voltage;
    uint16_t fuel;
    bool link_active;   // Safety flag
    bool box_alert;
    box_message box_alert_message;
} car_state_t;

void can_init(void);
// Updates the state struct based on whatever messages are in the buffer
// Returns true if ANY data was updated
bool can_update_state(car_state_t *state);
