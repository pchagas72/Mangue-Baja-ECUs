#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>
#include <esp_timer.h>
#include "driver/gpio.h" // Added for direct LED control

#include "can_management.h"
#include "esp_err.h"
#include "esp_event_base.h"
#include "portmacro.h"

#include "mqtt_client.h"
#include <esp_crt_bundle.h>
#include <pthread.h>
#include "sd_logging.h"
#include "sdkconfig.h"

#include "led_controls.h"
#include "server_controls.h"
#include "protocol.h"

#define TAG "SCU_MAIN"
#define TIMEOUT_MS 1500

// Use the struct properly
Server_State state = {0}; 
pthread_mutex_t led_mutex = PTHREAD_MUTEX_INITIALIZER;

char mqtt_response_topic[64];
char mqtt_broker_address[128];

typedef struct __attribute__((packed)) {
    float voltage;          // f (raw[0])
    uint8_t soc;            // B (raw[1])
    uint8_t cvt_temp;       // B (raw[2])
    float current;          // f (raw[3])
    uint8_t eng_temp;       // B (raw[4])
    uint16_t speed;         // H (raw[5])
    
    int16_t acc_x;          // h (raw[6])
    int16_t acc_y;          // h (raw[7])
    int16_t acc_z;          // h (raw[8])
    int16_t dps_x;          // h (raw[9])
    int16_t dps_y;          // h (raw[10])
    int16_t dps_z;          // h (raw[11])
    
    int16_t roll;           // h (raw[12])
    int16_t pitch;          // h (raw[13])
    uint16_t rpm;           // H (raw[14])
    uint8_t flags;          // B (raw[15])
    
    double latitude;        // d (raw[16])
    double longitude;       // d (raw[17])
    uint32_t timestamp;     // I (raw[18])
} telemetry_packet_t;


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected to HiveMQ Cloud");
            state.connected_to_mqtt = true; // Use state variable here
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected");
            state.connected_to_mqtt = false; // Use state variable here
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error type: 0x%x", event->error_handle->error_type);
            break;
        default:
            break;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Task starting up\n");

    snprintf(mqtt_response_topic, sizeof(mqtt_response_topic), "%s", CONFIG_SERVER_NAME);
    snprintf(mqtt_broker_address, sizeof(mqtt_broker_address), "%s://%s:%d",
            CONFIG_MQTT_URL_SCHEME,
            CONFIG_MQTT_IP,
            CONFIG_MQTT_PORT);

    // Initial setups
    server_connect_internet();
    server_check_connection_internet(&state, &led_mutex);

    esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = mqtt_broker_address,
            .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
            .credentials.username = CONFIG_MQTT_USER,      
            .credentials.authentication.password = CONFIG_MQTT_PASSWORD, 
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    
    blink_led(3, 500, &led_mutex);

    can_init();
    //sd_logging_init();

    car_state_t car = {0};
    telemetry_packet_t packet = {0};
    
    int64_t last_pkt_time = 0;
    int64_t led_off_timer = 0; // For non-blocking blinking
    
    // Ensure LED pin is ready
    gpio_set_direction(BLINK_LED, GPIO_MODE_OUTPUT);

    while (1)
    {
        int64_t now_us = esp_timer_get_time();
        int64_t now_ms = now_us / 1000;

        if (now_ms - last_pkt_time > TIMEOUT_MS) {
            car.link_active = false;
        }

        // Wait up to 50ms for a CAN packet (Eliminates the need for vTaskDelay!)
        if (can_update_state(&car, pdMS_TO_TICKS(500))) {
            last_pkt_time = now_ms;
            car.link_active = true;
            
            // Trigger Non-Blocking Blink (20ms flash)
            gpio_set_level(BLINK_LED, 1);
            led_off_timer = now_ms + 20;

            // Log to SD
            //sd_log_data(&car, (uint32_t)now_ms);

            // Prepare and send MQTT Packet only if connected
            if (state.connected_to_mqtt) {
                memset(&packet, 0, sizeof(telemetry_packet_t));

                packet.voltage = car.voltage;
                packet.soc = (uint8_t)car.fuel; // Cast applied
                packet.cvt_temp = car.cvt_temp;
                packet.current = 0.0f; 
                packet.eng_temp = car.eng_temp;
                packet.speed = car.speed;

                packet.roll = car.roll;
                packet.pitch = car.pitch;
                packet.rpm = car.rpm;
                packet.flags = (car.link_active ? 1 : 0) | (car.box_alert ? 2 : 0);
                packet.timestamp = (uint32_t)now_ms;

                esp_mqtt_client_publish(client, mqtt_response_topic, (const char*)&packet, sizeof(telemetry_packet_t), 0, 0);
            }
        }

        // Turn off LED if blink duration has passed
        if (led_off_timer > 0 && now_ms > led_off_timer) {
            gpio_set_level(BLINK_LED, 0);
            led_off_timer = 0;
        }
        
        // REMOVED vTaskDelay(100) because can_update_state() now handles the blocking timeout gracefully!
    }
}
