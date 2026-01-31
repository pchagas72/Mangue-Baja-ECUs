#include "test_modem.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "can_management.h"

void initialize_modem(const char *TAG) {
    ESP_LOGI(TAG, "Initializing Modem UART...");

    // 1. Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = MODEM_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    // 2. Install UART driver
    // We allocate a buffer for RX, but 0 for TX (blocking send)
    ESP_ERROR_CHECK(uart_driver_install(MODEM_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MODEM_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(MODEM_UART_NUM, MODEM_TX_PIN, MODEM_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 3. Hardware Reset / Power On Sequence
    // Most SIM800 modules need a pulse on the RST/PWRKEY pin to wake up
    ESP_LOGI(TAG, "Performing Hardware Reset/Power-on...");
    gpio_reset_pin(MODEM_RST_PIN);
    gpio_set_direction(MODEM_RST_PIN, GPIO_MODE_OUTPUT);
    
    // Wait for the modem to fully boot (can take 3-5 seconds)
    ESP_LOGI(TAG, "Waiting for Modem Boot...");
    vTaskDelay(pdMS_TO_TICKS(4000));
}

int send_at_command(const char *TAG, const char *command) {
    // 1. Flush the buffer to remove old data
    uart_flush_input(MODEM_UART_NUM);

    // 2. Add carriage return/newline if missing
    char cmd_buffer[64];
    snprintf(cmd_buffer, sizeof(cmd_buffer), "%s\r\n", command);

    // 3. Send Command
    ESP_LOGI(TAG, "Sending: %s", command);
    uart_write_bytes(MODEM_UART_NUM, cmd_buffer, strlen(cmd_buffer));

    // 4. Read Response
    uint8_t data[BUF_SIZE];
    int len = uart_read_bytes(MODEM_UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(2000)); // 2s timeout

    if (len > 0) {
        data[len] = '\0';
        ESP_LOGI(TAG, "Response:\n%s", (char *)data);
        
        // Simple check for "OK"
        if (strstr((char *)data, "OK") != NULL) {
            return 0; // Success
        }
    } else {
        ESP_LOGE(TAG, "No response received (Timeout)");
        return -1; // Timeout
    }
    return 1; // Received something, but not OK
}

// Função auxiliar para enviar dados binários brutos (necessário para MQTT payload se for binário)
int send_raw_data(const char *TAG, uint8_t *data, int len) {
    uart_write_bytes(MODEM_UART_NUM, (const char*)data, len);
    return 0;
}

// Função para publicar o pacote no tópico MQTT via comandos AT
// Nota: Assume que o modem já está conectado na internet e no broker (AT+CIPSTART feito no main)
void mqtt_publish_fixed(const char *TAG, can_packet *pkt) {
    
    // 1. Prepara o payload (exatamente como no código legado PlatformIO)
    // Se o servidor espera binário, enviamos a struct crua.
    uint8_t buffer[sizeof(can_packet)];
    memcpy(buffer, pkt, sizeof(can_packet));

    // 2. Comando AT para enviar dados (AT+CIPSEND)
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d", sizeof(can_packet));
    send_at_command(TAG, cmd); // Envia o comando e espera o ">" ou OK
    
    // Pequeno delay para o modem preparar o buffer
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Envia os dados binários
    ESP_LOGI(TAG, "Sending Binary MQTT Payload...");
    send_raw_data(TAG, buffer, sizeof(can_packet));

    // 4. (Opcional para SIM800 em modo não-transparente) Enviar CTRL+Z (0x1A)
    // Se estiver usando AT+CIPSEND com tamanho fixo, muitas vezes não precisa do CTRL+Z,
    // mas se o modem travar esperando, descomente abaixo:
    // uint8_t ctrl_z = 0x1A;
    // send_raw_data(TAG, &ctrl_z, 1);
}
