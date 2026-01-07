#include <Arduino.h>
#include <CAN.h>
#include <ota.h>
#include <SD_state_machine.h>
#include <CON_state_machine.h>

/* Task Management */
TaskHandle_t SDlogging = NULL, ConectivityState = NULL, BLE_RESQUEST_State = NULL;

/* SD status variables */
uint8_t _sd = FAIL_RESPONSE;       // flag to check if SD module compile
uint8_t sd_status = FAIL_RESPONSE; // flag to check if SD module still working

/* State Of Telemetry (SOT) variables */
uint8_t _sot = DISCONNECTED;

/* Struct for BLE debug */
bluetooth bluetooth_packet;

/* States Machines */
void SdStateMachine(void *pvParameters);
void ConnStateMachine(void *pvParameters);
void BLE_RESQUEST_StateMachine(void *pvParameters);

#define BLINK_PIN EMBEDDED_LED

void setup()
{
    Serial.begin(115200);
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

    /* SPIFFS Initialize */
    if(!SPIFFS.begin(true)){
        Serial.println("Ocorreu um erro ao montar o SPIFFS");
        return;
    }

    /* Hardware and Interrupt Config */
    pinMode(EMBEDDED_LED, OUTPUT);
    pinMode(DEBUG_LED, OUTPUT);

    /* CAN-BUS Initialize */
    if (!CAN_start_device())
        esp_restart();

    memset(&bluetooth_packet, 1, sizeof(bluetooth));

    setupnetwork();
    setupServerRoutes();
    pinMode(BLINK_PIN, OUTPUT);

    /* Tasks */
    // This state machine is responsible for the Basic CAN logging
    xTaskCreatePinnedToCore(SdStateMachine, "SDStateMachine", 4096, NULL, 5, &SDlogging, 0);
    // This state machine is responsible for send to the MPU the SCU debug data
    xTaskCreatePinnedToCore(BLE_RESQUEST_StateMachine, "BLE_RESQUEST_StateMachine", 4096, NULL, 3, &BLE_RESQUEST_State, 0);
    // This state machine is responsible for the GPRS connection
    xTaskCreatePinnedToCore(ConnStateMachine, "ConnectivityStateMachine", 4096, NULL, 5, &ConectivityState, 1);

}

void loop() {
    handleServerClient();

    digitalWrite(BLINK_PIN, HIGH);
    delay(100);
    digitalWrite(BLINK_PIN, LOW);
    delay(100);
    digitalWrite(BLINK_PIN, HIGH);
    delay(100);
    digitalWrite(BLINK_PIN, LOW);

    delay(1500);
}

void SdStateMachine(void *pvParameters){
    /*
       This function applies the bluetooth debugger project for the SD card.

       For further information read:
       */
    bluetooth_packet.sd_start = _sd = start_SD_device();

    while (1) {
        bluetooth_packet.check_sd = Check_SD_for_storage();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1);
}

void BLE_RESQUEST_StateMachine(void *pvParameters) {
    /*
       This function applies the bluetooth debugger project for the SD card.

       For further information read:
       */
    while (1) {
        if (MPU_request_Debug_data())
            Send_SCU_FLAGS(bluetooth_packet);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1);
}

void ConnStateMachine(void *pvParameters){
    /* 
       This function handles the bluetooth debugger output and the connection
       between the esp and the MQTT server.
       */
    _sot = Initialize_GSM();

    if (_sot == ERROR_CONECTION)
    { // enable the error bit
        Send_SOT_msg(_sot);
        bluetooth_packet.internet_modem = FAIL_RESPONSE;
        vTaskDelay(DELAY_ERROR(_sot));
    }

    else
    {
        bluetooth_packet.internet_modem = SUCESS_RESPONSE;
    }

    Send_SOT_msg(_sot);

    while (1){
        if (!Check_mqtt_client_conection())
        {
            bluetooth_packet.mqtt_client_connection = FAIL_RESPONSE;
            _sot == CONNECTED ? _sot = DISCONNECTED : 0; // disable online flag
            Send_SOT_msg(_sot);
            gsmReconnect(_sot);
            Send_SOT_msg(_sot);
        }

        else
        {
            bluetooth_packet.mqtt_client_connection = SUCESS_RESPONSE;
        }

        Send_msg_MQTT();

        vTaskDelay(1);
    }

    vTaskDelay(1);
}
