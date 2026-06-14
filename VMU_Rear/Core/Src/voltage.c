#include "../Inc/voltage.h"

void Voltage_Task(uint32_t current_tick, bool low_voltage_mode, tcu_state_t *tcu_current_state){
    static uint32_t last_adc_tick = 0;
    
    /* Delay de leitura de tensão aumenta se a bateria já estiver fraca */
    uint32_t current_delay = low_voltage_mode ? ADC_LB_DELAY : ADC_DELAY;
    
    if (current_tick - last_adc_tick >= current_delay) {
        last_adc_tick = current_tick;
        ADC_ChannelConfTypeDef sConfig = {0};

        /* Reads channel 4 - Voltage */
        sConfig.Channel = ADC_CHANNEL_4;
        sConfig.Rank = ADC_REGULAR_RANK_1;
        sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
        HAL_ADC_ConfigChannel(&hadc1, &sConfig);

        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint32_t adc_raw_value = HAL_ADC_GetValue(&hadc1);
            float voltage_at_pin = ((float)adc_raw_value / 4095.0f) * 3.3f;
            float actual_vcc_voltage = voltage_at_pin * 5.0f;

            /* Atualiza a flag de bateria no estado global */
            tcu_current_state->low_voltage = (actual_vcc_voltage <= LOW_VOLTAGE_THRESHOLD);

            CAN_Send_Voltage((uint16_t)(actual_vcc_voltage * 100.0f));
        }
        HAL_ADC_Stop(&hadc1);
    }
}
