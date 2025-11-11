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

//unsigned long timer;

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

  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();
  // Or, use modem.init() if you don't need the complete restart

  Serial.print("Modem: "); Serial.println(modem.getModemInfo());

  Serial.print("Status: "); Serial.println(modem.getSimStatus());

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3)
  {
    modem.simUnlock(simPIN);
  }

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork(15000L))
  {
    Serial.println("fail");
    return (uint8_t)ERROR_CONECTION;
  }
  Serial.println("OK");

  if (modem.isNetworkConnected())
  {
    Serial.println("Network connected");
  }

  Serial.print(F("Connecting to APN: "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass))
  {
    Serial.println(" fail");
    return (uint8_t)ERROR_CONECTION;
  }
  Serial.println("OK");

  mqttClient.setServer(node_server, PORT);
  // mqttClient.setCallback(gsmCallback);
  mqttClient.setBufferSize(MAX_GPRS_BUFFER - 1);

  setup_GSM_tic();

  return (uint8_t)CONNECTED;
}

void gsmCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
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
  return mqttClient.connected();
}

void gsmReconnect(uint8_t &_try_reconect)
{
  int count = 0;
  Serial.println("Conecting to MQTT Broker...");
  while (!mqttClient.connected() && count < 3)
  {
    count++;
    Serial.println("Reconecting to MQTT Broker..");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), "manguebaja", "Rolabosta1417", "/esp-connected", 2, true, "Offline", true))
    {
      sprintf(msg, "%s", "Online");
      mqttClient.publish("/esp-connected", msg);
      memset(msg, 0, sizeof(msg));
      Serial.println("Connected.");

      _try_reconect = CONNECTED; // enable online flag

      /* Subscribe to topics */
      mqttClient.subscribe("/esp-test");
      //digitalWrite(LED_BUILTIN, HIGH);

    } else {
      Serial.print("Failed with state");
      Serial.println(mqttClient.state());
      
      delay(2000); 
      _try_reconect = DISCONNECTED; // disable online flag 
    }
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
  /*
    Send the message using JSON example:
      * 1 - StaticJsonDocument<305> doc;
      * 2 - doc["data"] = data;
      * 3 - memset(msg, 0, sizeof(msg));
      * 4 - serializeJson(doc, msg);
      * 5 - mqttClient.publish("/logging", msg)
  */

  if (volatile_position + len > MSG_BUFFER_SIZE)
  {
    // Handle the case when the array is full, for example by resetting the current position to the beginning.
    volatile_position = 0;
  }

  if (buff)
  {
    memcpy(&volatile_bytes[volatile_position], (uint8_t *)T, len);

    volatile_position += len;
    buff = false;
  }

  if (sendFlag)
  {
    mqttClient.publish("/logging", volatile_bytes, MSG_BUFFER_SIZE);
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
