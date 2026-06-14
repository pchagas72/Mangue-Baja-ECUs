#ifndef MOTOR_TEMPERATURE_H
#define MOTOR_TEMPERATURE_H

#include "main.h"
#include "adc.h"
#include "can_management.h"
#include "state_machine.h"
#include "low_voltage_mode.h"
#include <math.h>
#include <stdint.h>

#define NTC_DELAY    100 // ms
#define R_FIXO       1000.0f
#define NTC_R0       10000.0f
#define NTC_T0       298.15f
#define NTC_BETA     3950.0f

void MotorTemp_Task(uint32_t current_tick, bool is_low_battery);

#endif // MOTOR_TEMPERATURE_H
