/* STM32CubeIDE HAL includes */
#include "adc.h"  
#include "can.h"  
#include "i2c.h"  

/* Custom includes */
#include "../Inc/state_machine.h"
#include "../Inc/low_voltage_mode.h"
#include "can_management.h"
#include "speed.h"
#include "voltage.h"
#include "motor_temperature.h"
#include "cvt_temperature.h"
#include "low_voltage_mode.h"

/* C std libs includes */
#include <stdint.h>
#include <stdbool.h>

#define DEBUG_DELAY    5000 // ms
extern volatile uint32_t contador_pulsos_indutivo;
static uint32_t last_speed_tick = 0;

void StateMachine_Init(tcu_state_t *tcu_current_state) {
    tcu_current_state->current_state = STATE_BOOT;
    tcu_current_state->CAN_initialized = false;
    tcu_current_state->CAN_ok = false;
    tcu_current_state->MLX_initialized = false;
    tcu_current_state->MLX_ok = false;
    tcu_current_state->low_voltage = false;
    tcu_current_state->boot_tries = 0;
}

void StateMachine_Update(tcu_state_t *tcu_current_state) {

    uint32_t current_tick = HAL_GetTick();

    switch (tcu_current_state->current_state) {

        case STATE_BOOT:
            /* Init MLX90614 Sensor */
            if (!tcu_current_state->MLX_initialized) {
                if (HAL_I2C_IsDeviceReady(&hi2c1, (0x5A << 1), 3, 10) == HAL_OK) {
                    tcu_current_state->MLX_initialized = true;
                }
            }

            /* Init CAN */
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

            /* Tries to initialize everything 50 times */
            if (tcu_current_state->CAN_initialized && tcu_current_state->MLX_initialized) {
                tcu_current_state->current_state = STATE_SELF_CHECK;
            } else if (tcu_current_state->boot_tries > 50) {
                if (tcu_current_state->CAN_initialized) {
                    tcu_current_state->current_state = STATE_SELF_CHECK; 
                } else {
                    tcu_current_state->current_state = STATE_ERROR;
                }
            }
            HAL_Delay(10);
            tcu_current_state->boot_tries += 1;
            break;

        case STATE_SELF_CHECK:
            /* Tests I²C communication with MLX90614 */
            if (tcu_current_state->MLX_initialized) {
                uint8_t mlx_test[3];
                if (HAL_I2C_Mem_Read(&hi2c1, (0x5A << 1), 0x07, I2C_MEMADD_SIZE_8BIT, mlx_test, 3, 10) == HAL_OK) {
                    tcu_current_state->MLX_ok = true;
                } else {
                    tcu_current_state->MLX_ok = false;
                }
            } else {
                tcu_current_state->MLX_ok = false;
            }

            tcu_current_state->CAN_ok = true;

            if (tcu_current_state->CAN_ok) {
                tcu_current_state->current_state = STATE_RUNNING;
            } else {
                tcu_current_state->current_state = STATE_ERROR;
            }
            break;

        case STATE_RUNNING:

            /* Delega a lógica e a temporização para os respectivos módulos */
            //Speed_Task(current_tick, tcu_current_state->low_voltage, contador_pulsos_indutivo, last_speed_tick);
            Voltage_Task(current_tick, tcu_current_state->low_voltage, tcu_current_state);
            MotorTemp_Task(current_tick, tcu_current_state->low_voltage);
            CVTTemp_Task(current_tick, tcu_current_state->low_voltage, tcu_current_state);

            uint32_t current_speed_delay = tcu_current_state->low_voltage ? SPEED_LB_DELAY : SPEED_DELAY;

            if (current_tick - last_speed_tick >= current_speed_delay) {
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

            /* Heartbeat LED blink: Zero blinks se low_voltage for true */
            if (!tcu_current_state->low_voltage) {
                static uint32_t last_led_tick = 0;
                if (current_tick - last_led_tick >= 100){
                    last_led_tick = current_tick;
                    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                }
            } else {
                /* Força o LED a ficar apagado (PC13 é active-low na maioria das Bluepills) */
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            }

            /* Debug CAN package */
            static uint32_t last_debug_tick = 0;
            if (current_tick - last_debug_tick >= DEBUG_DELAY) {
                last_debug_tick = current_tick;
                can_debug_packet_t debug_packet;
                debug_packet.current_state = tcu_current_state->current_state;
                debug_packet.CAN_ok = tcu_current_state->CAN_ok;
                debug_packet.MLX_ok = tcu_current_state->MLX_ok;
                debug_packet.boot_tries = tcu_current_state->boot_tries;

                CAN_Send_Debug(&debug_packet);
            }
            break;

        case STATE_ERROR:
            /* Tries to restart state machine 10 times in case of errors */
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
