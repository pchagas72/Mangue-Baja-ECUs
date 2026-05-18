#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/twai.h"  
#include "driver/gpio.h"  
#include "can_management.h"
#include "sd_logging.h"

static const char *TAG = "MAIN";

// ⚠️ UPDATE THIS: Map this to the internal GPIO matching your physical pad
#define RX_LED_GPIO GPIO_NUM_2 

void app_main(void)
{
    car_state_t car_state = {0};

    // 1. Configure the RX LED pin
    gpio_reset_pin(RX_LED_GPIO);
    gpio_set_direction(RX_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(RX_LED_GPIO, 0); // Start turned OFF

    // 2. Initializing TWAI (CAN) driver 
    can_init();

    // 3. Initializing SD Card Logging
    if (sd_logging_init() != ESP_OK) {
        ESP_LOGE(TAG, "SD Logging initialization failed.");
    }

    ESP_LOGI(TAG, "-----------------------------------");
    ESP_LOGI(TAG, "Starting FRONT ECU Telemetry...");
    ESP_LOGI(TAG, "-----------------------------------");

    uint32_t last_led_toggle_time = 0;
    uint8_t led_state = 0;

    while (1) {
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // can_update_state returns true if ANY new data was pulled from the TWAI RX buffer
        if (can_update_state(&car_state)) {
            
            // --- RX ACTIVITY LED LOGIC ---
            // Toggle the LED to show activity, limited to every 50ms so the flicker is highly visible
            if (now_ms - last_led_toggle_time > 50) {
                led_state = !led_state;
                gpio_set_level(RX_LED_GPIO, led_state);
                last_led_toggle_time = now_ms;
            }

            // Print the received data to the console
            ESP_LOGI(TAG, "-------------------------");
            ESP_LOGI(TAG, "RPM: %u", car_state.rpm);
            ESP_LOGI(TAG, "Speed: %u KPH", car_state.speed);
            ESP_LOGI(TAG, "Fuel: %u%%", car_state.fuel);
            ESP_LOGI(TAG, "Voltage: %.2fV", car_state.voltage);
            ESP_LOGI(TAG, "CVT Temp: %u C | Eng Temp: %u C", car_state.cvt_temp, car_state.eng_temp);
            ESP_LOGI(TAG, "Roll: %d | Pitch: %d", car_state.roll, car_state.pitch);

            // Log the data to the SD card CSV file
            sd_log_data(&car_state, now_ms);
            
        } else {
            // --- SILENCE TIMEOUT LOGIC ---
            // If the bus goes quiet for more than 200ms, ensure the LED turns off
            if (now_ms - last_led_toggle_time > 200 && led_state == 1) {
                led_state = 0;
                gpio_set_level(RX_LED_GPIO, led_state);
            }
        }

        // Small delay to prevent starving the CPU. 
        // Reduced to 50ms to make the LED blinking and data reading more responsive.
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
