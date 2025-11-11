/**
 * @file transmitter.cpp
 * @brief Main application file for the MPU_NACIONAL_2025-Transmitter.
 * * This file contains the main setup and loop functions, with a focus on
 * reliable LoRa packet transmission.
 */

#include <Arduino.h>
#include <StateMachine.h>
#include <CAN.h>

/* Task Management */
TaskHandle_t StateMachine_Task = NULL;

// Flag to store the initialization status of the LoRa module.
uint8_t status = FAIL_RESPONSE;

/* Task Function Prototypes */
void StateMachineTask(void *pvParameters);

/**
 * @brief Setup function, runs once on startup.
 * * Initializes serial communication, GPIO pins, CAN bus, LoRa module,
 * and creates the state machine task. BLE has been removed to prioritize
 * LoRa transmission and reduce power consumption.
 */
void setup()
{
  // Initialize serial communication for debugging.
  Serial.begin(115200);

  // Initialize the UART for the GPS module.
  GPS_uart.begin(GPS_Baud_Rate, SERIAL_8N1, GPS_TX, GPS_RX);

  // Set the onboard LED pin as an output.
  pinMode(EMBEDDED_LED, OUTPUT);

  // Initialize the CAN bus. If it fails, restart the ESP32.
  if (!CAN_start_device()) {
    esp_restart();
  }

  // Initialize the LoRa module and store the result in the status flag.
  status = LORA_init();

  // Initialize the circular buffer and tickers for the state machine.
  CircularBuffer_Ticker_Init();

  /* Create FreeRTOS Tasks */
  // Task for managing the state machine and circular buffer.
  xTaskCreatePinnedToCore(
    StateMachineTask,   // Task function
    "StateMachineTask", // Name of the task
    4096,               // Stack size in words
    NULL,               // Task input parameter
    5,                  // Priority of the task
    &StateMachine_Task, // Task handle to keep track of created task
    0                   // Core where the task should run (Core 0)
  );
}

/**
 * @brief Main loop function.
 * * This function is empty because the application's logic is handled by FreeRTOS tasks.
 */
void loop() {
  // The loop is empty as all functionality is handled by the created tasks.
}

/**
 * @brief Task to manage the state machine.
 * * This task runs in a continuous loop, processing the current state from the
 * circular buffer, focusing on handling CAN data and LoRa transmissions.
 * * @param pvParameters Not used.
 */
void StateMachineTask(void *pvParameters)
{
  while (1)
  {
    // Process the current state from the state machine's circular buffer.
    CircularBuffer_CurrentState();

    // Delay for 1ms to allow other tasks to run.
    vTaskDelay(1);
  }
}
