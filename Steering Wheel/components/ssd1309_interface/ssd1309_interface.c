#include "include/ssd1309_interface.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// 1 = Mirror/Flip, 0 = Normal (Adjust to how to screen is mounted)
#define SSD1309_FLIP_X  1  
#define SSD1309_FLIP_Y  1

// --- Font Data (5x7) ---
const uint8_t font[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // space
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, //  
    0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x0C, 0x52, 0x52, 0x52, 0x3E, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x7F, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44  // z
};

esp_err_t ssd1309_hw_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle) {

    // I2C Master bus configuration
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    // Allocate an I2C master bus
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, bus_handle));

    // I2C device configuration
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SSD1309_ADDR,
        .scl_speed_hz = 400000, // Fast-mode
    };

    // Add device to the master bus [cite: 580, 154]
    return i2c_master_bus_add_device(*bus_handle, &dev_cfg, dev_handle);
}

void ssd1309_write_cmd(i2c_master_dev_handle_t dev_handle, uint8_t command) {
    uint8_t cmd_buf[2] = {0x00, command}; // 0x00 indicates following byte is a command
    // Perform write transaction using new driver [cite: 603, 254]
    i2c_master_transmit(dev_handle, cmd_buf, sizeof(cmd_buf), -1);
}

void ssd1309_init(i2c_master_dev_handle_t dev_handle) {
    // Hardware Reset
    gpio_set_direction(PIN_RES, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RES, 0); vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_RES, 1); vTaskDelay(pdMS_TO_TICKS(100));

    // Clear Screen before turning on
    uint8_t blank[129];
    blank[0] = 0x40; // 0x40 indicates following bytes are data
    memset(&blank[1], 0, 128);

    for(int p=0; p<8; p++) {
        ssd1309_write_cmd(dev_handle, 0xB0 | p); 
        ssd1309_write_cmd(dev_handle, 0x00); 
        ssd1309_write_cmd(dev_handle, 0x10);
        
        i2c_master_transmit(dev_handle, blank, sizeof(blank), -1);
    }

    // Init Commands
    ssd1309_write_cmd(dev_handle, 0xAE); // OFF
    ssd1309_write_cmd(dev_handle, 0xFD); ssd1309_write_cmd(dev_handle, 0x12); // Unlock
    ssd1309_write_cmd(dev_handle, 0x20); ssd1309_write_cmd(dev_handle, 0x02); // PAGE MODE
    ssd1309_write_cmd(dev_handle, 0x81); ssd1309_write_cmd(dev_handle, 0x01); // Contrast
    
    ssd1309_write_cmd(dev_handle, SSD1309_FLIP_X ? 0xA1 : 0xA0); 
    ssd1309_write_cmd(dev_handle, SSD1309_FLIP_Y ? 0xC8 : 0xC0); 
    
    ssd1309_write_cmd(dev_handle, 0xA8); ssd1309_write_cmd(dev_handle, 0x3F);
    ssd1309_write_cmd(dev_handle, 0xD3); ssd1309_write_cmd(dev_handle, 0x00);
    ssd1309_write_cmd(dev_handle, 0x40); 
    ssd1309_write_cmd(dev_handle, 0xD5); ssd1309_write_cmd(dev_handle, 0x80);
    ssd1309_write_cmd(dev_handle, 0xD9); ssd1309_write_cmd(dev_handle, 0xF1); 
    ssd1309_write_cmd(dev_handle, 0xDA); ssd1309_write_cmd(dev_handle, 0x12);
    ssd1309_write_cmd(dev_handle, 0xDB); ssd1309_write_cmd(dev_handle, 0x40);
    ssd1309_write_cmd(dev_handle, 0xA4); 
    ssd1309_write_cmd(dev_handle, 0xA6); 
    ssd1309_write_cmd(dev_handle, 0xAF); // ON
}

esp_err_t ssd1309_display_buffer(i2c_master_dev_handle_t dev_handle, uint8_t *buffer) {
    uint8_t tx_buf[129];
    tx_buf[0] = 0x40; // Data control byte

    ssd1309_write_cmd(dev_handle, 0x40); 

    for (int page = 0; page < 8; page++) {
        ssd1309_write_cmd(dev_handle, 0xB0 | page); 
        ssd1309_write_cmd(dev_handle, 0x00); 
        ssd1309_write_cmd(dev_handle, 0x10); 

        memcpy(&tx_buf[1], &buffer[page * 128], 128);
        
        // Transmit page data [cite: 603]
        esp_err_t err = i2c_master_transmit(dev_handle, tx_buf, sizeof(tx_buf), 100);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

void ssd1309_clear_buffer(uint8_t *buffer) {
    memset(buffer, 0, SSD1309_BUFFER_SIZE);
}

void ssd1309_draw_pixel(uint8_t *buffer, int x, int y, int color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT || x < 0 || y < 0) return;
    int index = x + (y / 8) * SCREEN_WIDTH;
    int bit = y % 8;
    if (color) buffer[index] |= (1 << bit);
    else buffer[index] &= ~(1 << bit);
}

void ssd1309_draw_rect(uint8_t *buffer, int x, int y, int w, int h, int color, int fill) {
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            if (fill || i == x || i == x + w - 1 || j == y || j == y + h - 1) {
                ssd1309_draw_pixel(buffer, i, j, color);
            }
        }
    }
}

void ssd1309_draw_char(uint8_t *buffer, int x, int y, char c) {
    if (c < 32 || c > 122) c = 32; 
    int font_idx = c - 32;         
    for (int col = 0; col < 5; col++) {
        uint8_t line = font[font_idx * 5 + col];
        for (int row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                ssd1309_draw_pixel(buffer, x + col, y + row, 1);
            }
        }
    }
}

void ssd1309_draw_string(uint8_t *buffer, int x, int y, const char *format, ...) {
    char temp_str[64]; 
    va_list args;
    va_start(args, format);
    vsnprintf(temp_str, sizeof(temp_str), format, args);
    va_end(args);
    int cursor_x = x;
    char *str = temp_str;
    while (*str) {
        if (cursor_x > SCREEN_WIDTH - 6) { cursor_x = x; y += 8; }
        ssd1309_draw_char(buffer, cursor_x, y, *str);
        cursor_x += 6; 
        str++;
    }
}

void ssd1309_draw_string_large(uint8_t *buffer, int x, int y, int size, const char *format, ...) {
    char temp_str[64]; 
    va_list args;
    va_start(args, format);
    vsnprintf(temp_str, sizeof(temp_str), format, args);
    va_end(args);
    char *str = temp_str;
    int cursor_x = x;
    while (*str) {
        char c = *str;
        if (c < 32 || c > 122) c = 32;
        int font_idx = c - 32;
        for (int col = 0; col < 5; col++) {
            uint8_t line = font[font_idx * 5 + col];
            for (int row = 0; row < 8; row++) {
                if (line & (1 << row)) {
                    ssd1309_draw_rect(buffer, cursor_x + (col * size), y + (row * size), size, size, 1, 1);
                }
            }
        }
        cursor_x += (6 * size); 
        str++;
    }
}

void ssd1309_draw_line(uint8_t *buffer, int x0, int y0, int x1, int y1, int color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        ssd1309_draw_pixel(buffer, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ssd1309_draw_bitmap(uint8_t *fb, int x, int y, const uint8_t *bitmap, int w, int h, int color) {
    int byteWidth = (w + 7) / 8; 
    
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            if (bitmap[j * byteWidth + i / 8] & (128 >> (i & 7))) {
                ssd1309_draw_pixel(fb, x + i, y + j, color);
            }
        }
    }
}
