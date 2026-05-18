#pragma once

#include "esp_err.h"
#include "can_management.h"

// --- PIN CONFIGURATION (HSPI) ---
#define SD_CS    GPIO_NUM_5
#define SD_MOSI  GPIO_NUM_23
#define SD_CLK   GPIO_NUM_18
#define SD_MISO  GPIO_NUM_19

#define MOUNT_POINT "/sdcard"

esp_err_t sd_logging_init(void);

void sd_log_data(car_state_t *car, uint32_t timestamp_ms);

void sd_logging_deinit(void);
