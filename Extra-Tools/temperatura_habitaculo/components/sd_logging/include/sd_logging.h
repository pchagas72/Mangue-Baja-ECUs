#pragma once

#define MOUNT_POINT "/sdcard"
#define PIN_NUM_CS   15
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14
#define PIN_NUM_MISO 26

void initialize_sd(const char *TAG);
void write_DHT11_to_sd(const char *TAG, float temp, float hum);
