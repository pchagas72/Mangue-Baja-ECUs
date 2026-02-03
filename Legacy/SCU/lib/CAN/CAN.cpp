#include "CAN.h"

bool mode = false;
mqtt_packet_t can_receive_packet;
bool BLE_request = false;
CANmsg txMsg(CAN_RX_id, CAN_TX_id, CAN_BPS_1000K);

bool CAN_start_device(bool debug_mode)
{
  txMsg.Set_Debug_Mode(debug_mode);
  if (!txMsg.init(canISR))
  {
    Serial.println("CAN ERROR!! SYSTEM WILL RESTART IN 2 SECONDS...");
    //log_e("CAN ERROR!! SYSTEM WILL RESTART IN 2 SECONDS...");
    vTaskDelay(2000);
    return false;
  }
  /* if you needed apply a Mask and id to filtering write this:
   * txMsg.init(canISR, ID); or txMsg.init(canISR, ID, Mask);
   * if you ID is bigger then 0x7FF the extended mode is activated
   * you can set the filter in this function too:
   * txMsg.Set_Filter(uint32_t ID, uint32_t Mask, bool Extended);  */
  memset(&can_receive_packet, 0, sizeof(mqtt_packet_t));
  return true;
}

void Send_SOT_msg(uint8_t _msg)
{
  vTaskDelay(1);
  /* Sent State of Telemetry (SOT) */
  txMsg.clear(SOT_ID);
  txMsg << _msg;
  txMsg.write();

  /*
   * If you send a buffer message you can use this function:
   * SendMsgBuffer(uint32_t Id, bool ext, uint8_t length, unsigned char* data);
   * or you use this:
   * txMsg << data1 << data2 << data3 ...;
   */
}

void Send_SCU_FLAGS(bluetooth ble)
{
  uint8_t _msg = 0x00;

  // 2 bits per message
  _msg |= (ble.internet_modem | (ble.mqtt_client_connection << 2)); // Create LTE flag
  _msg |= ((ble.sd_start << 4) | (ble.check_sd << 6));              // Create SD flag

  //Serial.println(_msg);

  vTaskDelay(1);

  txMsg.clear(SCU_ID);
  txMsg << _msg;
  txMsg.write();

  /*
   * If you send a buffer message you can use this function:
   * SendMsgBuffer(uint32_t Id, bool ext, uint8_t length, unsigned char* data);
   * or you use this:
   * txMsg << data1 << data2 << data3 ...;
   */
}

mqtt_packet_t update_packet()
{
  return can_receive_packet;
}

bool MPU_request_Debug_data(void)
{
  bool t = BLE_request;
  BLE_request = false;
  return t;
}

/* CAN functions */
void canISR(CAN_FRAME *rxMsg)
{
  mode = !mode;
  digitalWrite(EMBEDDED_LED, mode);

  can_receive_packet.timestamp = millis();

  switch (rxMsg->id)
  {
  case IMU_ACC_ID:
    memcpy(&can_receive_packet.imu_acc, (imu_acc_t *)&rxMsg->data.uint8, sizeof(imu_acc_t));
    // Serial.printf("ACC X = %f\r\n", (float)((can_receive_packet.imu_acc.acc_x*0.061)/1000));
    // Serial.printf("ACC Y = %f\r\n", (float)((can_receive_packet.imu_acc.acc_y*0.061)/1000));
    // Serial.printf("ACC Z = %f\r\n", (float)((can_receive_packet.imu_acc.acc_z*0.061)/1000));
    break;

  case IMU_DPS_ID:
    memcpy(&can_receive_packet.imu_dps, (imu_dps_t *)&rxMsg->data.uint8, sizeof(imu_dps_t));
    // Serial.printf("DPS X = %d\r\n", can_receive_packet.imu_dps.dps_x);
    // Serial.printf("DPS Y = %d\r\n", can_receive_packet.imu_dps.dps_y);
    // Serial.printf("DPS Z = %d\r\n", can_receive_packet.imu_dps.dps_z);
    break;

  case ANGLE_ID:
    memcpy(&can_receive_packet.Angle, (Angle_t *)&rxMsg->data.uint8, sizeof(Angle_t));
    // Serial.printf("Angle Roll = %d\r\n", can_receive_packet.Angle.Roll);
    // Serial.printf("Angle Pitch = %d\r\n", can_receive_packet.Angle.Pitch);
    break;

  case RPM_ID:
    memcpy(&can_receive_packet.rpm, (uint16_t *)&rxMsg->data.uint8, sizeof(uint16_t));
    // Serial.printf("RPM = %d\r\n", can_receive_packet.rpm);
    break;

  case SPEED_ID:
    memcpy(&can_receive_packet.speed, (uint16_t *)&rxMsg->data.uint8, sizeof(uint16_t));
    // Serial.printf("Speed = %d\r\n", can_receive_packet.speed);
    break;

  case TEMPERATURE_ID:
    memcpy(&can_receive_packet.temperature, (uint8_t *)&rxMsg->data.uint8, sizeof(uint8_t));
    // Serial.printf("Motor = %d\r\n", can_receive_packet.temperature);
    break;

  case FLAGS_ID:
    memcpy(&can_receive_packet.flags, (uint8_t *)&rxMsg->data.uint8, sizeof(uint8_t));
    // Serial.printf("Flags = %d\r\n", can_receive_packet.flags);
    break;

  case SOC_ID:
    memcpy(&can_receive_packet.SOC, (uint8_t *)&rxMsg->data.uint8, sizeof(uint8_t));
    // Serial.printf("SOC = %d\r\n", can_receive_packet.SOC);
    break;

  case CVT_ID:
    memcpy(&can_receive_packet.cvt, (uint8_t *)&rxMsg->data.uint8, sizeof(uint8_t));
    // Serial.printf("CVT = %d\r\n", can_receive_packet.cvt);
    break;

  case VOLTAGE_ID:
    memcpy(&can_receive_packet.volt, (float *)&rxMsg->data.uint8, sizeof(float));
    // Serial.printf("Volt = %f\r\n", can_receive_packet.volt);
    break;

  case CURRENT_ID:
    memcpy(&can_receive_packet.current, (float *)&rxMsg->data.uint8, sizeof(float));
    // Serial.printf("Current = %f\r\n", can_receive_packet.current);
    break;

  case LAT_ID:
    memcpy(&can_receive_packet.latitude, (double *)&rxMsg->data.uint8, sizeof(double));
    // Serial.printf("Latitude (LAT) = %lf\r\n", can_receive_packet.latitude);
    break;

  case LNG_ID:
    memcpy(&can_receive_packet.longitude, (double *)&rxMsg->data.uint8, sizeof(double));
    // Serial.printf("Longitude (LNG) = %lf\r\n", can_receive_packet.longitude);
    break;

  case MPU_ID:
    memcpy(&BLE_request, (bool *)&rxMsg->data.uint8, sizeof(bool));
    // Serial.printf("BLE_request = %d\r\n", BLE_request);
    break;
  }
}
