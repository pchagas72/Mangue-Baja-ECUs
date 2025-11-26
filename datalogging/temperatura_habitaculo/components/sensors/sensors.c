#include "sensors.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sd_logging.h"
#include "dht.h" // Ensure this is included

void read_dht_data(const char *TAG, dht_sensor_type_t sensor_type, int gpio_pin) {
    float temperature = 0;
    float humidity = 0;

    if (dht_read_float_data(sensor_type, gpio_pin, &humidity, &temperature) == ESP_OK) {
        ESP_LOGI(TAG, "Read: Hum: %.1f%% Temp: %.1fC", humidity, temperature);
        write_DHT11_to_sd(TAG, temperature, humidity);
    } else {
        ESP_LOGE(TAG, "Could not read data from sensor");
    }
}
