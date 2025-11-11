#include "CAN.h"

CANmsg txMsg(CAN_RX_id, CAN_TX_id, CAN_BPS_1000K);
radio_packet_t can_receive_packet;
bool mode = false;

bool CAN_start_device(bool debug_mode)
{
  txMsg.Set_Debug_Mode(debug_mode);
  if (!txMsg.init(canISR))
  {
    Serial.println("CAN ERROR!! SYSTEM WILL RESTART IN 2 SECONDS...");
    vTaskDelay(2000);
    return false;
  }
  memset(&can_receive_packet, 0, sizeof(radio_packet_t));
  return true;
}

bool Send_GPS_data(double _msg, uint32_t _ID)
{
  txMsg.clear(_ID);
  txMsg << _msg;
  return txMsg.write();
}

bool Send_MPU_REQUEST(bool _msg)
{
  txMsg.clear(MPU_ID);
  txMsg << _msg;
  return txMsg.write();
}

// Safely copies the packet from the ISR's global variable.
radio_packet_t get_radio_packet()
{
  radio_packet_t local_copy;

  // Disable interrupts to prevent data corruption during the copy.
  noInterrupts();
  memcpy(&local_copy, &can_receive_packet, sizeof(radio_packet_t));
  interrupts();

  return local_copy;
}

/* CAN ISR - This now handles all incoming CAN messages */
void canISR(CAN_FRAME *rxMsg)
{
  mode = !mode;
  digitalWrite(EMBEDDED_LED, mode);

  can_receive_packet.timestamp = millis();

  switch (rxMsg->id)
  {
  case IMU_ACC_ID:
    memcpy(&can_receive_packet.imu_acc, (imu_acc_t *)&rxMsg->data.uint8, sizeof(imu_acc_t));
    break;
  case IMU_DPS_ID:
    memcpy(&can_receive_packet.imu_dps, (imu_dps_t *)&rxMsg->data.uint8, sizeof(imu_dps_t));
    break;
  case ANGLE_ID:
    memcpy(&can_receive_packet.Angle, (Angle_t *)&rxMsg->data.uint8, sizeof(Angle_t));
    break;
  case RPM_ID:
    memcpy(&can_receive_packet.rpm, (uint16_t *)&rxMsg->data.uint8, sizeof(uint16_t));
    break;
  case SPEED_ID:
    memcpy(&can_receive_packet.speed, (uint16_t *)&rxMsg->data.uint8, sizeof(uint16_t));
    break;
  case TEMPERATURE_ID:
    memcpy(&can_receive_packet.temperature, (uint8_t *)&rxMsg->data.uint8, sizeof(uint8_t));
    break;
  case FLAGS_ID:
    memcpy(&can_receive_packet.flags, (uint8_t *)&rxMsg->data.uint8, sizeof(uint8_t));
    break;
  case SOC_ID:
    memcpy(&can_receive_packet.SOC, (uint8_t *)&rxMsg->data.uint8, sizeof(uint8_t));
    break;
  case CVT_ID:
    memcpy(&can_receive_packet.cvt, (uint8_t *)&rxMsg->data.uint8, sizeof(uint8_t));
    break;
  case VOLTAGE_ID:
    memcpy(&can_receive_packet.volt, (float *)&rxMsg->data.uint8, sizeof(float));
    break;
  case CURRENT_ID:
    memcpy(&can_receive_packet.current, (float *)&rxMsg->data.uint8, sizeof(float));
    break;
  // Note: LAT_ID and LNG_ID are handled by the GPS task in the StateMachine
  }
}
