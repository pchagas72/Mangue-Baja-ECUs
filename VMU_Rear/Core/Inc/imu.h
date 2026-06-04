/*
 * @brief: 
 * - This code gets the raw readings from the lsm6ds3.c file
 *   and converts them to readable and processed roll and pitch.
 */

#ifndef IMU_H
#define IMU_H

#include <stdint.h>
#include <stdbool.h>
#include "lsm6ds3.h"
#include "kalman.h"
#include "stm32f1xx_hal.h"
#include <math.h>
#include "i2c.h"

typedef struct {
    int16_t roll;
    int16_t pitch;
} imu_processed_data_t;

extern Kalman_t KalmanRoll;
extern Kalman_t KalmanPitch;
extern imu_processed_data_t imu_data;
extern LSM6DS3_Data_t lsm6ds3_raw_data;

bool IMU_Process(imu_processed_data_t *out_data);

#endif /* IMU_H */
