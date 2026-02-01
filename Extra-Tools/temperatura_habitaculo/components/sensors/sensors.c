#include "sensors.h"
//#include "esp_err.h"
#include "esp_log.h"
#include "sd_logging.h"
#include "dht.h"
#include "led_controls.h"

int read_dht_data(const char *TAG, dht_sensor_type_t sensor_type, int gpio_pin, pthread_mutex_t *led_mutex) {
    float temperature = 0;
    float humidity = 0;

    ESP_LOGI(TAG, "Read: Hum: %.1f%% Temp: %.1fC", humidity, temperature);
    write_DHT11_to_sd(TAG, temperature, humidity);
    blink_led(3, 100, led_mutex);
    return 0;
}
