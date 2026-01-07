#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "LSM6DS3_APP";

// --- Configurações do I2C e do Sensor ---
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_PORT_NUM                0
#define I2C_MASTER_FREQ_HZ          400000 // 400kHz funciona bem no LSM6DS3
#define LSM6DS3_ADDR                0x6A   // Confirmado pelo seu scanner

// --- Registradores do LSM6DS3 ---
#define LSM6DS3_WHO_AM_I            0x0F
#define LSM6DS3_CTRL1_XL            0x10   // Config Acelerômetro
#define LSM6DS3_CTRL2_G             0x11   // Config Giroscópio
#define LSM6DS3_CTRL3_C             0x12   // Config Controle (BDU, Auto-Inc)
#define LSM6DS3_OUTX_L_G            0x22   // Início dos dados do Giroscópio
#define LSM6DS3_OUTX_L_XL           0x28   // Início dos dados do Acelerômetro

// Handle global para o dispositivo
i2c_master_dev_handle_t lsm6ds3_handle;

// Função auxiliar para escrever em um registrador
esp_err_t write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_transmit(lsm6ds3_handle, write_buf, sizeof(write_buf), -1);
}

// Função auxiliar para ler múltiplos bytes (com auto-incremento)
esp_err_t read_regs(uint8_t reg_start, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(lsm6ds3_handle, &reg_start, 1, data, len, -1);
}

void app_main(void) {
    ESP_LOGI(TAG, "Inicializando driver para LSM6DS3...");

    // 1. Configuração do Barramento I2C
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    // 2. Adiciona o dispositivo LSM6DS3
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LSM6DS3_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &lsm6ds3_handle));

    // 3. Verifica o ID do Chip
    uint8_t who_am_i;
    read_regs(LSM6DS3_WHO_AM_I, &who_am_i, 1);
    ESP_LOGI(TAG, "WHO_AM_I lido: 0x%02X (Esperado: 0x69)", who_am_i);

    if (who_am_i != 0x69) {
        ESP_LOGE(TAG, "Chip ID incorreto! Verifique as conexões.");
        return;
    }

    // 4. Configuração do Sensor
    // CTRL3_C: BDU=1 (Block Data Update), IF_INC=1 (Auto-incremento de endereço)
    // 0x44 = 0100 0100
    write_reg(LSM6DS3_CTRL3_C, 0x44);

    // CTRL1_XL (Acelerômetro): ODR=416Hz, FS=2g
    // 0x60 = 0110 0000 (Bits 7-4 definem a frequência)
    write_reg(LSM6DS3_CTRL1_XL, 0x60);

    // CTRL2_G (Giroscópio): ODR=416Hz, FS=2000dps
    // 0x60 = 0110 0000
    write_reg(LSM6DS3_CTRL2_G, 0x60);

    ESP_LOGI(TAG, "Sensor configurado. Iniciando leitura...");
    vTaskDelay(100 / portTICK_PERIOD_MS); // Espera estabilizar

    while (1) {
        uint8_t raw_data[12];
        int16_t accel_x, accel_y, accel_z;
        int16_t gyro_x, gyro_y, gyro_z;

        // Leitura em Burst: Lê 6 bytes do Gyro (0x22) e continua para 6 bytes do Accel (0x28)
        // O registrador do Gyro vem antes do Accel na memória do LSM6DS3
        // Nota: Se quiser ler separado, leia 6 bytes de 0x22 e depois 6 de 0x28
        
        // Vamos ler o Giroscópio (0x22 a 0x27)
        read_regs(LSM6DS3_OUTX_L_G, raw_data, 6);
        gyro_x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
        gyro_y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
        gyro_z = (int16_t)((raw_data[5] << 8) | raw_data[4]);

        // Vamos ler o Acelerômetro (0x28 a 0x2D)
        read_regs(LSM6DS3_OUTX_L_XL, raw_data, 6);
        accel_x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
        accel_y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
        accel_z = (int16_t)((raw_data[5] << 8) | raw_data[4]);

        // Imprime os valores brutos (Raw)
        // Para converter para unidades reais, multiplique pela sensibilidade (veja datasheet)
        // Ex: 2g range -> 0.061 mg/LSB
        // Ex: 2000dps range -> 70 mdps/LSB
        
        printf("ACC:\tX:%d\tY:%d\tZ:%d\t|\tGYRO:\tX:%d\tY:%d\tZ:%d\n", 
               accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z);

        vTaskDelay(100 / portTICK_PERIOD_MS); // 10Hz de taxa de impressão
    }
}
