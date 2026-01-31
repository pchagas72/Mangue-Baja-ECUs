#include "CON_state_machine.h"

/* GPRS credentials */
#ifdef TIM
  const char *apn = "timbrasil.br";    // Your APN
  const char *gprsUser = "tim";        // User
  const char *gprsPass = "tim";        // Password
  const char *simPIN = "1010";         // SIM card PIN code, if any
#elif defined(CLARO)
  const char *apn = "claro.com.br";    // Your APN
  const char *gprsUser = "claro";      // User
  const char *gprsPass = "claro";      // Password
  const char *simPIN = "3636";         // SIM cad PIN code, id any
#elif defined(VIVO)
  const char *apn = "zap.vivo.com.br";  // Your APN
  const char *gprsUser = "vivo";        // User
  const char *gprsPass = "vivo";        // Password
  const char *simPIN = "8486";          // SIM cad PIN code, id any
#else
  const char *apn = "timbrasil.br";    // Your APN
  const char *gprsUser = "tim";        // User
  const char *gprsPass = "tim";        // Password
  const char *simPIN = "1010";         // SIM card PIN code, if any
#endif

// Flags to ticker function 
bool sendFlag = false;
bool buff = false;

/* Variables to storage the data in array and send the 20 packets of 64 bytes */
uint8_t volatile_bytes[MSG_BUFFER_SIZE];
int volatile_position = 0;

const char *node_server = "69.55.61.114";
char payload_char[MSG_BUFFER_SIZE];
char msg[MSG_BUFFER_SIZE];

/* GSM definitions */
#include <TinyGSM.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqttClient(client);

uint8_t Initialize_GSM()
{
  Serial.println("\n\n----------------------------------");
  Serial.println("[DEBUG] > Entering Initialize_GSM...");
  
  // 1. BASIC AT CHECK
  // Send a raw "AT" command to see if there is ANY life on the serial line
  Serial.println("[DEBUG] > Testing Raw AT Command...");
  modem.sendAT(""); 
  if (modem.waitResponse(1000) == 1) {
    Serial.println("[DEBUG] > AT Command: OK (Modem is listening)");
  } else {
    Serial.println("[DEBUG] > AT Command: TIMEOUT (Check wiring/power/baudrate!)");
  }

  // 2. RESTART
  Serial.println("[DEBUG] > Calling modem.restart()...");
  modem.restart();
  Serial.println("[DEBUG] > modem.restart() returned.");

  // 3. MODEM INFO
  Serial.println("[DEBUG] > Fetching Modem Info...");
  String info = modem.getModemInfo();
  Serial.print("[DEBUG] > Modem Info: "); Serial.println(info);

  // 4. SIM STATUS
  // 0=ERROR, 1=READY, 2=LOCKED, 3=ANTITHEFT, 4=UNKNOWN
  Serial.println("[DEBUG] > Checking SIM Status...");
  int simStatus = modem.getSimStatus();
  Serial.print("[DEBUG] > SIM Status (0=Fail, 1=Ready): "); Serial.println(simStatus);

  // 5. BATTERY CHECK
  // Vital for GPRS. Should be > 3800mV
  Serial.println("[DEBUG] > Checking Battery...");
  int battV = modem.getBattVoltage();
  Serial.print("[DEBUG] > Battery Voltage: "); Serial.print(battV); Serial.println(" mV");

  // Unlock SIM if needed
  if (strlen(simPIN) && simStatus != SIM_READY) {
    Serial.println("[DEBUG] > Unlocking SIM...");
    modem.simUnlock(simPIN);
  }

  // 6. NETWORK REGISTRATION
  Serial.println("[DEBUG] > Waiting for network (15s timeout)...");
  // Print signal quality while waiting
  int csq = modem.getSignalQuality();
  Serial.print("[DEBUG] > Current Signal Quality (CSQ): "); Serial.println(csq);

  if (!modem.waitForNetwork(15000L)) {
    Serial.println("[DEBUG] > waitForNetwork: FAIL");
    
    // Detailed diag
    Serial.print("[DEBUG] > Registration Status: "); 
    Serial.println(modem.getRegistrationStatus());
    
    return (uint8_t)ERROR_CONECTION;
  }
  Serial.println("[DEBUG] > waitForNetwork: OK");

  if (modem.isNetworkConnected()) {
    Serial.println("[DEBUG] > Network is connected.");
  } else {
    Serial.println("[DEBUG] > Network IS NOT connected (logical check).");
  }

  // 7. GPRS CONNECTION
  Serial.print(F("[DEBUG] > Connecting to APN: "));
  Serial.println(apn);
  
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println("[DEBUG] > gprsConnect: FAIL");
    return (uint8_t)ERROR_CONECTION;
  }
  Serial.println("[DEBUG] > gprsConnect: OK");
  Serial.print("[DEBUG] > IP Address: "); Serial.println(modem.localIP());

  // 8. MQTT SETUP
  Serial.println("[DEBUG] > Setting up MQTT Client...");
  mqttClient.setServer(node_server, PORT);
  mqttClient.setBufferSize(MAX_GPRS_BUFFER - 1);

  setup_GSM_tic();
  Serial.println("[DEBUG] > Initialize_GSM Completed Successfully.");
  Serial.println("----------------------------------\n");

  return (uint8_t)CONNECTED;
}

void gsmCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("[DEBUG] > MQTT Msg arrived [");
  Serial.print(topic);
  Serial.print("] ");

  memset(payload_char, 0, sizeof(payload_char));

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    payload_char[i] = (char)payload[i];
  }
  Serial.println();
}

boolean Check_mqtt_client_conection()
{
  bool conn = mqttClient.connected();
  // Optional: print only on change or rarely to avoid spam
  // if(!conn) Serial.println("[DEBUG] > MQTT Disconnected!");
  return conn;
}

void gsmReconnect(uint8_t &_try_reconect)
{
  int count = 0;
  Serial.println("[DEBUG] > Connecting to MQTT Broker...");
  
  // Check if GPRS is still alive before trying MQTT
  if (!modem.isGprsConnected()) {
      Serial.println("[DEBUG] > GPRS lost! Reconnecting GPRS...");
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
          Serial.println("[DEBUG] > GPRS Reconnect Failed.");
          _try_reconect = DISCONNECTED;
          return;
      }
  }

  while (!mqttClient.connected() && count < 3)
  {
    count++;
    Serial.print("[DEBUG] > MQTT Attempt "); Serial.print(count); Serial.println("/3");
    
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), "manguebaja", "Rolabosta1417", "/esp-connected", 2, true, "Offline", true))
    {
      Serial.println("[DEBUG] > MQTT Connected!");
      sprintf(msg, "%s", "Online");
      mqttClient.publish("/esp-connected", msg);
      memset(msg, 0, sizeof(msg));

      _try_reconect = CONNECTED; 

      mqttClient.subscribe("/esp-test");
    } else {
      Serial.print("[DEBUG] > MQTT Failed. State: ");
      Serial.println(mqttClient.state()); // Print standard MQTT error code
      delay(2000); 
    }
  }
  
  if (!mqttClient.connected()) {
     Serial.println("[DEBUG] > Gave up on MQTT reconnect.");
     _try_reconect = DISCONNECTED;
  }
}

void Send_msg_MQTT()
{
  mqtt_packet_t recv = update_packet();
  publishPacket(&recv, sizeof(recv));
  mqttClient.loop();
}

void publishPacket(void *T, uint32_t len)
{
  if (volatile_position + len > MSG_BUFFER_SIZE)
  {
    volatile_position = 0;
    Serial.println("[DEBUG] > Buffer reset (overflow protection)");
  }

  if (buff)
  {
    memcpy(&volatile_bytes[volatile_position], (uint8_t *)T, len);
    volatile_position += len;
    buff = false;
  }

  if (sendFlag)
  {
    if (mqttClient.connected()) {
        // Serial.println("[DEBUG] > Publishing data...");
        mqttClient.publish("/logging", volatile_bytes, MSG_BUFFER_SIZE);
    } else {
        Serial.println("[DEBUG] > Skip publish: MQTT not connected");
    }
    sendFlag = false;
  }
}

/* Ticker functions */
Ticker ticker1Hz, ticker20Hz;

void setup_GSM_tic()
{
  ticker1Hz.attach(1.0f, ticker1HzISR);
  ticker20Hz.attach(0.05f, ticker20HzISR);
}

void ticker1HzISR()
{
  sendFlag = true;
}

void ticker20HzISR()
{
  buff = true;
}
