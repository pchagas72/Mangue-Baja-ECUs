/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : VMU_Center - ECU that integrates IMU data + optocoupler
 * to a baja-SAE vehicle CAN network.
 ******************************************************************************
 */

/* Default includes */
/* All made by stm32cubeIDE */
#include "../Inc/main.h"
#include "can.h"
#include "i2c.h"
#include "gpio.h"

/* User includes */
#include "../Inc/state_machine.h"
#include "rpm.h"

/* Private Variables */

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

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_CAN_Init();
    MX_I2C1_Init();

    StateMachine_Init(&ecu_state); // Inicia a FSM

    while (1) {
        StateMachine_Update(&ecu_state); // O coração da ECU bate aqui
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

/* User Function Definitions */

// === 1. Callback da Interrupção (EXTI) ===
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_4)
    {
        RPM_EXTI_Callback();
    }
}

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

