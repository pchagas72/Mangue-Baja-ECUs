#include "server_controls.h"
#include "led_controls.h"

void server_connect_internet(){
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
}

void server_check_connection_internet(Server_State *state, pthread_mutex_t *led_mutex){
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK){
        ESP_LOGI("WiFi", "Connected to %s\n", ap_info.ssid);
        state->connected_to_internet = true;
        blink_led(3, 100, led_mutex);
    }
}
