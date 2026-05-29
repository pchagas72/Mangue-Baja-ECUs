/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : PURE CAN LOOPBACK TEST - Nova Placa
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
CAN_TxHeaderTypeDef TxHeader;
uint32_t TxMailbox;

// Variáveis de Recepção CAN
CAN_RxHeaderTypeDef RxHeader;
uint8_t RxData[8];

// Variável de teste para envio
uint8_t dado_teste = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock to 72 MHz (HSE 8MHz) */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init(); // Certifique-se de que no can.c está CAN_MODE_LOOPBACK

  // MX_I2C1_Init(); <-- Removido para o teste puro de CAN

  /* USER CODE BEGIN 2 */

  // Configuração do filtro CAN "passa-tudo" direcionado ao FIFO1
  CAN_FilterTypeDef canfilterconfig;
  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
  canfilterconfig.FilterBank = 0;
  canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
  canfilterconfig.FilterIdHigh = 0x0000;
  canfilterconfig.FilterIdLow = 0x0000;
  canfilterconfig.FilterMaskIdHigh = 0x0000;
  canfilterconfig.FilterMaskIdLow = 0x0000;
  canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;

  // Aplica o filtro, ativa interrupção RX e dá a partida na CAN
  HAL_CAN_ConfigFilter(&hcan, &canfilterconfig);
  HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
  HAL_CAN_Start(&hcan);

  // Garante que o LED PC13 comece apagado (Lógica Invertida: SET = OFF)
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  	 for (int i = 0; i < 6; i++){
  		 HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  		 HAL_Delay(100);
  	 }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // PREPARA O PACOTE CAN DE TESTE
      TxHeader.StdId = 0x7FF; // ID de teste genérico
      TxHeader.ExtId = 0;
      TxHeader.IDE = CAN_ID_STD;
      TxHeader.RTR = CAN_RTR_DATA;
      TxHeader.DLC = 1; // Enviando apenas 1 byte
      TxHeader.TransmitGlobalTime = DISABLE;

      uint8_t dados_can[1];
      dados_can[0] = dado_teste++; // Incrementa o dado a cada envio

      // ENVIA PARA A REDE CAN
      HAL_CAN_AddTxMessage(&hcan, &TxHeader, dados_can, &TxMailbox);

      // Espera 250ms (4Hz). Você verá o LED piscar rapidamente se o loopback funcionar.
      HAL_Delay(100);

  /* USER CODE END WHILE */
  }
}

/**
  * @brief System Clock Configuration (Apenas HSE a 72MHz)
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

/* USER CODE BEGIN 4 */

// === CALLBACK DE RECEPÇÃO CAN (LOOPBACK) ===
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK)
    {
        if (RxHeader.StdId == 0x7FF) // Verifica se é o nosso ID de teste
        {
            // Pisca o LED PC13 confirmando que a mensagem bateu no controlador CAN e voltou
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
}
/* USER CODE END 4 */

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
