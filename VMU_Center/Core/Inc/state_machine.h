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

#define RPM_DELAY 100
#define IMU_DELAY 100
#define DEBUG_DELAY 10000

enum VMU_CENTER_STATES {
    STATE_BOOT = 0,
    STATE_SELF_CHECK = 1,
    STATE_RUNNING = 2,
    STATE_ERROR = -1,
};

/* 
 * current_state: Statemachine current job
 * boot_tries: Number of times the statemachine tried to start components
 * IMU_initialized: True if the I²C communication with the sensor is alive
 * IMU_ok: True if the sensor is sending live data
 * CAN_initialized: True if the CAN communication HAL could be initialized
 * CAN_ok: True if the CAN communication is tested by ping packages
 * Makes no sense to initialize RPM
 * RPM_warning: This warning tells if the car is either stalled or the RPM circuit is dead
 */
typedef struct vmu_state{
    enum VMU_CENTER_STATES current_state;
    int boot_tries;
    bool IMU_initialized;
    bool IMU_ok;
    bool CAN_initialized;
    bool CAN_ok;
    bool RPM_warning;
} vmu_state_t;

void StateMachine_Init(vmu_state_t *vmu_current_state);
void StateMachine_Update(vmu_state_t *vmu_current_state);

#endif
