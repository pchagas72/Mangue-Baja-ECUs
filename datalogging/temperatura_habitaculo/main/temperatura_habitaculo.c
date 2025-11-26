#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "dht.h"
#include "sd_logging.h"

static const char *TAG = "LOGGER";

// DHT definitions
#define DHT_GPIO_PIN 4
#define SENSOR_TYPE DHT_TYPE_DHT11 

void app_main(void) {
   
    // Initializes sd card
    initialize_sd(TAG);

    float temperature = 0;
    float humidity = 0;

    while (1) {
        if (dht_read_float_data(SENSOR_TYPE, DHT_GPIO_PIN, &humidity, &temperature) == ESP_OK) {
            ESP_LOGI(TAG, "Read: Hum: %.1f%% Temp: %.1fC", humidity, temperature);
            write_DHT11_to_sd(TAG, temperature, humidity);
        } else {
            ESP_LOGE(TAG, "Could not read data from sensor");
        }

        // DHT11 can't handle reading < 2s
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}
