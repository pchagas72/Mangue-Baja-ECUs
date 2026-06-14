#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdbool.h>
#include "lsm6ds3.h"
#include "i2c.h"
#include "kalman.h"
#include "imu.h"
#include "rpm.h"
#include "can.h"
#include "can_management.h"
#include "stm32f1xx_hal_conf.h"
#include "stm32f1xx_it.h"
#include "low_voltage_mode.h"

enum VMU_CENTER_STATES {
    STATE_BOOT = 0,
    STATE_SELF_CHECK = 1,
    STATE_RUNNING = 2,
    STATE_ERROR = -1,
};

typedef struct vmu_state{
    enum VMU_CENTER_STATES current_state;
    int boot_tries;
    bool IMU_initialized;
    bool IMU_ok;
    bool CAN_initialized;
    bool CAN_ok;
    /* Makes no sense to initialize RPM */
    /* This warning works like an engine light */
    bool RPM_warning;
    bool low_voltage;
} vmu_state_t;

void StateMachine_Init(vmu_state_t *vmu_current_state);
void StateMachine_Update(vmu_state_t *vmu_current_state);

#endif
