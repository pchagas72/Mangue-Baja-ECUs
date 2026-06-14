#ifndef CVT_TEMPERATURE_H
#define CVT_TEMPERATURE_H

#include "main.h"
#include "state_machine.h"
#include "i2c.h"
#include "can_management.h"
#include "low_voltage_mode.h"
#include <stdint.h>

#define CVT_TEMP_DELAY 100

void CVTTemp_Task(uint32_t current_tick, bool is_low_battery, tcu_state_t *tcu_current_state);

#endif // CVT_TEMPERATURE_H
