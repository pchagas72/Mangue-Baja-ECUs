/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : ECU FRONT - Baja CAN Simulator, Optocoupler & IMU
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "i2c.h"
#include "gpio.h"
#include "kalman.h"
#include "lsm6ds3.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h> // Necessário para as funções trigonométricas (atan2, sqrt)
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
uint16_t rpm_recebido = 0;

// Variável Global do Optoacoplador (Volatile porque muda na interrupção)
volatile uint32_t contador_pulsos = 0;

// Variáveis do LSM6DS3
LSM6DS3_Data_t imu_data;
Kalman_t KalmanRoll;
Kalman_t KalmanPitch;
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
  MX_CAN_Init(); // Lembre-se: CAN_MODE_NORMAL na bancada real!
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */

    // 1. ESPERA O SENSOR ACORDAR ANTES DE FALAR COM ELE!
    HAL_Delay(100);

    // 2. Inicialização do LSM6DS3
    if (LSM6DS3_Init(&hi2c1) != 1) {
        // FALHA: Pisca devagar
        for (int i = 0; i <= 6; i++){
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(300);
        }
    } else {
        // SUCESSO: O Sensor respondeu! Pisca rápido para comemorar.
        for (int i = 0; i <= 10; i++){
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(50);
        }
    }

    Kalman_Init(&KalmanRoll);
    Kalman_Init(&KalmanPitch);

    // Inicialização da rede CAN

  // Inicialização da rede CAN

  CAN_FilterTypeDef canfilterconfig;

  // Configuração do filtro CAN "passa-tudo" direcionado ao FIFO1
  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
  canfilterconfig.FilterBank = 0;
  canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
  canfilterconfig.FilterIdHigh = 0x0000;
  canfilterconfig.FilterIdLow = 0x0000;
  canfilterconfig.FilterMaskIdHigh = 0x0000;
  canfilterconfig.FilterMaskIdLow = 0x0000;
  canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;

  // Aplica o filtro, ativa interrupção e dá a partida na CAN
  HAL_CAN_ConfigFilter(&hcan, &canfilterconfig);
  HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
  HAL_CAN_Start(&hcan);

  // Garante que o LED PC13 comece apagado (Lógica Invertida: SET = OFF)
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // ====================================================================
      // 1. LEITURA E CÁLCULO DO MOTOR (OPTOACOPLADOR)
      // ====================================================================
      uint16_t rpm_real = contador_pulsos * 10 * 60;
      contador_pulsos = 0; // Zera para os próximos 100ms

      // Prepara e envia o CAN do RPM (ID 0x304)
      TxHeader.StdId = 0x304;
      TxHeader.ExtId = 0;
      TxHeader.IDE = CAN_ID_STD;
      TxHeader.RTR = CAN_RTR_DATA;
      TxHeader.DLC = 2;
      TxHeader.TransmitGlobalTime = DISABLE;

      uint8_t dados_rpm[2];
      dados_rpm[0] = rpm_real & 0xFF;        // LSB primeiro
      dados_rpm[1] = (rpm_real >> 8) & 0xFF; // MSB depois
      HAL_CAN_AddTxMessage(&hcan, &TxHeader, dados_rpm, &TxMailbox);


      // ====================================================================
      // 2. LEITURA E CÁLCULO DA SUSPENSÃO (IMU + KALMAN)
      // ====================================================================
      LSM6DS3_Read(&hi2c1, &imu_data);

      // Converte para unidades físicas (g e dps)
      float acc_x_real = imu_data.acc_x * (2.0f / 32768.0f);
      float acc_y_real = imu_data.acc_y * (2.0f / 32768.0f);
      float acc_z_real = imu_data.acc_z * (2.0f / 32768.0f);
      float rate_x_dps = imu_data.gyr_x * (245.0f / 32768.0f); // Para Roll
      float rate_y_dps = imu_data.gyr_y * (245.0f / 32768.0f); // Para Pitch

      // Calcula ângulos brutos
      float roll_bruto = atan2(acc_y_real, acc_z_real) * 180.0f / M_PI;
      float pitch_bruto = atan2(-acc_x_real, sqrt((acc_y_real * acc_y_real) + (acc_z_real * acc_z_real))) * 180.0f / M_PI;

      // Aplica o filtro de Kalman (dt = 0.1s pois o loop tem delay de 100ms)
      float roll_filtrado = Kalman_getAngle(&KalmanRoll, roll_bruto, rate_x_dps, 0.1f);
      float pitch_filtrado = Kalman_getAngle(&KalmanPitch, pitch_bruto, rate_y_dps, 0.1f);

      // Prepara e envia o CAN da IMU (ID 0x205)
      CAN_TxHeaderTypeDef imuTxHeader;
      imuTxHeader.StdId = 0x205; // ID dedicado aos ângulos do carro
      imuTxHeader.ExtId = 0;
      imuTxHeader.IDE = CAN_ID_STD;
      imuTxHeader.RTR = CAN_RTR_DATA;
      imuTxHeader.DLC = 4; // 4 bytes (2 pro Roll, 2 pro Pitch)
      imuTxHeader.TransmitGlobalTime = DISABLE;

      // Convertendo os floats para inteiros (graus puros) para a rede CAN
      int16_t roll_send = (int16_t)roll_filtrado;
      int16_t pitch_send = (int16_t)pitch_filtrado;

      uint8_t dados_imu[4];
      // Empacota o Roll (Little-Endian)
      dados_imu[0] = roll_send & 0xFF;
      dados_imu[1] = (roll_send >> 8) & 0xFF;
      // Empacota o Pitch (Little-Endian)
      dados_imu[2] = pitch_send & 0xFF;
      dados_imu[3] = (pitch_send >> 8) & 0xFF;

      HAL_CAN_AddTxMessage(&hcan, &imuTxHeader, dados_imu, &TxMailbox);


      // ====================================================================
      // 3. CONTROLE DE TEMPO (10 Hz)
      // ====================================================================
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

// === 1. CALLBACK DE INTERRUPÇÃO EXTERNA (OPTOACOPLADOR / MOTOR) ===
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_4) // Pino PB4 da placa customizada
    {
        contador_pulsos++;
        //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_11);
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}

// === 2. CALLBACK DE RECEPÇÃO CAN (LOOPBACK/NORMAL) ===
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK)
    {
        if (RxHeader.StdId == 0x304)
        {
            rpm_recebido = RxData[0] | (RxData[1] << 8);
            //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
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
