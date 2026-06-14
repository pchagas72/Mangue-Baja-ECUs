#include "../Inc/motor_temperature.h"

void MotorTemp_Task(uint32_t current_tick, bool is_low_battery){
    static uint32_t last_ntc_tick = 0;
    
    uint32_t current_delay = is_low_battery ? NTC_LB_DELAY : NTC_DELAY;

    if (current_tick - last_ntc_tick >= current_delay) {
        last_ntc_tick = current_tick;
        ADC_ChannelConfTypeDef sConfig = {0};

        /* Reads channel 0 - NTC */
        sConfig.Channel = ADC_CHANNEL_0;
        sConfig.Rank = ADC_REGULAR_RANK_1;
        sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
        HAL_ADC_ConfigChannel(&hadc1, &sConfig);

        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint32_t adc_raw_ntc = HAL_ADC_GetValue(&hadc1);
            if (adc_raw_ntc > 0 && adc_raw_ntc < 4095) {
                float ntc_resistance = R_FIXO * ((float)adc_raw_ntc / (4095.0f - (float)adc_raw_ntc));
                float temperature_kelvin = 1.0f / ((1.0f / NTC_T0) + (1.0f / NTC_BETA) * log(ntc_resistance / NTC_R0));
                float temperature_celsius = temperature_kelvin - 273.15f;

                CAN_Send_Temp_NTC((int16_t)(temperature_celsius * 100.0f));
            }
        }
        HAL_ADC_Stop(&hadc1);
    }
}
