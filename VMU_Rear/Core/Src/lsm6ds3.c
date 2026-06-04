#include "lsm6ds3.h"

/*
 * TODO: Translate this code to fit design choices.
 */

uint8_t endereco_descoberto_stm32 = 0x00;

uint8_t LSM6DS3_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t chip_id = 0;
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(hi2c, 0xD6, LSM6DS3_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &chip_id, 1, 100);
    if (status == HAL_OK) {
        endereco_descoberto_stm32 = 0xD6;
    } else {
        HAL_Delay(10);
        status = HAL_I2C_Mem_Read(hi2c, 0xD4, LSM6DS3_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &chip_id, 1, 100);
        if (status == HAL_OK) {
            endereco_descoberto_stm32 = 0xD4;
        }
    }

    if (status != HAL_OK || (chip_id != 0x69 && chip_id != 0x6A)) {
        return 0;
    }

    // 1. Liga o Auto-Incremento para leitura em rajada não travar (Reg 0x12)
    uint8_t ctrl3_c = 0x04;
    HAL_I2C_Mem_Write(hi2c, endereco_descoberto_stm32, 0x12, I2C_MEMADD_SIZE_8BIT, &ctrl3_c, 1, 100);
    HAL_Delay(5);

    // 2. Acorda o Acelerómetro (104 Hz)
    uint8_t ctrl1_xl = 0x40;
    HAL_I2C_Mem_Write(hi2c, endereco_descoberto_stm32, LSM6DS3_CTRL1_XL, I2C_MEMADD_SIZE_8BIT, &ctrl1_xl, 1, 100);
    HAL_Delay(5);

    // 3. Acorda o Giroscópio (104 Hz)
    uint8_t ctrl2_g = 0x40;
    HAL_I2C_Mem_Write(hi2c, endereco_descoberto_stm32, LSM6DS3_CTRL2_G, I2C_MEMADD_SIZE_8BIT, &ctrl2_g, 1, 100);
    HAL_Delay(5);

    return 1; // SUCESSO!
}

uint8_t LSM6DS3_Read(I2C_HandleTypeDef *hi2c, LSM6DS3_Data_t *data) {
    uint8_t raw_buffer[12];

    if(endereco_descoberto_stm32 == 0x00) return 0;

    if (HAL_I2C_Mem_Read(hi2c, endereco_descoberto_stm32, LSM6DS3_OUTX_L_G, I2C_MEMADD_SIZE_8BIT, raw_buffer, 12, 100) == HAL_OK) {
        data->gyr_x = (int16_t)(raw_buffer[0] | (raw_buffer[1] << 8));
        data->gyr_y = (int16_t)(raw_buffer[2] | (raw_buffer[3] << 8));
        data->gyr_z = (int16_t)(raw_buffer[4] | (raw_buffer[5] << 8));

        data->acc_x = (int16_t)(raw_buffer[6] | (raw_buffer[7] << 8));
        data->acc_y = (int16_t)(raw_buffer[8] | (raw_buffer[9] << 8));
        data->acc_z = (int16_t)(raw_buffer[10] | (raw_buffer[11] << 8));

        return 1;
    }

    return 0;
}
