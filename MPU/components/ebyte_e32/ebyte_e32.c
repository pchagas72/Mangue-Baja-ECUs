#include "ebyte_e32.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "EBYTE_TX";

void e32_init(void) {
    ESP_LOGI(TAG, "Initializing LoRa Hardware Pins...");
    
    // M0 and M1 Output
    gpio_reset_pin(E32_M0_PIN);
    gpio_set_direction(E32_M0_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(E32_M1_PIN);
    gpio_set_direction(E32_M1_PIN, GPIO_MODE_OUTPUT);

    // Force Transparent Mode (M0 = 0, M1 = 0)
    gpio_set_level(E32_M0_PIN, 0);
    gpio_set_level(E32_M1_PIN, 0);

    // AUX Input
    gpio_reset_pin(E32_AUX_PIN);
    gpio_set_direction(E32_AUX_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(E32_AUX_PIN, GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG, "Configuring LoRa UART2 at 9600 Baud...");
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    uart_driver_install(E32_UART_NUM, 1024, 0, 0, NULL, 0);
    uart_param_config(E32_UART_NUM, &uart_config);
    uart_set_pin(E32_UART_NUM, E32_TX_PIN, E32_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "LoRa UART Ready.");
}

void e32_send_struct(void *data, size_t size) {
    // Block until the E32 module finishes internal processing
    while (gpio_get_level(E32_AUX_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    // Send standard start marker to trigger the PlatformIO receiver logic
    const uint8_t start_marker[] = {0xAA, 0xBB, 0xCC, 0xDD};
    uart_write_bytes(E32_UART_NUM, (const char*)start_marker, sizeof(start_marker));
    
    // Write struct directly to UART
    uart_write_bytes(E32_UART_NUM, (const char*)data, size);
    
    ESP_LOGI(TAG, "Broadcasted %d bytes to Receiver", size);
}

/*
void e32_send_string(const char *str) {
    // Wait for module to be ready
    while (gpio_get_level(E32_AUX_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    // Write raw characters directly to UART
    uart_write_bytes(E32_UART_NUM, (const char*)str, strlen(str));
    ESP_LOGI(TAG, "Sent string: %s", str);
}
*/
