 /*
 ******************************************************************************
 * @file
 * - main.c
 *
 * @brief
 * - Firmware for a baja-SAE ECU that acquires engine RPM and vehicle roll and pitch
 *   via a otpocoupler circuit and LSM6DS3 IMU sensor. The data is sent through CAN
 *   network to other ECU's that store and broadcast data.
 ******************************************************************************
 */

/* Includes */

/* Standard libraries and STM32CubeIDE includes */
#include "../Inc/main.h"
#include "can.h"
#include "i2c.h"
#include "gpio.h"
#include <math.h> // Necessary for IMU calculations (atan2, sqrt)

/* User includes */
#include "rpm.h"
#include "../Inc/state_machine.h"
#include "kalman.h"
#include "lsm6ds3.h"

/* Private Variables */

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// CAN Transmission variables
CAN_TxHeaderTypeDef TxHeader;
uint32_t TxMailbox;

// CAN Reception variables
CAN_RxHeaderTypeDef RxHeader;
uint8_t RxData[8];

// Statemachine super variable
vmu_state_t ecu_state;

/* Private function prototyples */

void SystemClock_Config(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan);
void Error_Handler(void);

/* Main function and state machine superloop */

int main(void) {

    /* Initializing HAL and microcontroler features */

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_CAN_Init();
    MX_I2C1_Init();

    /* Initializing statemachine variables */

    StateMachine_Init(&ecu_state); // Inicia a FSM

    while (1) {
        /* Please read state_machine.c */
        StateMachine_Update(&ecu_state);
    }
}

/* Default Function Definitions */

/**
 * @brief System Clock Configuration (Only using HSE at 72MHz)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
        |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

 /*
  * Interruption Callback (EXTI)
  * Callback for optocoupler RPM circuit
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_4) // Assumindo PB4 como no código anterior
    {
        RPM_EXTI_Callback();
    }
}

/*
 * CAN RX callbacks, mostly used for flags such as LOW_BATTERY_MODE
 */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK)
    {
        /* Later check for low_battery flag and debug flag */
    }
}

/*
 * Hard errors just blink the board's LED
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        for(volatile int i=0; i<100000; i++);
    }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */

