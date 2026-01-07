#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

// Inclui o driver da Bosch
#include "bmi160.h"

static const char *TAG = "BMI160_SPI";

// --- Configurações dos Pinos SPI (VSPI Padrão) ---
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// Handle global para o dispositivo SPI
// Necessário porque dev->id é apenas uint8_t e não cabe o ponteiro do handle
spi_device_handle_t spi_bmi160;

// ============================================================================
// Funções de Interface (Bridge) SPI para o Driver Bosch
// ============================================================================

// O BMI160 espera que o primeiro byte seja o endereço do registrador.
// O driver da Bosch (bmi160.c) já aplica a máscara de Leitura (0x80) ou Escrita (0x7F) no reg_addr.

int8_t user_spi_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    esp_err_t ret;
    spi_transaction_t t;
    
    if (len == 0) return BMI160_OK;

    // Prepara os buffers
    // Precisamos enviar 1 byte (endereço) e receber 'len' bytes.
    // No modo Full-Duplex, o ESP32 envia e recebe simultaneamente.
    // Enviaremos [ADDR] + [ZEROS...]
    // Receberemos [LIXO] + [DADOS...]
    
    uint8_t *tx_buffer = heap_caps_malloc(len + 1, MALLOC_CAP_DMA);
    uint8_t *rx_buffer = heap_caps_malloc(len + 1, MALLOC_CAP_DMA);
    
    if (!tx_buffer || !rx_buffer) return BMI160_E_COM_FAIL;

    memset(tx_buffer, 0, len + 1);
    memset(rx_buffer, 0, len + 1);

    tx_buffer[0] = reg_addr; // O driver já configurou o bit de leitura (MSB = 1)

    memset(&t, 0, sizeof(t));
    t.length = 8 * (len + 1); // Total de bits (1 byte cmd + dados)
    t.tx_buffer = tx_buffer;
    t.rx_buffer = rx_buffer;

    ret = spi_device_transmit(spi_bmi160, &t);

    if (ret == ESP_OK) {
        // Copia os dados recebidos (pulando o primeiro byte que é resposta do comando)
        memcpy(data, &rx_buffer[1], len);
    }

    free(tx_buffer);
    free(rx_buffer);

    return (ret == ESP_OK) ? BMI160_OK : BMI160_E_COM_FAIL;
}

int8_t user_spi_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    esp_err_t ret;
    spi_transaction_t t;

    if (len == 0) return BMI160_OK;

    // Buffer de transmissão: [Endereço] + [Dados]
    uint8_t *tx_buffer = heap_caps_malloc(len + 1, MALLOC_CAP_DMA);
    if (!tx_buffer) return BMI160_E_COM_FAIL;

    tx_buffer[0] = reg_addr; // O driver já configurou o bit de escrita (MSB = 0)
    memcpy(&tx_buffer[1], data, len);

    memset(&t, 0, sizeof(t));
    t.length = 8 * (len + 1);
    t.tx_buffer = tx_buffer;
    t.rx_buffer = NULL; // Não precisamos ler nada na escrita

    ret = spi_device_transmit(spi_bmi160, &t);

    free(tx_buffer);

    return (ret == ESP_OK) ? BMI160_OK : BMI160_E_COM_FAIL;
}

void user_delay_ms(uint32_t period) {
    // Delay em milissegundos
    // vTaskDelay requer TickType_t, conversão necessária
    uint32_t ticks = period / portTICK_PERIOD_MS;
    if (ticks == 0) ticks = 1; // Garante pelo menos 1 tick de espera
    vTaskDelay(ticks);
}

// ============================================================================
// Inicialização do SPI
// ============================================================================
static void spi_init(void) {
    esp_err_t ret;

    // Configuração do Barramento SPI
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 128, // Tamanho máximo de transferência em bytes
    };

    // Configuração do Dispositivo no Barramento
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 5 * 1000 * 1000, // Clock de 5 MHz
        .mode = 0,                         // SPI Mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = PIN_NUM_CS,        // Pino CS
        .queue_size = 7,                   // Tamanho da fila de transações
    };

    // Inicializa o barramento SPI2 (HSPI) ou SPI3 (VSPI)
    // Usando SPI2_HOST (ou HSPI_HOST em versões antigas do IDF)
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    // Adiciona o dispositivo ao barramento
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_bmi160);
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "SPI Inicializado com sucesso");
}

// ============================================================================
// Aplicação Principal
// ============================================================================
void app_main(void) {
    // 1. Inicializa SPI
    spi_init();

    struct bmi160_dev sensor;

    // 2. Configura a estrutura do sensor
    sensor.id = 0;  // ID não é usado na nossa implementação de wrapper SPI (usamos handle global)
    sensor.intf = BMI160_SPI_INTF; // Define interface como SPI
    sensor.read = user_spi_read;
    sensor.write = user_spi_write;
    sensor.delay_ms = user_delay_ms;

    // 3. Inicializa o Sensor
    int8_t rslt = bmi160_init(&sensor);
    
    if (rslt == BMI160_OK) {
        ESP_LOGI(TAG, "BMI160 Inicializado (SPI)! Chip ID: 0x%X", sensor.chip_id);
    } else {
        ESP_LOGE(TAG, "Falha na inicialização do BMI160 (SPI). Codigo: %d", rslt);
        // Tente verificar as conexões se falhar aqui
        return;
    }

    // 4. Configura Acelerômetro e Giroscópio
    sensor.accel_cfg.odr = BMI160_ACCEL_ODR_100HZ;
    sensor.accel_cfg.range = BMI160_ACCEL_RANGE_2G;
    sensor.accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;
    sensor.accel_cfg.power = BMI160_ACCEL_NORMAL_MODE;

    sensor.gyro_cfg.odr = BMI160_GYRO_ODR_100HZ;
    sensor.gyro_cfg.range = BMI160_GYRO_RANGE_250_DPS;
    sensor.gyro_cfg.bw = BMI160_GYRO_BW_NORMAL_MODE;
    sensor.gyro_cfg.power = BMI160_GYRO_NORMAL_MODE;

    rslt = bmi160_set_sens_conf(&sensor);
    if (rslt != BMI160_OK) {
        ESP_LOGE(TAG, "Falha ao configurar sensores. Codigo: %d", rslt);
        return;
    }

    ESP_LOGI(TAG, "Sensores configurados. Iniciando leitura...");

    // 5. Loop de leitura
    struct bmi160_sensor_data accel;
    struct bmi160_sensor_data gyro;

    while (1) {
        rslt = bmi160_get_sensor_data((BMI160_ACCEL_SEL | BMI160_GYRO_SEL), &accel, &gyro, &sensor);

        if (rslt == BMI160_OK) {
            ESP_LOGI(TAG, "ACCEL [X:%6d Y:%6d Z:%6d] | GYRO [X:%6d Y:%6d Z:%6d]", 
                     accel.x, accel.y, accel.z, 
                     gyro.x, gyro.y, gyro.z);
        } else {
            ESP_LOGE(TAG, "Erro na leitura de dados");
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
