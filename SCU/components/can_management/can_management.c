#include "can_management.h"
#include "driver/twai.h"
#include "esp_log.h"
#include <string.h>

#define CAN_TX_PIN GPIO_NUM_22
#define CAN_RX_PIN GPIO_NUM_21
#define TAG "CAN_RX"

void can_init(void) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "Driver installed");
    }
    if (twai_start() == ESP_OK) {
        ESP_LOGI(TAG, "Driver started");
    }
}

void can_recover_if_needed(void) {
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
        if (status.state == TWAI_STATE_BUS_OFF) {
            ESP_LOGE(TAG, "Bus Off detected! Recovering...");
            twai_initiate_recovery();
        }
        if (status.state == TWAI_STATE_STOPPED) {
             ESP_LOGI(TAG, "Restarting Driver...");
             twai_start();
        }
    }
}

// Now takes a timeout parameter to replace the vTaskDelay in the main loop
bool can_update_state(car_state_t *state, uint32_t timeout_ticks) {
    twai_message_t msg;
    bool updated = false;

    can_recover_if_needed();

    // Block/wait here until a message arrives OR the timeout is hit
    if (twai_receive(&msg, timeout_ticks) == ESP_OK) {
        updated = true;
        
        // Process the first message, then quickly drain any others that arrived simultaneously
        do {
            switch (msg.identifier) {
                case ID_RPM: 
                    state->rpm = msg.data[0] | (msg.data[1] << 8);
                    break;
                case ID_SPEED: 
                    state->speed = msg.data[0] | (msg.data[1] << 8);
                    break;
                case ID_CVT_TEMP: 
                    state->cvt_temp = msg.data[0];
                    break;
                case ID_VOLTAGE: 
                    memcpy(&state->voltage, msg.data, 4);
                    break;
                case ID_FUEL: 
                    state->fuel = msg.data[0] | (msg.data[1] << 8);
                    break;
                case ID_ANGLE: 
                    state->roll  = (int16_t)(msg.data[0] | (msg.data[1] << 8));
                    state->pitch = (int16_t)(msg.data[2] | (msg.data[3] << 8));
                    break;
                case ID_ENG_TEMP: 
                    state->eng_temp = msg.data[0];
                    break;
            }
        } while (twai_receive(&msg, 0) == ESP_OK); // 0 ticks = instantly drain buffer
    }
    
    return updated;
}
