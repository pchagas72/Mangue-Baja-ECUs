#ifndef VOLTAGE_H
#define VOLTAGE_H

#include "adc.h"
#include "can_management.h"
#include "main.h"
#include "state_machine.h"
#include "low_voltage_mode.h"
#include <stdint.h>

#define LOW_VOLTAGE_THRESHOLD 10.5f 
#define ADC_DELAY 100 // ms

void Voltage_Task(uint32_t current_tick, bool low_voltage_mode, tcu_state_t *tcu_current_state);

#endif // VOLTAGE_H
