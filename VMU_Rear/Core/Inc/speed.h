#ifndef SPEED_H
#define SPEED_H

#include "main.h"
#include <stdint.h>
#include "low_voltage_mode.h"
#include "can_management.h"

#define SPEED_DELAY              100  // ms
#define PI                       3.1416f
#define WHEEL_DIAMETER           0.5842f
#define WHEEL_HOLES_NUMBER_REAR  12.0f

void Speed_Task(uint32_t current_tick, bool is_low_battery, uint32_t contador_pulsos_indutivo, uint32_t last_speed_tick);

#endif // SPEED_H
