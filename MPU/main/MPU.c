#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

// Custom Components
#include "can_management.h"
#include "ebyte_e32.h"
#include "packets.h" 

static const char *TAG = "MPU";

#define LORA_TX_INTERVAL_MS 1000 // XHz rate

void app_main(void)
{

    // Initilize CAN driver
    can_init();
    
    // Initialize the EBYTE LoRa driver
    e32_init();
    
    // Separate packet for radio telemetry
    radio_packet_t radio_telemetry_packet;
    memset(&radio_telemetry_packet, 0, sizeof(radio_packet_t));

    while (1)
    {
        // Maintain CAN Bus Health
        can_recover_if_needed();

        // Check GPS here or create a separate task for it

        // Map CAN package to radio telemetry package based on ticks
        // For example: send GPS every time and send temperature every 5 seconds
        // Code example: radio_telemetry_packet.rpm = car_state.rpm;
        // radio_telemetry_packet.rpm = car_state.rpm;
        // TODO: Check SCU code.
        // Prepare Data
        if (gpio_get_level(GPIO_NUM_0) == 0 && radio_telemetry_packet.rpm <= 3800) {
            radio_telemetry_packet.rpm += (100 - radio_telemetry_packet.rpm/100)*3;
        } else if (radio_telemetry_packet.rpm >= 700 && gpio_get_level(GPIO_NUM_0) != 0){
            radio_telemetry_packet.rpm -= 100 + (radio_telemetry_packet.rpm/50)*3;
        }
        
        // Stamp the packet with the current uptime in milliseconds
        radio_telemetry_packet.timestamp = (uint32_t)(esp_timer_get_time() / 1000ULL);

        // Transmit the packed 51-byte struct over the air
        e32_send_struct(&radio_telemetry_packet, sizeof(radio_packet_t));
        
        // Yield CPU to maintain steady transmission rate
        vTaskDelay(pdMS_TO_TICKS(LORA_TX_INTERVAL_MS));
    }
}
