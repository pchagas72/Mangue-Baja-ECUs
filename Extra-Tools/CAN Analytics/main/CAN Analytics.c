#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/twai.h"  
#include "driver/gpio.h"  
#include "can_management.h"
#include "sd_logging.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

static const char *TAG = "MAIN";

// ⚠️ UPDATE THIS: Map this to the internal GPIO matching your physical pad
#define RX_LED_GPIO GPIO_NUM_2 

// Force the compiler to pack the struct exactly as Python's struct.unpack expects
#pragma pack(push, 1)
typedef struct {
    float volt;          // f
    uint8_t soc;         // B
    uint8_t temp_cvt;    // B
    float current;       // f
    uint8_t temperature; // B
    uint8_t speed;       // B
    int16_t acc_x;       // h
    int16_t acc_y;       // h
    int16_t acc_z;       // h
    int16_t dps_x;       // h
    int16_t dps_y;       // h
    int16_t dps_z;       // h
    int16_t roll;        // h
    int16_t pitch;       // h
    uint16_t rpm;        // H
    uint8_t flags;       // B
    double latitude;     // d
    double longitude;    // d
    uint32_t timestamp;  // I
} telemetry_packet_t;
#pragma pack(pop)

void send_binary_telemetry(car_state_t *state, uint32_t now_ms) {
    telemetry_packet_t packet;
    memset(&packet, 0, sizeof(telemetry_packet_t));

    // Map your CAN data into the Python backend structure
    packet.volt = state->voltage;
    packet.soc = (uint8_t)state->fuel;     // Using Fuel as SOC for now
    packet.temp_cvt = state->cvt_temp;
    packet.current = 0.0f;                 // No current sensor mapped yet
    packet.temperature = state->eng_temp;
    packet.speed = (uint8_t)state->speed;  // Casting uint16_t to uint8_t 
    
    // Fill unused IMU/GPS slots with 0 to maintain packet size
    packet.acc_x = 0; packet.acc_y = 0; packet.acc_z = 0;
    packet.dps_x = 0; packet.dps_y = 0; packet.dps_z = 0;
    packet.latitude = 0.0; packet.longitude = 0.0;
    
    packet.roll = state->roll;
    packet.pitch = state->pitch;
    packet.rpm = state->rpm;
    
    // Store debug state in the flags byte
    packet.flags = state->debug.current_state; 
    packet.timestamp = now_ms;

    // 1. Send the Start Marker exactly as expected by LoRa.py
    const uint8_t marker[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    fwrite(marker, 1, 4, stdout);
    
    // 2. Send the 51-byte payload
    fwrite(&packet, 1, sizeof(packet), stdout);
    
    // Push the buffer immediately to the serial port
    fflush(stdout);
}

void app_main(void)
{
    car_state_t car_state = {0};

    gpio_reset_pin(RX_LED_GPIO);
    gpio_set_direction(RX_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(RX_LED_GPIO, 0); 

    can_init();

    if (sd_logging_init() != ESP_OK) {
        ESP_LOGE(TAG, "SD Logging initialization failed.");
    }

    // Keep startup logs as they execute before the Python server connects
    ESP_LOGI(TAG, "-----------------------------------");
    ESP_LOGI(TAG, "Starting FRONT ECU Telemetry...");
    ESP_LOGI(TAG, "Broadcasting binary data to stdout");
    ESP_LOGI(TAG, "-----------------------------------");

    uint32_t last_led_toggle_time = 0;
    uint8_t led_state = 0;

    esp_vfs_dev_uart_port_set_tx_line_endings(UART_NUM_0, ESP_LINE_ENDINGS_LF);

    while (1) {
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        if (can_update_state(&car_state)) {
            
            if (now_ms - last_led_toggle_time > 50) {
                led_state = !led_state;
                gpio_set_level(RX_LED_GPIO, led_state);
                last_led_toggle_time = now_ms;
            }

            // Fire the binary packet over UART
            send_binary_telemetry(&car_state, now_ms);
            
            sd_log_data(&car_state, now_ms);
            
        } else {
            if (now_ms - last_led_toggle_time > 200 && led_state == 1) {
                led_state = 0;
                gpio_set_level(RX_LED_GPIO, led_state);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
