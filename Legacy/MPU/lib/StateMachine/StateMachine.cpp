#include "StateMachine.h"

// if you wanna to push the DEBUG_ST, uncomment this define
//#define DEBUG

CircularBuffer<state_t, BUFFER_SIZE> state_buffer;
TinyGPSPlus gps;
Ticker ticker1Hz, ticker250mHz;

radio_packet_t volatile_packet;
state_t current_state = IDLE_ST;
bool buffer_full = false, g = false;

void CircularBuffer_Ticker_Init()
{
    memset(&volatile_packet, 0, sizeof(radio_packet_t));
    ticker1Hz.attach(1.0, ticker1HzISR);
    ticker250mHz.attach(4.0, ticker250mHzISR);
}

void CircularBuffer_CurrentState()
{
    if (state_buffer.isFull())
    {
        // buffer_full = true;
        current_state = state_buffer.pop();
    } else {
        // buffer_full = false;
        if (!state_buffer.isEmpty())
            current_state = state_buffer.pop();
        else
            current_state = IDLE_ST;
    }

    switch (current_state)
    {
    case IDLE_ST:
        //Serial.println("i");
        break;

    case RADIO_ST:
        { // <--- ADDED SCOPE BRACE TO FIX COMPILATION ERROR
            // --- MODIFICATION START ---
            // 1. Preserve the latest GPS and timestamp data
            // This prevents CAN data from overwriting the locally retrieved GPS values.
            double temp_lat = volatile_packet.latitude;
            double temp_lng = volatile_packet.longitude;
            uint32_t temp_timestamp = volatile_packet.timestamp;

            // 2. Get the latest complete packet from the CAN ISR (overwrites GPS/timestamp with 0.0)
            volatile_packet = get_radio_packet();

            // 3. Restore the latest GPS and timestamp data
            volatile_packet.latitude = temp_lat;
            volatile_packet.longitude = temp_lng;
            volatile_packet.timestamp = temp_timestamp;
            // --- MODIFICATION END ---

            // --- Print packet data before sending ---
            Serial.println("--- Sending LoRa Packet ---");
            Serial.printf("Timestamp: %lu\n", volatile_packet.timestamp);
            Serial.printf("Latitude: %f\n", volatile_packet.latitude);
            Serial.printf("Longitude: %f\n", volatile_packet.longitude);
            Serial.printf("Speed: %d\n", volatile_packet.speed);
            Serial.printf("RPM: %d\n", volatile_packet.rpm);
            Serial.printf("CVT Temp: %d\n", volatile_packet.cvt);
            Serial.printf("Motor Temp: %d\n", volatile_packet.temperature);
            Serial.printf("Voltage: %f\n", volatile_packet.volt);
            Serial.printf("SOC: %d\n", volatile_packet.SOC);
            Serial.printf("IMU Accel X: %d\n", volatile_packet.imu_acc.acc_x);
            Serial.printf("IMU Accel Y: %d\n", volatile_packet.imu_acc.acc_y);
            Serial.printf("IMU Accel Z: %d\n", volatile_packet.imu_acc.acc_z);
            Serial.printf("IMU Gyro X: %d\n", volatile_packet.imu_dps.dps_x);
            Serial.printf("IMU Gyro Y: %d\n", volatile_packet.imu_dps.dps_y);
            Serial.printf("IMU Gyro Z: %d\n", volatile_packet.imu_dps.dps_z);
            Serial.println("---------------------------\n");

            LORA_SendStruct(&volatile_packet, sizeof(volatile_packet));
        } // <--- ADDED SCOPE BRACE
        break;

    case GPS_ST:
        // Process incoming GPS data (existing logic)
        while (GPS_uart.available() > 0)
        {
            if (gps.encode(GPS_uart.read()))
                g = get_GPS_data();
        }

        // --- Integrated GPS Status Logging Logic ---
        // 'g' and 'volatile_packet' are updated in get_GPS_data()
        if (g && gps.location.age() < 2000) {
            Serial.println("===== GPS SINCRONIZADO ✅ =====");
            Serial.printf("Latitude : %.6f\n", volatile_packet.latitude);
            Serial.printf("Longitude: %.6f\n", volatile_packet.longitude);
            Serial.printf("Satélites: %d\n", gps.satellites.value());
            Serial.printf("Precisão (HDOP): %.2f\n", gps.hdop.hdop());
            Serial.printf("Altitude : %.2f m\n", gps.altitude.meters());

            // Mostra hora e data do GPS (UTC)
            if (gps.time.isValid() && gps.date.isValid()) {
                Serial.print("Hora (UTC): ");
                Serial.printf("%02d:%02d:%02d\n", gps.time.hour(), gps.time.minute(), gps.time.second());
                Serial.print("Data (UTC): ");
                Serial.printf("%02d/%02d/%d\n", gps.date.day(), gps.date.month(), gps.date.year());
            }
            Serial.println("===============================\n");
        } else {
            // No fix - print status (note: this state runs every 4 seconds, acting as its own rate limiter)
            Serial.println("⏳ Aguardando sincronização GPS... (sem fix ainda)");
            Serial.print("Satélites detectados: ");
            Serial.println(gps.satellites.value());
            Serial.println();
        }
        // -------------------------------------------

        /* Send latitude message */
        if (Send_GPS_data(volatile_packet.latitude, LAT_ID))
        {
            /* Send longitude message if latitude is successful */
            Send_GPS_data(volatile_packet.longitude, LNG_ID);
        }

        break;

    case DEBUG_ST:
        Serial.println("Debug state");
        Serial.printf("Latitude (LAT) = %lf\r\n", volatile_packet.latitude);
        Serial.printf("Longitude (LNG) = %lf\r\n", volatile_packet.longitude);

        PRINT_GPS_TIME();
        PRINT_GPS_DATE();

        Serial.println("\n\n");
        break;
    }
}

bool get_GPS_data()
{

    if (gps.location.isValid())
    {
        volatile_packet.latitude = gps.location.lat();
        volatile_packet.longitude = gps.location.lng();

        return true;
    }

    //else
    //{
    //    volatile_packet.latitude = 0;
    //    volatile_packet.longitude = 0;

        return false;
    //}
}

/* Tickers */
void ticker250mHzISR(void)
{
    state_buffer.push(GPS_ST);
}

void ticker1HzISR()
{
    state_buffer.push(RADIO_ST);

    #ifdef DEBUG
        state_buffer.push(DEBUG_ST);
    #endif
}

// DEBUG FUNCTIONS
void PRINT_GPS_TIME(void)
{
    // Time and Data in point 0 of the map
    (gps.time.isValid() ? Serial.printf("Time: %dh:%dm:%ds\r\n", gps.time.hour(), \
        gps.time.minute(), gps.time.second()) : Serial.println("Time: 0h:0m:0s\r\n"));
}

void PRINT_GPS_DATE(void)
{
    (gps.date.isValid() ? Serial.printf("Date: %d/%d/%d\r\n", gps.date.day(), gps.date.month(), \
        gps.date.year()) : Serial.println("Date: 0/0/0\r\n"));
}
