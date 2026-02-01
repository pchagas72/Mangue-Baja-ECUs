#pragma once
#include "esp_err.h"
#include "driver/i2c_master.h" // New driver [cite: 62]
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2C Pins
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define PIN_RES                     4
#define I2C_MASTER_NUM              0
#define SSD1309_ADDR                0x3C

// Resolution
#define SCREEN_WIDTH                128
#define SCREEN_HEIGHT               64
#define SSD1309_BUFFER_SIZE         1024

// Core Functions - Updated for new driver handles
esp_err_t ssd1309_hw_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);
void ssd1309_init(i2c_master_dev_handle_t dev_handle);
esp_err_t ssd1309_display_buffer(i2c_master_dev_handle_t dev_handle, uint8_t *buffer);
void ssd1309_clear_buffer(uint8_t *buffer);

// Graphics
void ssd1309_draw_pixel(uint8_t *buffer, int x, int y, int color);
void ssd1309_draw_rect(uint8_t *buffer, int x, int y, int w, int h, int color, int fill);
void ssd1309_draw_char(uint8_t *buffer, int x, int y, char c);
void ssd1309_draw_string(uint8_t *buffer, int x, int y, const char *format, ...);
void ssd1309_draw_string_large(uint8_t *buffer, int x, int y, int size, const char *format, ...);
void ssd1309_draw_line(uint8_t *buffer, int x0, int y0, int x1, int y1, int color);
void ssd1309_draw_bitmap(uint8_t *fb, int x, int y, const uint8_t *bitmap, int w, int h, int color);

#ifdef __cplusplus
}
#endif
