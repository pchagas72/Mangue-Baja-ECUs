#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

// Alter the source file to change pins
#include "sd_logging.h"

static const char *TAG = "Storage Control Unit";

void app_main(void) {

    ESP_LOGI(TAG, "Iniciando app_main");

    initialize_sd(TAG);

    while (1) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

