#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"

// --- CONFIGURATION ---
#define TX_PIN GPIO_NUM_5
#define RX_PIN GPIO_NUM_4
#define TAG "SIM_HEALER"

// --- LEGACY IDs ---
#define ID_RPM          0x304
#define ID_SPEED        0x300
#define ID_ANGLE        0x205 
#define ID_CVT_TEMP     0x401
#define ID_ENG_TEMP     0x400
#define ID_VOLTAGE      0x502
#define ID_FUEL         0x500

void app_main(void)
{
    // 1. Install Driver (Standard Mode)
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_PIN, RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("CAN Driver Installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    if (twai_start() == ESP_OK) {
        printf("CAN Driver Started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }

    float time_counter = 0;

    while(1) {
        // --- BUS RECOVERY LOGIC ---
        twai_status_info_t status;
        twai_get_status_info(&status);

        if (status.state == TWAI_STATE_BUS_OFF) {
            ESP_LOGE(TAG, "BUS OFF DETECTED! - INITIATING RECOVERY...");
            twai_initiate_recovery(); 
            vTaskDelay(pdMS_TO_TICKS(100)); 
            twai_start(); 
            ESP_LOGI(TAG, "BUS RECOVERED!");
        }

        // 2. Prepare Data
        uint16_t rpm = (uint16_t)(3800 * fabs(sin(time_counter * 0.3)));
        uint16_t speed = rpm / 70; 
        int16_t roll = (int16_t)(200 * sin(time_counter * 0.8)); 
        int16_t pitch = (int16_t)(100 * cos(time_counter * 0.5)); 
        uint16_t fuel = 50 + (50 * sin(time_counter * 0.2)); 
        float volt = 12.0 + (0.5 * sin(time_counter * 0.2)); 
        uint8_t cvt_temp = 80 + (15 * sin(time_counter * 0.2)); 
        uint8_t eng_temp = 100 + (15 * sin(time_counter * 0.2)); 

        // 3. Send Messages
        twai_message_t msg;
        msg.extd = 0;
        msg.rtr = 0;
        
        // Use a consistent timeout to prevent buffer overflow when sending bursts
        TickType_t tx_timeout = pdMS_TO_TICKS(1);

        // RPM (0x304)
        msg.identifier = ID_RPM;
        msg.data_length_code = 2;
        memcpy(msg.data, &rpm, 2);
        twai_transmit(&msg, 0);

        // Speed (0x300)
        msg.identifier = ID_SPEED;
        msg.data_length_code = 2;
        memcpy(msg.data, &speed, 2);
        twai_transmit(&msg, 0);

        // Fuel (0x500)
        msg.identifier = ID_FUEL;
        msg.data_length_code = 2;
        memcpy(msg.data, &fuel, 2);
        twai_transmit(&msg, tx_timeout);
                                
        // Angles (0x205)
        msg.identifier = ID_ANGLE;
        msg.data_length_code = 4;
        memcpy(&msg.data[0], &roll, 2);
        memcpy(&msg.data[2], &pitch, 2);
        twai_transmit(&msg, 0);

        // Volt (0x502)
        msg.identifier = ID_VOLTAGE;
        msg.data_length_code = 4;
        memcpy(msg.data, &volt, 4);
        twai_transmit(&msg, 0);

        // CVT Temp (0x401)
        msg.identifier = ID_CVT_TEMP;
        msg.data_length_code = 1;
        memcpy(msg.data, &cvt_temp, 1);
        twai_transmit(&msg, tx_timeout);

        // ENG Temp (0x400) - Fixed ID assignment and removed redundant call
        msg.identifier = ID_ENG_TEMP;
        msg.data_length_code = 1;
        memcpy(msg.data, &eng_temp, 1);
        twai_transmit(&msg, 0);

        time_counter += 0.1;
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}
