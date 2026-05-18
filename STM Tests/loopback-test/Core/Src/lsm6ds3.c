#include "lsm6ds3.h"

uint8_t LSM6DS3_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t chip_id;

    // Verifica se o sensor está vivo no barramento I2C
    if (HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, LSM6DS3_WHO_AM_I, 1, &chip_id, 1, 100) != HAL_OK) {
        return 0; // Erro I2C (falha de hardware/cabos)
    }

    // O WHO_AM_I do LSM6DS3 tem de retornar sempre 0x69
    if (chip_id != 0x69) {
        return 0;
    }

    // Configura Acelerómetro (CTRL1_XL): 104 Hz, fundo de escala 2g
    uint8_t ctrl1_xl = 0x40;
    HAL_I2C_Mem_Write(hi2c, LSM6DS3_ADDR, LSM6DS3_CTRL1_XL, 1, &ctrl1_xl, 1, 100);

    // Configura Giroscópio (CTRL2_G): 104 Hz, fundo de escala 245 dps
    uint8_t ctrl2_g = 0x40;
    HAL_I2C_Mem_Write(hi2c, LSM6DS3_ADDR, LSM6DS3_CTRL2_G, 1, &ctrl2_g, 1, 100);

    return 1; // Inicialização com sucesso!
}

void LSM6DS3_Read(I2C_HandleTypeDef *hi2c, LSM6DS3_Data_t *data) {
    uint8_t raw_buffer[12];

    // Aproveitamos a funcionalidade de auto-incremento de endereço do sensor.
    // Lemos 12 bytes de uma só vez (os 6 eixos do giroscópio e do acelerómetro).
    HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, LSM6DS3_OUTX_L_G, 1, raw_buffer, 12, 100);

    // A união dos bytes (Little-Endian)
    data->gyr_x = (int16_t)(raw_buffer[0] | (raw_buffer[1] << 8));
    data->gyr_y = (int16_t)(raw_buffer[2] | (raw_buffer[3] << 8));
    data->gyr_z = (int16_t)(raw_buffer[4] | (raw_buffer[5] << 8));

    data->acc_x = (int16_t)(raw_buffer[6] | (raw_buffer[7] << 8));
    data->acc_y = (int16_t)(raw_buffer[8] | (raw_buffer[9] << 8));
    data->acc_z = (int16_t)(raw_buffer[10] | (raw_buffer[11] << 8));
}
