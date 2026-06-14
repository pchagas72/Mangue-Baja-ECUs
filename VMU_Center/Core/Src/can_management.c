#include "../Inc/can_management.h"
#include "stm32f1xx_hal.h"

extern CAN_HandleTypeDef hcan;
extern vmu_state_t ecu_state;

/* Sending */
#define CAN_ID_DEBUG  0x102
#define CAN_ID_IMU    0x205
#define CAN_ID_RPM    0x304

/* Receiving*/
#define CAN_ID_VOLTAGE_REAR 0x502
#define LOW_VOLTAGE_THRESHOLD 1050

void CAN_Send_Debug(can_debug_packet_t *debug_packet){
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[5] = {0};

    TxHeader.StdId = CAN_ID_DEBUG;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 5; 
    TxHeader.TransmitGlobalTime = DISABLE;

    TxData[0] = (uint8_t)debug_packet->current_state;
    TxData[1] = debug_packet->IMU_ok ? 1 : 0;
    TxData[2] = debug_packet->CAN_ok ? 1 : 0;
    TxData[3] = debug_packet->RPM_warning ? 1 : 0;
    TxData[4] = (uint8_t)debug_packet->boot_tries;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);
}

void CAN_Send_RPM(uint16_t rpm) {
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[2];

    TxHeader.StdId = CAN_ID_RPM;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 2; 
    TxHeader.TransmitGlobalTime = DISABLE;

    // Respeitando o empacotamento original do ECU_Navegacao (Little-Endian)
    TxData[0] = rpm & 0xFF;
    TxData[1] = (rpm >> 8) & 0xFF;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);
}

void CAN_Send_IMU(imu_processed_data_t *imu_data) {
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[4];

    TxHeader.StdId = CAN_ID_IMU;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 4; 
    TxHeader.TransmitGlobalTime = DISABLE;

    // Respeitando o empacotamento original (Little-Endian)
    TxData[0] = imu_data->roll & 0xFF;
    TxData[1] = (imu_data->roll >> 8) & 0xFF;
    TxData[2] = imu_data->pitch & 0xFF;
    TxData[3] = (imu_data->pitch >> 8) & 0xFF;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);
}

// Callback for FIFO1 where your filter drops the messages
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK) {
        
        // Listen for the Voltage broadcasted from VMU_Rear_local
        if (RxHeader.StdId == CAN_ID_VOLTAGE_REAR) {
            
            // Reconstruct the 16-bit voltage (Little-Endian per VMU_Rear packing)
            uint16_t rear_voltage = RxData[0] | (RxData[1] << 8);
            
            // Update the state machine flag
            if (rear_voltage < LOW_VOLTAGE_THRESHOLD) {
                ecu_state.low_voltage = true;
            } else {
                ecu_state.low_voltage = false;
            }
        }
    }
}
