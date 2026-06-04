#ifndef CAN_MANAGEMENT_H
#define CAN_MANAGEMENT_H

#include "main.h"
#include <stdbool.h>

/* Estrutura para pacote de Saúde da TCU */
typedef struct {
    uint8_t current_state;
    bool CAN_ok;
    bool MLX_ok;
    uint8_t boot_tries;
} can_debug_packet_t;

void CAN_Send_Speed(uint16_t speed);
void CAN_Send_Voltage(uint16_t voltage);
void CAN_Send_Temp_NTC(int16_t temp_ntc);
void CAN_Send_Temp_CVT(int16_t temp_cvt);
void CAN_Send_Debug(can_debug_packet_t *debug_packet);

#endif // CAN_MANAGEMENT_H
