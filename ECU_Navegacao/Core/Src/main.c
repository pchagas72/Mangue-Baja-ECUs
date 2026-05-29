/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : ECU FRONT - ECU that integrates IMU data + optocoupler 
  *                   to a baja-SAE vehicle CAN network.
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

#include <math.h> // Necessary for IMU calculations (atan2, sqrt)
                  //
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
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
CAN_TxHeaderTypeDef TxHeader;
uint32_t TxMailbox;

// CAN Reception variables
CAN_RxHeaderTypeDef RxHeader;
uint8_t RxData[8];
uint16_t rpm_recebido = 0;

// Global optocoupler variables (Volatile because interruption changes it)
volatile uint32_t contador_pulsos = 0;
uint32_t ultimo_tempo_pulso = 0;

// LSM6DS3 IMU variables
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

    // 1. Waits for the LSM6DS3 sensor to init properly
    // TODO: Test if this is really necessary
    HAL_Delay(1000);

    // 2. Inicialização do LSM6DS3
    if (LSM6DS3_Init(&hi2c1) != 1) {
        // Critical failure warning
    	for (int i = 0; i <= 6; i++){
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(300);
        }
    } else {
        // Success on turning on I²C communications with the IMU
        for (int i = 0; i <= 10; i++){
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(50);
        }
        // PC13 finishes start process turned off
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    }

    Kalman_Init(&KalmanRoll);
    Kalman_Init(&KalmanPitch);

    // Initializing CAN network
    CAN_FilterTypeDef canfilterconfig;

    // Initializes CAN to FIFO1 and without filters
    canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig.FilterBank = 0;
    canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
    canfilterconfig.FilterIdHigh = 0x0000;
    canfilterconfig.FilterIdLow = 0x0000;
    canfilterconfig.FilterMaskIdHigh = 0x0000;
    canfilterconfig.FilterMaskIdLow = 0x0000;
    canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;

    // Apply everything and starts CAN network
    HAL_CAN_ConfigFilter(&hcan, &canfilterconfig);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
    HAL_CAN_Start(&hcan);

    uint32_t ultimo_tick = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	        // ====================================================================
	        // 1. Reading optocoupler and transforming result into RPM
	        // ====================================================================
    
            // TODO: Check for better way of calculating RPM
	        uint16_t rpm_real = contador_pulsos * 10 * 60;
	        contador_pulsos = 0;

	        TxHeader.StdId = 0x304;
	        TxHeader.ExtId = 0;
	        TxHeader.IDE = CAN_ID_STD;
	        TxHeader.RTR = CAN_RTR_DATA;
	        TxHeader.DLC = 2;
	        TxHeader.TransmitGlobalTime = DISABLE;

	        uint8_t dados_rpm[2];
	        dados_rpm[0] = rpm_real & 0xFF;
	        dados_rpm[1] = (rpm_real >> 8) & 0xFF;
	        HAL_CAN_AddTxMessage(&hcan, &TxHeader, dados_rpm, &TxMailbox);

            // Spacing for CAN bus: Waits some time in order not to overload CAN
            HAL_Delay(3);

            // ====================================================================
            // 2. IMU reading, Kalman filter application
            // ====================================================================
            if (LSM6DS3_Read(&hi2c1, &imu_data) == 1)
            {
                // 2.1 Calculates dt in seconds for kalman filter
                uint32_t tick_atual = HAL_GetTick();
                float dt = (float)(tick_atual - ultimo_tick) / 1000.0f;
                ultimo_tick = tick_atual;

                // Protection against double instant readings (if dt == 0)
                if (dt > 0.0f)
                {
                    // 2.2 Accelerometer: Calculates roll and pitch using gravity
                    float roll_acc = atan2f((float)imu_data.acc_y, (float)imu_data.acc_z) * 180.0f / M_PI;
                    float pitch_acc = atan2f((float)-imu_data.acc_x, sqrtf((float)imu_data.acc_y * imu_data.acc_y + (float)imu_data.acc_z * imu_data.acc_z)) * 180.0f / M_PI;

                    // 2.3 Gyroscope: Converts crude values into dps (degrees per second)
                    // Sensibility for +-250 dps (default) = 8.75 mdps/LSB -> 0.00875
                    float gyro_rate_x = (float)imu_data.gyr_x * 0.00875f;
                    float gyro_rate_y = (float)imu_data.gyr_y * 0.00875f;

                    // 2.4 Applying kalman filter
                    float roll_kalman = Kalman_getAngle(&KalmanRoll, roll_acc, gyro_rate_x, dt);
                    float pitch_kalman = Kalman_getAngle(&KalmanPitch, pitch_acc, gyro_rate_y, dt);

                    // 2.5 Assemble CAN package
                    int16_t roll_enviar = (int16_t)roll_kalman;
                    int16_t pitch_enviar = (int16_t)pitch_kalman;

                    CAN_TxHeaderTypeDef imuTxHeader;
                    imuTxHeader.StdId = 0x205;
                    imuTxHeader.ExtId = 0;
                    imuTxHeader.IDE = CAN_ID_STD;
                    imuTxHeader.RTR = CAN_RTR_DATA;
                    imuTxHeader.DLC = 4;
                    imuTxHeader.TransmitGlobalTime = DISABLE;

                    uint8_t dados_imu[4];
                    dados_imu[0] = roll_enviar & 0xFF;
                    dados_imu[1] = (roll_enviar >> 8) & 0xFF;
                    dados_imu[2] = pitch_enviar & 0xFF;
                    dados_imu[3] = (pitch_enviar >> 8) & 0xFF;

                    HAL_CAN_AddTxMessage(&hcan, &imuTxHeader, dados_imu, &TxMailbox);
                }

                // Toggle LED everytime the main loop is done
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            }

            // Mantém a frequência em torno de 10 Hz
            HAL_Delay(95);

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

//
// === 1. Optodecoupler callback (ext_interruption) ===
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_4) // PB4 PIN
    {
        uint32_t tempo_atual = HAL_GetTick();

        // Ignores pulses that happen in less than 3ms (Max 20.000 RPM)
        if ((tempo_atual - ultimo_tempo_pulso) > 3)
        {
            contador_pulsos++;
            ultimo_tempo_pulso = tempo_atual;
        }
    }
}

// === 2. CAN RX CALLBACK FOR FLAGS ===
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK)
    {
        /*
        // This is a flag reception example
        if (RxHeader.StdId == 0x304)
        {
            rpm_recebido = RxData[0] | (RxData[1] << 8);
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
        */
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
