#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht.h"
#include "led_controls.h"
#include "sd_logging.h"
#include "sensors.h"
#include <pthread.h>

static const char *TAG = "LOGGER";
pthread_mutex_t led_mutex = PTHREAD_MUTEX_INITIALIZER;

// DHT definitions
#define DHT_GPIO_PIN 4
#define SENSOR_TYPE DHT_TYPE_DHT11 

void app_main(void) {
   
    initialize_sd(TAG);
    int res;

    while (1) {
        res = read_dht_data(TAG, SENSOR_TYPE, DHT_GPIO_PIN, &led_mutex);
        if (res == 0) {
            // DHT11 can't handle reading < 2s
            vTaskDelay(pdMS_TO_TICKS(5000)); 
        } else if (res == 1){
            blink_led(5, 100, &led_mutex);
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }

    }
}
