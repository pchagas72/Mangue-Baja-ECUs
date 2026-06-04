#include "../Inc/imu.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Kalman_t KalmanRoll;
Kalman_t KalmanPitch;
imu_processed_data_t imu_data;
LSM6DS3_Data_t lsm6ds3_raw_data;
static uint32_t ultimo_tick = 0;

bool IMU_Process(imu_processed_data_t *out_data) {

    if (LSM6DS3_Read(&hi2c1, &lsm6ds3_raw_data) == 1) {
        uint32_t tick_atual = HAL_GetTick();
        
        // Proteção na primeira leitura
        if (ultimo_tick == 0) ultimo_tick = tick_atual - 1; 

        float dt = (float)(tick_atual - ultimo_tick) / 1000.0f;
        ultimo_tick = tick_atual;

        if (dt > 0.0f) {
            float roll_acc = atan2f((float)lsm6ds3_raw_data.acc_y, (float)lsm6ds3_raw_data.acc_z) * 180.0f / M_PI;
            float pitch_acc = atan2f((float)-lsm6ds3_raw_data.acc_x, sqrtf((float)lsm6ds3_raw_data.acc_y * lsm6ds3_raw_data.acc_y + (float)lsm6ds3_raw_data.acc_z * lsm6ds3_raw_data.acc_z)) * 180.0f / M_PI;

            // Sensibilidade padrão: 8.75 mdps/LSB -> 0.00875
            float gyro_rate_x = (float)lsm6ds3_raw_data.gyr_x * 0.00875f;
            float gyro_rate_y = (float)lsm6ds3_raw_data.gyr_y * 0.00875f;

            float roll_kalman = Kalman_getAngle(&KalmanRoll, roll_acc, gyro_rate_x, dt);
            float pitch_kalman = Kalman_getAngle(&KalmanPitch, pitch_acc, gyro_rate_y, dt);

            out_data->roll = (int16_t)roll_kalman;
            out_data->pitch = (int16_t)pitch_kalman;
        }
        return true;
    }
    return false; // Falha na leitura I2C
}
