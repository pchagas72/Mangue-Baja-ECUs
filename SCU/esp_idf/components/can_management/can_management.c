#include "include/can_management.h"
#include "driver/twai.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

void initialize_CAN(const char *TAG) {
    ESP_LOGI(TAG, "Initializing CAN (TWAI)...");

    // Configuration structures using macros

    // General config: TX pin, RX pin, Mode (Normal)
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);
    
    // Timing config: Set baud rate to 500 kbps (Common standard)
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    
    // Filter config: Accept all messages (No filtering)
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "TWAI Driver installed");
    } else {
        ESP_LOGE(TAG, "Failed to install TWAI driver");
        return;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        ESP_LOGI(TAG, "TWAI Driver started");
    } else {
        ESP_LOGE(TAG, "Failed to start TWAI driver");
    }
}

int read_packet(const char *TAG, mqtt_packet_t *out_packet) {
    twai_message_t message;

    // Wait for message
    esp_err_t ret = twai_receive(&message, pdMS_TO_TICKS(100));

    if (ret == ESP_OK) {
        // Message received
        if (!(message.flags & TWAI_MSG_FLAG_RTR)) {
            // Debug
            ESP_LOGI(TAG, "Message received! ID: 0x%lx DLC: %d", message.identifier, message.data_length_code);

            switch (message.identifier) {
                case RPM_ID:
                    // Copy the two first bytes to the uint16_t defined in mqtt_packet_t
                    if (message.data_length_code >= 2) {
                        memcpy(&out_packet->rpm, message.data, 2);
                    }
                    break;

                case VOLTAGE_ID:
                    // Copy the four first bytes to the float defined in mqtt_packet_t
                    if (message.data_length_code >= 4) {
                        memcpy(&out_packet->volt, message.data, 4);
                    }
                    break;

                case SOC_ID:
                    // 1 byte
                    if (message.data_length_code >= 1) {
                        out_packet->SOC = message.data[0];
                    }
                    break;

                case CVT_ID:
                    // 1 byte
                    if (message.data_length_code >= 1) {
                        out_packet->cvt = message.data[0];
                    }
                    break;

                case CURRENT_ID:
                    // Copy four bytes to the float defined in mqtt_packet_t
                    if (message.data_length_code >= 4) {
                        memcpy(&out_packet->current, message.data, 4);
                    }
                    break;

                case TEMPERATURE_ID:
                    // 1 byte
                    if (message.data_length_code >= 1) {
                        out_packet->temperature = message.data[0];
                    }
                    break;

                case SPEED_ID:
                    // Copy two bytes to the uint16_t defined in mqtt_packet_t
                    if (message.data_length_code >= 2) {
                        memcpy(&out_packet->speed, message.data, 2);
                    }
                    break;

                case IMU_ACC_ID:
                    // Copy 6 bytes to the struct imu_acc_t defined in mqtt_packet_t
                    if (message.data_length_code >= 6) {
                        memcpy(&out_packet->imu_acc, message.data, 6);
                    }
                    break;

                case IMU_DPS_ID:
                    // Copy 6 bytes to the struct imu_dps_t defined in mqtt_packet_t
                    if (message.data_length_code >= 6) {
                        memcpy(&out_packet->imu_dps, message.data, 6);
                    }
                    break;

                case ANGLE_ID:
                    // Copy four bytes to the angle_t struct defined in mqtt_packet_t
                    if (message.data_length_code >= 4) {
                        memcpy(&out_packet->Angle, message.data, 4);
                    }
                    break;

                case LAT_ID:
                    // Copy eight bytes to the double defined in mqtt_packet_t
                    if (message.data_length_code >= 8) {
                        memcpy(&out_packet->latitude, message.data, 8);
                    }
                    break;

                case LNG_ID:
                    // Copy eight bytes to the double defined in mqtt_packet_t
                    if (message.data_length_code >= 8) {
                        memcpy(&out_packet->longitude, message.data, 8);
                    }
                    break;

                default:
                    // Unknown ID
                    break;
            }
        }
        return 0; // Success
    } else if (ret == ESP_ERR_TIMEOUT) {
        // No messages arrived
        return 1;
    } else {
        ESP_LOGE(TAG, "Failed to receive message: %s", esp_err_to_name(ret));
        return -1; // Error
    }
}
