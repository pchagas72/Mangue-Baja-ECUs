#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "can_management.h"
#include "sd_logging.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    car_state_t car_state = {0};

    // Initializing TWAI driver 
    // TX GPIO 05 
    // RX GPIO 18
    can_init();

    // Initializing SD Card Logging
    // SPI3: MOSI:13, MISO:26, CLK:14, CS:15
    if (sd_logging_init() != ESP_OK) {
        ESP_LOGE(TAG, "SD Logging initialization failed.");
    }

    ESP_LOGI(TAG, "CAN Reader System Started. Monitoring bus...");

    while (1) {
        // can_update_state returns true if any new data was processed
        if (can_update_state(&car_state)) {
            
            // Print the received data to the console
            ESP_LOGI(TAG, "-------------------------");
            ESP_LOGI(TAG, "RPM: %u", car_state.rpm);
            ESP_LOGI(TAG, "Speed: %u KPH", car_state.speed);
            ESP_LOGI(TAG, "Fuel: %u%%", car_state.fuel);
            ESP_LOGI(TAG, "Voltage: %.2fV", car_state.voltage);
            ESP_LOGI(TAG, "CVT Temp: %u C | Eng Temp: %u C", car_state.cvt_temp, car_state.eng_temp);
            ESP_LOGI(TAG, "Roll: %d | Pitch: %d", car_state.roll, car_state.pitch);

            // Log the data to the SD card CSV file
            uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            sd_log_data(&car_state, now_ms);
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // Small delay to prevent starving the CPU and control print frequency
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
