#include "include/sd_logging.h"
#include "esp_err.h"
#include <stdio.h>
#include <esp_log.h>
#include "esp_vfs_fat.h"

void initialize_sd(const char *TAG){
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SD card via SPI...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    // Mount the filesystem
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        return;
    }
 
}

// Write file to SD
// Add packet as parameter
int write_packet_to_sd(const char *TAG) {
    ESP_LOGI(TAG, "Opening file to write...");
    
    // Open file in append mode ('a')
    FILE *f = fopen(MOUNT_POINT"/log.txt", "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return 1;
    }
    
    // Write data (CSV format: Timestamp, Temp, Hum)
    // TODO: ADD RTC
    // fprintf(f, "DATA PACKET");
    
    fclose(f);
    ESP_LOGI(TAG, "Data written successfully");
    return 0;
}
