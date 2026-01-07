#include "SD_state_machine.h"

// Define timeout time in milliseconds,0 (example: 2000ms = 2s)
const long timeoutTime = 3000;
bool mounted = false;
char file_name[20];
File dataFile;
/* Debug Variables */
boolean savingBlink = false;
boolean saveFlag = false;

uint8_t start_SD_device()
{
  do
  { Serial.println("Mount SD..."); } while (!sdConfig() && millis() < timeoutTime);

  if (!mounted)
  {
    Serial.println("SD mounted error!!");
    return FAIL_RESPONSE;
  }
    
  sdSave(true);
  setup_SD_ticker();

  return SUCESS_RESPONSE;
}

bool sdConfig()
{
  if (!SD.begin(SD_CS))
    return false;

  File root;
  root = SD.open("/");
  int num_files = countFiles(root);
  sprintf(file_name, "/%s%d.csv", "data", num_files + 1);
  mounted = true;

  return true;
}

int countFiles(File dir)
{
  int fileCountOnSD = 0; // for counting files
  for (;;)
  {
    File entry = dir.openNextFile();

    // no more files
    if (!entry)
      break;

    // for each file count it
    fileCountOnSD++;
    entry.close();
  }

  return fileCountOnSD - 1;
}

uint8_t sdSave(bool set)
{
  uint8_t check_sd = FAIL_RESPONSE;

  dataFile = SD.open(file_name, FILE_APPEND);

  if (dataFile)
  {
    dataFile.println(packetToString(set));
    dataFile.close();
    savingBlink = !savingBlink;
    digitalWrite(DEBUG_LED, savingBlink);
    check_sd = SUCESS_RESPONSE;
  }
  
  else
  {
    digitalWrite(DEBUG_LED, LOW);
    Serial.println(F("falha no save"));
    check_sd = FAIL_RESPONSE;
  }
  
  //Serial.println("Into the function");
  //Serial.print("check_sd --> ");
  //Serial.println(check_sd);

  return check_sd;
}

String packetToString(bool err)
{
  mqtt_packet_t SD_data = update_packet();

  String dataString = "";
  if (err)
  {
    dataString += "ACCX";
    dataString += ",";
    dataString += "ACCY";
    dataString += ",";
    dataString += "ACCZ";
    dataString += ",";
    dataString += "DPSX";
    dataString += ",";
    dataString += "DPSY";
    dataString += ",";
    dataString += "DPSZ";
    dataString += ",";
    dataString += "ROLL";
    dataString += ",";
    dataString += "PITCH";
    dataString += ",";

    dataString += "RPM";
    dataString += ",";
    dataString += "VEL";
    dataString += ",";
    dataString += "TEMP_MOTOR";
    dataString += ",";
    dataString += "SOC";
    dataString += ",";
    dataString += "TEMP_CVT";
    dataString += ",";
    // dataString += "FUEL_LEVEL";
    // dataString += ",";
    dataString += "VOLT";
    dataString += ",";
    dataString += "CURRENT";
    dataString += ",";
    dataString += "FLAGS";
    dataString += ",";
    dataString += "LATITUDE";
    dataString += ",";
    dataString += "LONGITUDE";
    dataString += ",";
    dataString += "TIMESTAMP";
  }

  else
  {
    // imu
    dataString += String((SD_data.imu_acc.acc_x * 0.061) / 1000);
    dataString += ",";
    dataString += String((SD_data.imu_acc.acc_y * 0.061) / 1000);
    dataString += ",";
    dataString += String((SD_data.imu_acc.acc_z * 0.061) / 1000);
    dataString += ",";
    dataString += String(SD_data.imu_dps.dps_x);
    dataString += ",";
    dataString += String(SD_data.imu_dps.dps_y);
    dataString += ",";
    dataString += String(SD_data.imu_dps.dps_z);
    dataString += ",";
    dataString += String(SD_data.Angle.Roll);
    dataString += ",";
    dataString += String(SD_data.Angle.Pitch);

    dataString += ",";
    dataString += String(SD_data.rpm);
    dataString += ",";
    dataString += String(SD_data.speed);
    dataString += ",";
    dataString += String(SD_data.temperature);
    dataString += ",";
    dataString += String(SD_data.SOC);
    dataString += ",";
    dataString += String(SD_data.cvt);
    dataString += ",";
    // dataString += String(SD_data.fuel);
    // dataString += ",";
    dataString += String(SD_data.volt);
    dataString += ",";
    dataString += String(SD_data.current);
    dataString += ",";
    dataString += String(SD_data.flags);
    dataString += ",";
    dataString += String(SD_data.latitude);
    dataString += ",";
    dataString += String(SD_data.longitude);
    dataString += ",";
    dataString += String(SD_data.timestamp);
  }

  return dataString;
}

uint8_t Check_SD_for_storage()
{
  static uint8_t sd_status = FAIL_RESPONSE;

  if (saveFlag && mounted)
  {
    sd_status = sdSave(false);
    saveFlag = false;
  }

  return sd_status;
}

/* Ticker routine */
Ticker ticker40Hz;

void setup_SD_ticker()
{
  ticker40Hz.attach(0.025f, ticker40HzISR);
}

void ticker40HzISR()
{
  saveFlag = true; // set to true when the ECU store the CAN datas
}
