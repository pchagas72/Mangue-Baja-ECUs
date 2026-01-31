#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "../../can_management/include/can_management.h"

// HIJACKING CAN PINS FOR MODEM TEST
#define MODEM_TX_PIN GPIO_NUM_21  // Connect to Modem RX
#define MODEM_RX_PIN GPIO_NUM_22  // Connect to Modem TX

#define MODEM_RST_PIN GPIO_NUM_4 

#define MODEM_UART_NUM UART_NUM_1
#define MODEM_BAUD_RATE 115200
#define BUF_SIZE 1024

void initialize_modem(const char *TAG);
int send_at_command(const char *TAG, const char *command);
void mqtt_publish_fixed(const char *TAG, can_packet *pkt);
