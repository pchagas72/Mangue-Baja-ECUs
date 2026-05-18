#ifndef EBYTE_E32_H
#define EBYTE_E32_H

#include <stdint.h>
#include "driver/uart.h"
#include "driver/gpio.h"

/* --- Adjusted pins to match your KiCad Schematic --- */
/*
#define E32_UART_NUM UART_NUM_2
#define E32_TX_PIN   GPIO_NUM_16
#define E32_RX_PIN   GPIO_NUM_17
#define E32_M0_PIN   GPIO_NUM_12
#define E32_M1_PIN   GPIO_NUM_14
#define E32_AUX_PIN  GPIO_NUM_13
*/

#define E32_UART_NUM UART_NUM_2
#define E32_TX_PIN   GPIO_NUM_32 // RX_LORA
#define E32_RX_PIN   GPIO_NUM_33 // TX_LORA
#define E32_M0_PIN   GPIO_NUM_13 // Ler como a voltagem do M0 e M1 influenciam no funcionamento do módulo
#define E32_M1_PIN   GPIO_NUM_26 // Ler como a voltagem do M0 e M1 influenciam no funcionamento do módulo
#define E32_AUX_PIN  GPIO_NUM_27 // Ler como a voltagem AUX influencia no funcionamento do módulo

//void e32_init(void);
//void e32_set_mode_transparent(void);
//void e32_send_data(const uint8_t *data, size_t len);
//int  e32_receive_data(uint8_t *buffer, size_t max_len);

void e32_init(void);
void e32_send_struct(void *data, size_t size);
// void e32_send_string(const char *str);

#endif
