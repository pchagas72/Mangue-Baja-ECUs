#include "can_management.h"
#include "can.h" // Resolve hcan

void CAN_Send_Speed(uint16_t speed) {
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t can_data[2];

    TxHeader.StdId = 0x300;
    TxHeader.ExtId = 0;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 2;
    TxHeader.TransmitGlobalTime = DISABLE;

    can_data[0] = speed & 0xFF;
    can_data[1] = (speed >> 8) & 0xFF;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, can_data, &TxMailbox);
}

void CAN_Send_Voltage(uint16_t voltage) {
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t can_data[2];

    TxHeader.StdId = 0x502;
    TxHeader.ExtId = 0;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 2;
    TxHeader.TransmitGlobalTime = DISABLE;

    can_data[0] = voltage & 0xFF;
    can_data[1] = (voltage >> 8) & 0xFF;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, can_data, &TxMailbox);
}

void CAN_Send_Temp_NTC(int16_t temp_ntc) {
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t can_data[2];

    TxHeader.StdId = 0x400;
    TxHeader.ExtId = 0;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 2;
    TxHeader.TransmitGlobalTime = DISABLE;

    can_data[0] = temp_ntc & 0xFF;
    can_data[1] = (temp_ntc >> 8) & 0xFF;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, can_data, &TxMailbox);
}

void CAN_Send_Temp_CVT(int16_t temp_cvt) {
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t can_data[2];

    TxHeader.StdId = 0x401;
    TxHeader.ExtId = 0;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 2;
    TxHeader.TransmitGlobalTime = DISABLE;

    can_data[0] = temp_cvt & 0xFF;
    can_data[1] = (temp_cvt >> 8) & 0xFF;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, can_data, &TxMailbox);
}

void CAN_Send_Debug(can_debug_packet_t *debug_packet) {
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t can_data[4];

    // Endereço para log de debug da TCU (escolhido 0x600 como exemplo)
    TxHeader.StdId = 0x600;
    TxHeader.ExtId = 0;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 4;
    TxHeader.TransmitGlobalTime = DISABLE;

    can_data[0] = debug_packet->current_state;
    can_data[1] = debug_packet->CAN_ok;
    can_data[2] = debug_packet->MLX_ok;
    can_data[3] = debug_packet->boot_tries;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, can_data, &TxMailbox);
}
