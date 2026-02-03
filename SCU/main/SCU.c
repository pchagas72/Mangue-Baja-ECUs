#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <string.h>
#include <esp_timer.h>

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

Server_State state;
pthread_mutex_t led_mutex = PTHREAD_MUTEX_INITIALIZER;

char mqtt_response_topic[64];
char mqtt_broker_address[128];
bool mqtt_connected = false;

typedef struct __attribute__((packed)) {
    float voltage;          // f (raw[0])
    uint8_t soc;            // B (raw[1])
    uint8_t cvt_temp;       // B (raw[2])
    float current;          // f (raw[3])
    uint8_t eng_temp;       // B (raw[4])
    uint16_t speed;         // H (raw[5])
    
    // Accel (raw shorts)
    int16_t acc_x;          // h (raw[6])
    int16_t acc_y;          // h (raw[7])
    int16_t acc_z;          // h (raw[8])
    
    // Gyro (raw shorts)
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
            mqtt_connected = true;
            // Subscribe to the command topic
            //esp_mqtt_client_subscribe(event->client, mqtt_receive_topic, 1);
            break;
        case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT Disconnected");
        mqtt_connected = false;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT Error type: 0x%x", event->error_handle->error_type);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG, "Last TLS error: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGE(TAG, "TLS stack error: 0x%x", event->error_handle->esp_tls_stack_err);
        }
        break;
    default:
        break;
    }
}

void app_main(void)
{
    char *taskName = pcTaskGetName(NULL);
    ESP_LOGI(taskName, "Task starting up\n");

    // Read topics and server IP from the config
    snprintf(mqtt_response_topic, sizeof(mqtt_response_topic), "%s", CONFIG_SERVER_NAME);
    snprintf(mqtt_broker_address, sizeof(mqtt_broker_address), "%s://%s:%d",
            CONFIG_MQTT_URL_SCHEME,
            CONFIG_MQTT_IP,
            CONFIG_MQTT_PORT);

    // Start internet connection
    server_connect_internet();

    // Gather info about internet connection
    server_check_connection_internet(&state, &led_mutex);

    esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = mqtt_broker_address,
            .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
            .credentials.username = CONFIG_MQTT_USER,      // Add Username
            .credentials.authentication.password = CONFIG_MQTT_PASSWORD, // Add Password
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
    blink_led(3, 500, &led_mutex);

    // Start CAN network
    can_init();
    sd_logging_init();

    car_state_t car = {0};
    telemetry_packet_t packet = {0};
    int64_t last_pkt_time = 0;

    // Any side processes can be executed here
    while (1)
    {
        int64_t now_us = esp_timer_get_time();
        int64_t now_ms = now_us / 1000;

        if (now_ms - last_pkt_time > TIMEOUT_MS) {
            car.link_active = false;
        }

        // 1. Update CAN Data
        if (can_update_state(&car)) {
            last_pkt_time = now_ms;
            car.link_active = true;
            sd_log_data(&car, now_ms);

            // 2. Prepare MQTT Packet
            memset(&packet, 0, sizeof(telemetry_packet_t));

            packet.voltage = car.voltage;
            packet.soc = car.fuel;  // Mapping fuel to SOC slot based on your data structure
            packet.cvt_temp = car.cvt_temp;
            packet.current = 0.0f; 
            packet.eng_temp = car.eng_temp;
            packet.speed = car.speed;

            // Accelerometer & Gyro (Zeros until you add the sensor driver)
            packet.acc_x = 0; 
            packet.acc_y = 0;
            packet.acc_z = 0;
            packet.dps_x = 0;
            packet.dps_y = 0;
            packet.dps_z = 0;

            packet.roll = car.roll;
            packet.pitch = car.pitch;
            packet.rpm = car.rpm;
            packet.flags = (car.link_active ? 1 : 0) | (car.box_alert ? 2 : 0);

            packet.latitude = 0.0;
            packet.longitude = 0.0;
            packet.timestamp = (uint32_t)now_ms;

            if (client) {
                esp_mqtt_client_publish(client, mqtt_response_topic, (const char*)&packet, sizeof(telemetry_packet_t), 0, 0);

            }
        }

        /*
        if (!state.connected_to_mqtt || !state.connected_to_internet) {
            blink_led(5, 50, &led_mutex);
        }
        */

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
