// MPU_NACIONAL_2025-Transmitter/receiver.cpp
#include <Arduino.h>
#include <EBYTE.h>
#include <cmath> // Include for math functions used in simulation
#include "hard_defs_rec.h" // Assuming .h or .hh, corrected to .h for commonality
#include "packets.h"

#define OPT_TP30 0b00 // 30 db

// --- Simulation Flag ---
// Set to 1 to enable data simulation, 0 to use real LoRa data
#define SIMULATE_DATA 0

EBYTE Lora(&LoRaUART, PIN_M0, PIN_M1, PIN_AUX);
radio_packet_t volatile_packet;
uint32_t packet_counter = 0; // Counter for simulated packets

// --- The Start Marker ---
// This must be EXACTLY the same as in your server's telemetry_serial.py
const uint8_t start_marker[] = {0xAA, 0xBB, 0xCC, 0xDD};

// --- Data Simulation Function (from serial_sim.cpp) ---
void populate_packet(radio_packet_t& pkt, uint32_t counter) {
    pkt.volt = 12.5f + sin(counter * 0.1) * 0.5f;
    pkt.SOC = 98 - (counter % 20);
    pkt.cvt = 80 + (int)(sin(counter * 0.2) * 5);
    pkt.current = 15.3f + cos(counter * 0.1) * 2.0f;
    pkt.temperature = 75 + (int)(cos(counter * 0.3) * 3);
    pkt.speed = (counter * 2) % 60;

    pkt.imu_acc = {
        (int16_t)(sin(counter * 0.5) * 100),
        (int16_t)(cos(counter * 0.5) * 100),
        (int16_t)(980 + sin(counter * 0.2) * 10)
    };

    pkt.imu_dps = {
        (int16_t)(cos(counter * 0.4) * 50),
        (int16_t)(sin(counter * 0.4) * 50),
        (int16_t)(cos(counter * 0.1) * 5)
    };

    pkt.Angle = {(int16_t)(sin(counter * 0.1) * 20), (int16_t)(cos(counter * 0.1) * 10)};
    pkt.rpm = 3000 + (uint16_t)(sin(counter * 0.8) * 500);
    pkt.flags = counter % 2;

    pkt.latitude = -8.05428 + sin(counter * 0.01) * 0.001;
    pkt.longitude = -34.8813 + cos(counter * 0.01) * 0.001;

    pkt.timestamp = millis();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing LoRa\n");
    LoRaUART.begin(LoRa_Baud_Rate);
    pinMode(LED_BUILTIN, OUTPUT);

#if SIMULATE_DATA == 0
    // Only initialize LoRa if not in simulation mode
    if (!Lora.init()) {
        Serial.println("LoRa INIT FAILED");
        while (1);
    }
    
    // Configure LoRa parameters
    Lora.SetAddressH(1);
    Lora.SetAddressL(1);
    Lora.SetChannel(11);
    Lora.SetAirDataRate(ADR_2400);
    Lora.SetTransmitPower(OPT_TP30);
    Lora.SetMode(EBYTE_MODE_NORMAL);
    Lora.SetUARTBaudRate(UDR_9600);
    Lora.SetFECMode(OPT_FECENABLE);
    Lora.SaveParameters(PERMANENT);
    Lora.SetUARTBaudRate(UDR_9600);
    
    Serial.println("--- LoRa Receiver Ready ---");
#else
    Serial.println("--- Starting Data Simulation Mode ---");
#endif
}

void loop() {
#if SIMULATE_DATA == 1
    // --- SIMULATION MODE ---
    digitalWrite(LED_BUILTIN, HIGH);

    populate_packet(volatile_packet, packet_counter);

    // 1. Send the start marker
    Serial.write(start_marker, sizeof(start_marker));

    // 2. Send the raw binary data of the struct
    Serial.write((uint8_t*)&volatile_packet, sizeof(volatile_packet));

    digitalWrite(LED_BUILTIN, LOW);

    packet_counter++;
    delay(50);

#else
    // --- NORMAL LORA RECEIVER MODE ---
    if (LoRaUART.available()) {
        digitalWrite(LED_BUILTIN, HIGH);

        Serial.println("LoRa is available");
        if (Lora.GetStruct(&volatile_packet, sizeof(volatile_packet))) {
            
            // 1. Send the start marker
            Serial.write(start_marker, sizeof(start_marker));

            // 2. Send the raw binary data of the struct
            Serial.write((uint8_t*)&volatile_packet, sizeof(volatile_packet));
            Serial.println("Receiving LoRa data");
        }
        
        digitalWrite(LED_BUILTIN, LOW);
    }
#endif
}
