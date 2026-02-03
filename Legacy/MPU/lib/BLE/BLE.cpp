#include "BLE.h"

/* Defines for debug */
//#define BLEdebug
//#define PrintJSON

std::string msgBLE = "";
bool deviceConnected = false, oldDeviceConnected = false, App_data_request = false;
BLEServer *pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *pCharacteristic = NULL;

/* Start the BLE host connection */
void Init_BLE_Server()
{
    // Create the BLE Device
    BLEDevice::init("MangueBaja_Debug");

    // Set maximum MTU (512 bytes)
    BLEDevice::setMTU(MAX_BLE_LENGTH);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_WRITE  |
        BLECharacteristic::PROPERTY_READ
    );

    // Create a BLE Descriptor
    // pDescr_1 = new BLEDescriptor((uint16_t)0x2901);
    // pDescr_1->setValue("A very interesting variable");
    // pCharacteristic_1->addDescriptor(pDescr_1);

    // Add the BLE2902 Descriptor because we are using "PROPERTY_NOTIFY"
    // pBLE2902_1 = new BLE2902();
    // pBLE2902_1->setNotifications(true);
    // pCharacteristic_1->addDescriptor(pBLE2902_1);

    // pBLE2902_2 = new BLE2902();
    // pBLE2902_2->setNotifications(true);
    // pCharacteristic_2->addDescriptor(pBLE2902_2);

    // add callback functions here:
    pCharacteristic->setCallbacks(new CharacteristicCallbacks());

    // Start the service
    pCharacteristic->setValue(" ");
    pService->start();

    // Start advertising
    // pinMode(BLE_DEBUG_LED, OUTPUT);
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    #ifdef BLEdebug
        Serial.println("Waiting a client connection to notify...");
    #endif
}

/* If there is any device connected to BLE @return true */
bool BLE_connected()
{
    if (deviceConnected)
        oldDeviceConnected = true;

    // disconnecting
    if (!deviceConnected && oldDeviceConnected)
    {
        // digitalWrite(BLE_DEBUG_LED, LOW);
        pServer->startAdvertising(); // restart advertising
        #ifdef BLEdebug
            Serial.println("start advertising");
        #endif
        oldDeviceConnected = deviceConnected;
    }

    // connecting
    if (deviceConnected && !oldDeviceConnected)
        oldDeviceConnected = deviceConnected;

    return deviceConnected;
}

bool App_MPU_data_request()
{
    bool t = App_data_request;
    App_data_request = false;
    return t;
}

void Send_BLE_msg()
{
    bluetooth pkt = update_packet();
    JsonDocument doc;

    /* Make the JSON packet in the std::string format */
    Make_JSON_packet(doc, &pkt);
    msgBLE.clear();
    serializeJson(doc, msgBLE);

    /* Print data and length of the std::string */
    #ifdef PrintJSON
        Serial.print("JSON document Size: ");
        Serial.println(doc.size());
        Serial.println(msgBLE.data());
        Serial.print("JSON in std::string size: ");
        Serial.println(msgBLE.length());
        Serial.println();
    #endif

    /* Set and send the value */
    pCharacteristic->setValue(msgBLE);
    pCharacteristic->notify();
}

void Make_JSON_packet(JsonDocument &JSON, bluetooth *msg_packet)
{
    JSON["REQUEST"] = String(millis());

    JSON["MPU"]["LORA_INIT"] = String(msg_packet->lora_init);

    JSON["MMI"]["ACCELEROMETER_BEGIN"] = String(msg_packet->accel_begin);

    JSON["TCU"]["TERMISTOR"] = String(msg_packet->termistor);
    JSON["TCU"]["CVT_TEMPERATURE"] = String(msg_packet->cvt_temperature);
    JSON["TCU"]["VOLT"] = String(msg_packet->measure_volt);
    JSON["TCU"]["SPEED"] = String(msg_packet->speed_pulse_counter);
    JSON["TCU"]["SERVO_STATE"] = String((msg_packet->servo_state == 4 ? "CHOKE" : msg_packet->servo_state == 3 ? "MID"
                                                                                : msg_packet->servo_state == 2 ? "RUN"
                                                                                                               : "ERRO"));

    JSON["SCU"]["INTERNET_MODEM"] = String(msg_packet->internet_modem);
    JSON["SCU"]["MQTT_CONNECTION"] = String(msg_packet->mqtt_client_connection);
    JSON["SCU"]["SD_START"] = String(msg_packet->sd_start);
    JSON["SCU"]["SD_STATUS"] = String(msg_packet->check_sd);
}

/* Callback when the device connects to BLE */
void ServerCallbacks::onConnect(BLEServer *pServer)
{
    #ifdef BLEdebug
        Serial.println("Client connected");
    #endif
    deviceConnected = true;
}

/* Callback when the device disconnects to BLE */
void ServerCallbacks::onDisconnect(BLEServer *pServer)
{
    #ifdef BLEdebug
        Serial.println("Disconnected");
    #endif
    deviceConnected = false;
}

void CharacteristicCallbacks::onWrite(BLECharacteristic *SenderCharacteristic)
{
    std::string value = SenderCharacteristic->getValue();

    if (SenderCharacteristic->getLength() > 0)
    {
        for (int i = 0; i < SenderCharacteristic->getLength(); i++)
            value[i] = toupper(value[i]);

        if (value.compare("MB") == 0)
            App_data_request = true;
        else
            App_data_request = false;
    }
}
