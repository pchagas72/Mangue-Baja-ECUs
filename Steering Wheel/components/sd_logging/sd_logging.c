#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_logging.h"
#include "driver/sdspi_host.h"

static const char *TAG = "SD_LOG";
static sdmmc_card_t *card;
static char current_filename[32];
static bool is_mounted = false;

esp_err_t sd_logging_init(void)
{
    esp_err_t ret;

    // SPI Configuration
    ESP_LOGI(TAG, "Initializing SD card via SPI...");
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    host.max_freq_khz = 400; // 5MHz is safe for long wires

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize SPI Bus
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus.");
        return ret;
    }

    // Mount Configuration
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = host.slot;

    // Mount Filesystem
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. Check if SD card is inserted.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "SD Card mounted successfully!");
    is_mounted = true;

    // Find unique filename (log_0.csv, log_1.csv...)
    int i = 0;
    while (1) {
        sprintf(current_filename, "%s/log_%d.csv", MOUNT_POINT, i);
        struct stat st;
        if (stat(current_filename, &st) != 0) {
            // File doesn't exist, we can use this name
            break;
        }
        i++;
    }
    ESP_LOGI(TAG, "Logging to: %s", current_filename);

    // Write Header
    FILE *f = fopen(current_filename, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "Time_ms,RPM,Speed_KPH,Fuel_Pct,Volts,CVT_Temp,Eng_Temp,Roll,Pitch\n");
    fclose(f);

    return ESP_OK;
}

void sd_log_data(car_state_t *car, uint32_t timestamp_ms)
{
    if (!is_mounted) return;

    // Open in Append mode ("a")
    FILE *f = fopen(current_filename, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending");
        return;
    }

    // Write Data Line
    // Format: Time, RPM, Speed, Fuel, Volt, CVT, ENG, Roll, Pitch
    fprintf(f, "%lu,%d,%d,%d,%.2f,%d,%d,%d,%d\n",
            timestamp_ms,
            car->rpm,
            car->speed,
            car->fuel,
            car->voltage,
            car->cvt_temp,
            car->eng_temp,
            car->roll,
            car->pitch
    );

    // Close immediately to save data in case of power loss (Baja vibration!)
    fclose(f);
}

void sd_logging_deinit(void)
{
    if (is_mounted) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        ESP_LOGI(TAG, "Card unmounted");
        is_mounted = false;
    }
}
