#include "can_management.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // Required for vTaskDelay
#include "freertos/task.h"     // Required for vTaskDelay
#include <string.h>

#define CAN_TX_PIN GPIO_NUM_22
#define CAN_RX_PIN GPIO_NUM_21
#define TAG "CAN_RX"

void can_init(void) {
    // FIX: Changed from TWAI_MODE_NO_ACK to TWAI_MODE_NORMAL
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install and start, always checking for errors
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "Driver installed");
    }
    if (twai_start() == ESP_OK) {
        ESP_LOGI(TAG, "Driver started");
    }
}

// Helper to check for Bus-Off state and recover
void can_recover_if_needed(void) {
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
        if (status.state == TWAI_STATE_BUS_OFF) {
            ESP_LOGE(TAG, "Bus Off detected! Recovering...");
            twai_initiate_recovery();
            // FIX: Wait for bus to stabilize (128 occurrences of 11 recessive bits)
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
        if (status.state == TWAI_STATE_STOPPED) {
             ESP_LOGI(TAG, "Restarting Driver...");
             twai_start();
        }
    }
}


bool can_update_state(car_state_t *state) {
    twai_message_t msg;
    bool updated = false;

    // Check Health First
    can_recover_if_needed();

    // Process all incoming messages
    while (twai_receive(&msg, 0) == ESP_OK) {
        updated = true;
        
        switch (msg.identifier) {
            case ID_RPM: // 0x304
                state->rpm = msg.data[0] | (msg.data[1] << 8);
                break;

            case ID_SPEED:
                if (msg.data_length_code >= 2) {
                    state->speed = (uint16_t)(msg.data[0] | (msg.data[1] << 8));
                }
                break;

            case ID_CVT_TEMP: // 0x401
            {
                // 1. Reconstrói o inteiro de 16 bits (Little Endian)
                int16_t raw_cvt_temp = (int16_t)(msg.data[0] | (msg.data[1] << 8));

                // 2. Divide por 100 para voltar a graus Celsius e converte para o tamanho da variável do state
                // (Se a temperatura puder ficar negativa no painel, considere mudar o tipo no car_state_t para int8_t)
                state->cvt_temp = (uint8_t)(raw_cvt_temp / 100);
            }
            break;

            case ID_VOLTAGE: // 0x502
            {
                uint16_t raw_voltage = 0;

                // Reconstrói o inteiro de 16 bits usando os 2 bytes recebidos (Little Endian)
                raw_voltage = (uint16_t)(msg.data[0] | (msg.data[1] << 8));

                // Converte de volta para float dividindo pelo fator de escala (100)
                state->voltage = (float)raw_voltage / 100.0f;
            }
            break;

            case ID_FUEL: // 0x500
                state->fuel = msg.data[0] | (msg.data[1] << 8);
                break;

            case ID_ANGLE: // 0x205
                state->roll  = (int16_t)(msg.data[0] | (msg.data[1] << 8));
                state->pitch = (int16_t)(msg.data[2] | (msg.data[3] << 8));
                break;

            case ID_ENG_TEMP: // 0x400
                {
                    // 1. Reconstrói o inteiro de 16 bits enviado pela ECU
                    int16_t raw_temp = (int16_t)(msg.data[0] | (msg.data[1] << 8));

                    // 2. Divide por 100 e faz o cast para uint8_t (ex: 8534 / 100 = 85)
                    state->eng_temp = (uint8_t)(raw_temp / 100);
                }
                break;

            case ID_DEBUG:
                if (msg.data_length_code == 5) {
                    // Save to the state struct instead of a local variable
                    state->debug.current_state = msg.data[0];
                    state->debug.IMU_ok = msg.data[1] ? true : false;
                    state->debug.CAN_ok = msg.data[2] ? true : false;
                    state->debug.RPM_warning = msg.data[3] ? true : false;
                    state->debug.boot_tries = msg.data[4];
                }
                break;
        }
    }
    return updated;
}
