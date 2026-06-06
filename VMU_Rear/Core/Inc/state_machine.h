#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "main.h"
#include <stdbool.h>

/* Delays for each task */
#define SPEED_DELAY    100  // ms
#define ADC_DELAY      100  // ms
#define MLX_DELAY      100  // ms
#define DEBUG_DELAY    5000 // ms

/* Physics constants and wheel data */
#define PI                        3.1416f
#define WHEEL_DIAMETER            0.5842f
#define WHEEL_HOLES_NUMBER_REAR   12.0f

/* NTC constants */
#define R_FIXO                    1000.0f
#define NTC_R0                    10000.0f
#define NTC_T0                    298.15f
#define NTC_BETA                  3950.0f

/* I²C constants - MLX90614 */
#define MLX90614_I2C_ADDR         (0x5A << 1)
#define MLX90614_REG_TOBJ1        0x07

enum TCU_STATES {
    STATE_BOOT = 0,
    STATE_SELF_CHECK = 1,
    STATE_RUNNING = 2,
    STATE_ERROR = -1,
};

typedef struct tcu_state {
    enum TCU_STATES current_state;
    int boot_tries;
    bool CAN_initialized;
    bool CAN_ok;
    bool MLX_initialized;
    bool MLX_ok;
} tcu_state_t;

void StateMachine_Init(tcu_state_t *tcu_current_state);
void StateMachine_Update(tcu_state_t *tcu_current_state);

#endif // STATE_MACHINE_H
