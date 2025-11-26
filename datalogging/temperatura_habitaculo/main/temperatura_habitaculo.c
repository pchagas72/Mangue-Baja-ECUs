#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht.h"
#include "sd_logging.h"
#include "sensors.h"

static const char *TAG = "LOGGER";

// DHT definitions
#define DHT_GPIO_PIN 4
#define SENSOR_TYPE DHT_TYPE_DHT11 

void app_main(void) {
   
    initialize_sd(TAG);

    while (1) {
        read_dht_data(TAG, SENSOR_TYPE, DHT_GPIO_PIN);

        // DHT11 can't handle reading < 2s
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}
