#ifndef LSM6DS3_H
#define LSM6DS3_H

#include "stm32f1xx_hal.h"

// Endereço travado em 0xD6 pois o pino SDO/SA0 está em 3.3V
#define LSM6DS3_ADDR 0xD6

// Registradores principais do sensor
#define LSM6DS3_WHO_AM_I 0x0F
#define LSM6DS3_CTRL1_XL 0x10
#define LSM6DS3_CTRL2_G  0x11
#define LSM6DS3_OUTX_L_G 0x22

typedef struct {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyr_x;
    int16_t gyr_y;
    int16_t gyr_z;
} LSM6DS3_Data_t;

uint8_t LSM6DS3_Init(I2C_HandleTypeDef *hi2c);
uint8_t LSM6DS3_Read(I2C_HandleTypeDef *hi2c, LSM6DS3_Data_t *data);

#endif
