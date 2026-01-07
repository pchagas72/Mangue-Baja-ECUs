#include "sensors.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sd_logging.h"
#include "dht.h"
#include "led_controls.h"
#include "driver/i2c.h"

// I2C Configuration based on your image (SDA=21, SCL=22, ADDR=0x68)
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

// BMI160 Registers and Commands
#define BMI160_SENSOR_ADDR          0x68
#define BMI160_CHIP_ID_REG          0x00
#define BMI160_CMD_REG              0x7E
#define BMI160_ACCEL_DATA_ADDR      0x12
#define BMI160_GYRO_DATA_ADDR       0x0C

// Power mode commands
#define CMD_ACCEL_NORMAL            0x11
#define CMD_GYRO_NORMAL             0x15

static bool i2c_is_initialized = false;

// Internal Helper: Initialize I2C Driver
static esp_err_t i2c_master_init(void) {
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) return err;

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Internal Helper: Configure BMI160 (Wake up from suspend)
static esp_err_t bmi160_config(const char *TAG) {
    uint8_t cmd_data[2];
    esp_err_t ret;

    // Check Chip ID
    uint8_t chip_id = 0;
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, BMI160_SENSOR_ADDR, 
                                       (uint8_t[]){BMI160_CHIP_ID_REG}, 1, 
                                       &chip_id, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to communicate with BMI160. Check wiring.");
        return ret;
    }
    ESP_LOGI(TAG, "BMI160 Found. Chip ID: 0x%X", chip_id);

    // Send Command: Set Accelerometer to Normal Mode
    cmd_data[0] = BMI160_CMD_REG;
    cmd_data[1] = CMD_ACCEL_NORMAL;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, BMI160_SENSOR_ADDR, cmd_data, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for power up

    // Send Command: Set Gyroscope to Normal Mode
    cmd_data[1] = CMD_GYRO_NORMAL;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, BMI160_SENSOR_ADDR, cmd_data, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(pdMS_TO_TICKS(100)); 

    return ret;
}

// Existing DHT function
int read_dht_data(const char *TAG, dht_sensor_type_t sensor_type, int gpio_pin, pthread_mutex_t *led_mutex) {
    float temperature = 0;
    float humidity = 0;
    int res = 1;

    if (dht_read_float_data(sensor_type, gpio_pin, &humidity, &temperature) == ESP_OK) {
        ESP_LOGI(TAG, "Read: Hum: %.1f%% Temp: %.1fC", humidity, temperature);
        res = write_DHT11_to_sd(TAG, temperature, humidity);
        blink_led(3, 100, led_mutex);
        return res;
    } else {
        ESP_LOGE(TAG, "Could not read data from sensor");
        return res;
    }
}

// New BMI160 function
int read_bmi_data() {
    const char *TAG = "BMI160";
    
    // 1. Initialize Bus and Sensor ONCE
    if (!i2c_is_initialized) {
        if (i2c_master_init() == ESP_OK) {
            ESP_LOGI(TAG, "I2C Initialized");
            if (bmi160_config(TAG) == ESP_OK) {
                i2c_is_initialized = true;
            } else {
                return 1; // Failed config
            }
        } else {
            ESP_LOGE(TAG, "I2C Init Failed");
            return 1;
        }
    }

    uint8_t data[6];
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    // 2. Read Accelerometer (0x12 - 0x17)
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, BMI160_SENSOR_ADDR, 
                                     (uint8_t[]){BMI160_ACCEL_DATA_ADDR}, 1, 
                                     data, 6, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    if (ret == ESP_OK) {
        ax = (int16_t)((data[1] << 8) | data[0]);
        ay = (int16_t)((data[3] << 8) | data[2]);
        az = (int16_t)((data[5] << 8) | data[4]);
    } else {
        ESP_LOGE(TAG, "Failed to read Accel");
        return 1;
    }

    // 3. Read Gyroscope (0x0C - 0x11)
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, BMI160_SENSOR_ADDR, 
                                     (uint8_t[]){BMI160_GYRO_DATA_ADDR}, 1, 
                                     data, 6, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    if (ret == ESP_OK) {
        gx = (int16_t)((data[1] << 8) | data[0]);
        gy = (int16_t)((data[3] << 8) | data[2]);
        gz = (int16_t)((data[5] << 8) | data[4]);
        
        // Log the RAW data
        // Note: To get "g" or "deg/s", you need to divide by the sensitivity scale 
        // (default +/- 2g is 16384 LSB/g, +/- 2000dps is 16.4 LSB/dps)
        ESP_LOGI(TAG, "Accel [X:%d Y:%d Z:%d] | Gyro [X:%d Y:%d Z:%d]", ax, ay, az, gx, gy, gz);
        return 0;
    } else {
        ESP_LOGE(TAG, "Failed to read Gyro");
        return 1;
    }
}
