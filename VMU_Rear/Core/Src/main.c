#include "main.h"
#include "adc.h"
#include "can.h"
#include "i2c.h"
#include "gpio.h"

/* Inclusão da Máquina de Estados */
#include "state_machine.h"

/* Variáveis Globais Essenciais */
volatile uint32_t contador_pulsos_indutivo = 0;
tcu_state_t ecu_state;

void SystemClock_Config(void);

int main(void) {

    /* Configuração padrão do STM32CubeIDE */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_CAN_Init();
    MX_I2C1_Init();
    MX_ADC1_Init();

    // Inicia e constrói a FSM da TCU
    StateMachine_Init(&ecu_state);

    while (1) {
        // Tudo acontece aqui sem nenhum HAL_Delay() travando a MCU
        StateMachine_Update(&ecu_state);
    }
}

/**
 * Interrupção do Sensor Indutivo
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_1) { // Pino PB1 (Sensor Indutivo)
        contador_pulsos_indutivo++;
    }
}

void SystemClock_Config(void) {
    // Configurações do Clock geradas pelo seu CubeMX (Mantidas do seu main anterior)
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* Callback chamado automaticamente pelo hardware quando chega um pacote CAN */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];

    // Apenas ler a mensagem da FIFO é suficiente para o hardware
    // limpar a flag de interrupção e parar de travar a placa.
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK) {
        // Futuramente, se a TCU precisar receber dados (ex: Dashboard),
        // você coloca os seus "Ifs" de leitura aqui!
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}
