#include "../Inc/cvt_temperature.h"

#define MLX90614_I2C_ADDR       (0x5A << 1)
#define MLX90614_REG_TOBJ1      0x07

void CVTTemp_Task(uint32_t current_tick, bool is_low_battery, tcu_state_t *tcu_current_state){
    static uint32_t last_mlx_tick = 0;
    
    uint32_t current_delay = is_low_battery ? CVT_TEMP_LB_DELAY : CVT_TEMP_DELAY;

    if (current_tick - last_mlx_tick >= current_delay) {
        last_mlx_tick = current_tick;
        uint8_t mlx_data[3];

        if (HAL_I2C_Mem_Read(&hi2c1, MLX90614_I2C_ADDR, MLX90614_REG_TOBJ1, I2C_MEMADD_SIZE_8BIT, mlx_data, 3, 50) == HAL_OK) {
            tcu_current_state->MLX_ok = true; 

            uint16_t temp_raw = (mlx_data[1] << 8) | mlx_data[0];
            float temp_kelvin = temp_raw * 0.02f;
            float mlx_temperature_celsius = temp_kelvin - 273.15f;

            CAN_Send_Temp_CVT((int16_t)(mlx_temperature_celsius * 100.0f));
        } else {
            tcu_current_state->MLX_ok = false;
        }
    }
}
