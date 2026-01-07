#pragma once

#define MOUNT_POINT "/sdcard"
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

void initialize_sd(const char *TAG);
int write_packet_to_sd(const char *TAG);
