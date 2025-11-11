#include "LORA.h"

/* Radio LoRa tool */
EBYTE LoRa(&LoRaUART, PIN_M0, PIN_M1, PIN_AUX);

uint8_t LORA_init()
{
    uint8_t res = FAIL_RESPONSE;

    LoRaUART.begin(LoRa_Baud_Rate);

    if (LoRa.init())
        res = SUCESS_RESPONSE;
    else
        res = FAIL_RESPONSE;

    LoRa.SetAddressH(1);
    LoRa.SetAddressL(1);
    LoRa.SetChannel(11);
    LoRa.SetAirDataRate(ADR_1200);
    LoRa.SetTransmitPower(OPT_TP30);
    LoRa.SetMode(EBYTE_MODE_NORMAL);
    LoRa.SetUARTBaudRate(UDR_9600);
    LoRa.SetFECMode(OPT_FECENABLE);
    LoRa.SaveParameters(PERMANENT);
    LoRa.PrintParameters();

    return res;
}

bool LORA_SendStruct(void *TheStructure, uint16_t size_)
{
    Serial.println("Send lora packet");
    return LoRa.SendStruct(TheStructure, size_);
}
