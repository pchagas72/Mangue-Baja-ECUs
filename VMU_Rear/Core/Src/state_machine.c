#include "state_machine.h"
#include "can_management.h"
#include <math.h> // Resolve o erro da função log()
#include "adc.h"  // Resolve hadc1
#include "can.h"  // Resolve hcan
#include "i2c.h"  // Resolve hi2c1

/* Variável global que vem do main.c via EXTI (Sensor Indutivo) */
extern volatile uint32_t contador_pulsos_indutivo;

/* Timers estáticos para o Superloop Não-Bloqueante */
static uint32_t last_speed_tick = 0;
static uint32_t last_adc_tick = 0;
static uint32_t last_mlx_tick = 0;
static uint32_t last_debug_tick = -10000; // Força envio imediato no primeiro loop
static uint32_t last_led_tick = 0;

void StateMachine_Init(tcu_state_t *tcu_current_state) {
    tcu_current_state->current_state = STATE_BOOT;
    tcu_current_state->CAN_initialized = false;
    tcu_current_state->CAN_ok = false;
    tcu_current_state->MLX_initialized = false;
    tcu_current_state->MLX_ok = false;
    tcu_current_state->boot_tries = 0;
}

void StateMachine_Update(tcu_state_t *tcu_current_state) {

    uint32_t current_tick = HAL_GetTick();

    switch (tcu_current_state->current_state) {

        case STATE_BOOT:
            // 1. Init MLX90614 Sensor
            if (!tcu_current_state->MLX_initialized) {
                if (HAL_I2C_IsDeviceReady(&hi2c1, MLX90614_I2C_ADDR, 3, 10) == HAL_OK) {
                    tcu_current_state->MLX_initialized = true;
                    for (int i = 0; i <6; i++){
                    	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                    	HAL_Delay(30);
                    }

                    HAL_Delay(500);
                }
            }

            // 2. Init CAN Network
            if (!tcu_current_state->CAN_initialized) {
                CAN_FilterTypeDef canfilterconfig;
                canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
                canfilterconfig.FilterBank = 0;
                canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
                canfilterconfig.FilterIdHigh = 0x0000;
                canfilterconfig.FilterIdLow = 0x0000;
                canfilterconfig.FilterMaskIdHigh = 0x0000;
                canfilterconfig.FilterMaskIdLow = 0x0000;
                canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
                canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;

                if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig) == HAL_OK &&
                    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING) == HAL_OK &&
                    HAL_CAN_Start(&hcan) == HAL_OK) {
                    tcu_current_state->CAN_initialized = true;
                }
            }

            /* Transição de Boot (50 tentativas = 500ms) */
            if (tcu_current_state->CAN_initialized && tcu_current_state->MLX_initialized) {
                tcu_current_state->current_state = STATE_SELF_CHECK;
                for (int i = 0; i <6; i++){
                	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                	HAL_Delay(30);
                }

                HAL_Delay(500);
            } else if (tcu_current_state->boot_tries > 50) {
                if (tcu_current_state->CAN_initialized) {
                    tcu_current_state->current_state = STATE_SELF_CHECK; // Avanca mesmo sem o MLX
                } else {
                    tcu_current_state->current_state = STATE_ERROR; // Sem a CAN a ECU não tem utilidade
                }
            }

            HAL_Delay(10);
            tcu_current_state->boot_tries += 1;
            break;


        case STATE_SELF_CHECK:
            // Testa se a I2C está lendo algo real
            if (tcu_current_state->MLX_initialized) {
                uint8_t mlx_test[3];
                if (HAL_I2C_Mem_Read(&hi2c1, MLX90614_I2C_ADDR, MLX90614_REG_TOBJ1, I2C_MEMADD_SIZE_8BIT, mlx_test, 3, 10) == HAL_OK) {
                    tcu_current_state->MLX_ok = true;
                } else {
                    tcu_current_state->MLX_ok = false;
                }
            } else {
                tcu_current_state->MLX_ok = false;
            }

            // Assume CAN ok para iniciar (Pode adicionar pacote de Ping futuramente)
            tcu_current_state->CAN_ok = true;

            if (tcu_current_state->CAN_ok) {
                tcu_current_state->current_state = STATE_RUNNING;
            } else {
                tcu_current_state->current_state = STATE_ERROR;
            }
            break;


        case STATE_RUNNING:

            /* 100ms Task -> Lê e envia Velocidade */
            if (current_tick - last_speed_tick >= SPEED_DELAY) {
                uint32_t dt_ms = current_tick - last_speed_tick;
                last_speed_tick = current_tick;

                __disable_irq();
                uint32_t pulsos_locais = contador_pulsos_indutivo;
                contador_pulsos_indutivo = 0;
                __enable_irq();

                float speed_hz = 0;
                if (dt_ms > 0) {
                    speed_hz = (pulsos_locais * 1000.0f) / (float)dt_ms;
                }
                float speed_kmh = (3.6f * PI * WHEEL_DIAMETER * speed_hz) / WHEEL_HOLES_NUMBER_REAR;

                CAN_Send_Speed((uint16_t)speed_kmh);
            }

            /* 100ms Task -> Lê e envia ADC (Voltagem + Temperatura Motor) */
            if (current_tick - last_adc_tick >= ADC_DELAY) {
                last_adc_tick = current_tick;
                ADC_ChannelConfTypeDef sConfig = {0};

                // Leitura Canal 4 (Voltagem)
                sConfig.Channel = ADC_CHANNEL_4;
                sConfig.Rank = ADC_REGULAR_RANK_1;
                sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
                HAL_ADC_ConfigChannel(&hadc1, &sConfig);

                HAL_ADC_Start(&hadc1);
                if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
                    uint32_t adc_raw_value = HAL_ADC_GetValue(&hadc1);
                    float voltage_at_pin = ((float)adc_raw_value / 4095.0f) * 3.3f;
                    float actual_vcc_voltage = voltage_at_pin * 5.0f;

                    CAN_Send_Voltage((uint16_t)(actual_vcc_voltage * 100.0f));
                }
                HAL_ADC_Stop(&hadc1);

                // Leitura Canal 0 (NTC)
                sConfig.Channel = ADC_CHANNEL_0;
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

            /* 100ms Task -> Lê e envia Temperatura CVT via MLX90614 */
            if (tcu_current_state->MLX_ok && (current_tick - last_mlx_tick >= MLX_DELAY)) {
                last_mlx_tick = current_tick;
                uint8_t mlx_data[3];

                if (HAL_I2C_Mem_Read(&hi2c1, MLX90614_I2C_ADDR, MLX90614_REG_TOBJ1, I2C_MEMADD_SIZE_8BIT, mlx_data, 3, 10) == HAL_OK) {
                    uint16_t temp_raw = (mlx_data[1] << 8) | mlx_data[0];
                    float temp_kelvin = temp_raw * 0.02f;
                    float mlx_temperature_celsius = temp_kelvin - 273.15f;

                    CAN_Send_Temp_CVT((int16_t)(mlx_temperature_celsius * 100.0f));
                } else {
                    tcu_current_state->MLX_ok = false; // Se a leitura falhar, derruba a flag
                }
            }

            /* 5000ms Task -> Pacote de Saúde/Debug (Health Package) */
            if (current_tick - last_debug_tick >= DEBUG_DELAY) {
                last_debug_tick = current_tick;
                can_debug_packet_t debug_packet;
                debug_packet.current_state = tcu_current_state->current_state;
                debug_packet.CAN_ok = tcu_current_state->CAN_ok;
                debug_packet.MLX_ok = tcu_current_state->MLX_ok;
                debug_packet.boot_tries = tcu_current_state->boot_tries;

                CAN_Send_Debug(&debug_packet);
            }

            /* Pisca LED de Heartbeat (100ms) */
            if (current_tick - last_led_tick >= 100){
                last_led_tick = current_tick;
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            }
            break;


        case STATE_ERROR:
            /* Tenta reiniciar o processo 10 vezes em caso de falha severa */
            if (tcu_current_state->boot_tries < 500) {
                HAL_Delay(1000);
                StateMachine_Init(tcu_current_state);
                for (int i = 0; i <60; i++){
                	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                	HAL_Delay(30);
                }

                HAL_Delay(500);
            }
            break;
    }
}
