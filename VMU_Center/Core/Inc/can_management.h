#ifndef CAN_MANAGEMENT_H
#define CAN_MANAGEMENT_H

#include <stdint.h>
#include "state_machine.h"
#include "imu.h"

typedef struct can_debug_packet{
    uint8_t boot_tries;
    uint8_t current_state;
    bool IMU_ok;
    bool CAN_ok;
    bool RPM_warning;
} can_debug_packet_t;

void CAN_Send_Debug(can_debug_packet_t *debug_packet);
void CAN_Send_RPM(uint16_t rpm);
void CAN_Send_IMU(imu_processed_data_t *imu_data);

#endif /* CAN_MANAGEMENT_H */
