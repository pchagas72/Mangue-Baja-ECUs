#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Componentes
#include "sd_logging.h"
#include "test_modem.h"
#include "can_management.h" 

static const char *TAG = "MAIN_TEST";

void app_main(void) {
    ESP_LOGI(TAG, "--- Starting Legacy Code Migration Test ---");

    // 1. Inicializa SD
    initialize_sd(TAG);

    // 2. Inicializa Modem UART
    initialize_modem(TAG);

    // 3. Configuração Inicial da Conexão (Manual sequence for testing)
    ESP_LOGI(TAG, "Setting up GPRS connection...");
    send_at_command(TAG, "AT");
    vTaskDelay(pdMS_TO_TICKS(1000));
    send_at_command(TAG, "AT+CPIN?"); // Verifica SIM
    send_at_command(TAG, "AT+CREG?"); // Verifica Registro na Rede
    send_at_command(TAG, "AT+CGATT=1"); // Anexa ao GPRS
    
    // Configura APN (Ajuste para sua operadora, ex: timbrasil.br)
    send_at_command(TAG, "AT+CSTT=\"zap.vivo.com.br\",\"vivo\",\"vivo\"");
    send_at_command(TAG, "AT+CIICR"); // Traz a conexão wireless
    send_at_command(TAG, "AT+CIFSR"); // Pega o IP Local

    // Conecta no Broker MQTT (TCP)
    // Substitua pelo IP do seu broker e porta
    send_at_command(TAG, "AT+CIPSTART=\"TCP\",\"69.55.61.114\",\"1883\"");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Espera conectar

    // Estrutura de dados para o teste
    can_packet current_packet;

    while (1) {
        ESP_LOGI(TAG, "--- Cycle Start ---");

        // A. Obter Dados (MOCK / FIXO)
        get_fixed_packet(&current_packet);

        // B. Gravar no SD
        if (write_packet_to_sd(TAG, &current_packet) == 0) {
            ESP_LOGI(TAG, "SD Save: OK");
        } else {
            ESP_LOGE(TAG, "SD Save: FAIL");
        }

        // C. Enviar via Modem (MQTT Payload)
        // Isso vai mandar o pacote binário via TCP socket aberto acima
        mqtt_publish_fixed(TAG, &current_packet);

        ESP_LOGI(TAG, "Data sent via Modem.");

        // Loop de 1Hz (1000ms)
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}
