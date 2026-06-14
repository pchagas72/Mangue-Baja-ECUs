#include "../Inc/state_machine.h"

static uint32_t last_rpm_tick = 0;
static uint32_t last_imu_tick = 0;
static uint32_t last_debug_tick = -10000;
static uint32_t last_led_tick = 0;

void StateMachine_Init(vmu_state_t *vmu_current_state){
    vmu_current_state->current_state = STATE_BOOT;

    vmu_current_state->IMU_initialized = false;
    vmu_current_state->IMU_ok = false;
    vmu_current_state->CAN_initialized = false;
    vmu_current_state->CAN_ok = false;
    vmu_current_state->RPM_warning = false;
    vmu_current_state->low_voltage = false;
    vmu_current_state->boot_tries = 0;
}

void StateMachine_Update(vmu_state_t *vmu_current_state){

    /* Updating state machine */

    uint32_t current_tick = HAL_GetTick();

    switch (vmu_current_state->current_state) {

        case STATE_BOOT:

            // Init IMU
            if (!vmu_current_state->IMU_initialized) {
                if (LSM6DS3_Init(&hi2c1) == 1) {
                    Kalman_Init(&KalmanRoll);
                    Kalman_Init(&KalmanPitch);
                    vmu_current_state->IMU_initialized = true;
                }
            }

            // Init CAN Network
            if (!vmu_current_state->CAN_initialized) {
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

                if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig) == HAL_OK &&
                        HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING) == HAL_OK &&
                        HAL_CAN_Start(&hcan) == HAL_OK) {

                    vmu_current_state->CAN_initialized = true;
                }
            }

            RPM_Init();

            // --- Lógica de Transição Tolerante a Falhas ---
            if (vmu_current_state->CAN_initialized && vmu_current_state->IMU_initialized) {
                // Cenário Ideal: Tudo ligou, vamos para a pista!
                vmu_current_state->current_state = STATE_SELF_CHECK;
            }
            else if (vmu_current_state->boot_tries > 50) {
                // Timeout: Se passarem ~500ms e a IMU não acordar, joga a ECU pra frente para não
                // perder a telemetria do motor (RPM). Ficar preso aqui no Baja é inaceitável.
                if (vmu_current_state->CAN_initialized) {
                    vmu_current_state->current_state = STATE_SELF_CHECK;
                } else {
                    vmu_current_state->current_state = STATE_ERROR; // Sem CAN a ECU é inútil
                }
            }

            // Dá um tempo vital de 10ms para os hardwares externos (IMU) "respirarem"
            HAL_Delay(10);
            vmu_current_state->boot_tries += 1;

            break;

        case STATE_SELF_CHECK:
            // Só tenta ler os dados se a IMU tiver conseguido sair do boot com vida
            if (vmu_current_state->IMU_initialized && LSM6DS3_Read(&hi2c1, &lsm6ds3_raw_data)){
                vmu_current_state->IMU_ok = true;
            } else {
                vmu_current_state->IMU_ok = false;
            }

            vmu_current_state->CAN_ok = true;

            if (vmu_current_state->CAN_ok){
                vmu_current_state->current_state = STATE_RUNNING;
            } else {
                vmu_current_state->current_state = STATE_ERROR;
            }

            break;


        case STATE_RUNNING:
            uint32_t current_rpm_delay = vmu_current_state->low_voltage ? RPM_LB_DELAY : RPM_DELAY;
            uint32_t current_imu_delay = vmu_current_state->low_voltage ? IMU_LB_DELAY : IMU_DELAY;

            /* 10ms task -> Read and send RPM */
            if (current_tick - last_rpm_tick >= current_rpm_delay) {
                last_rpm_tick = current_tick;

                uint16_t current_rpm = RPM_Read();
                vmu_current_state->RPM_warning = RPM_CheckWarning(current_rpm);

                CAN_Send_RPM(current_rpm);
            }

            /* 500ms task -> Process IMU (Kalman) and send */
            if (vmu_current_state->IMU_ok) {
                if (current_tick - last_imu_tick >= current_imu_delay) {
                    last_imu_tick = current_tick;

                    if (IMU_Process(&imu_data)) {
                        CAN_Send_IMU(&imu_data);

                    } else {
                        // If reading fails mid-race, drop the OK flag
                        vmu_current_state->IMU_ok = false; 
                    }
                }
            }

            /* 5000ms task -> Send VMU Debug State */
            if (current_tick - last_debug_tick >= DEBUG_DELAY) {
                last_debug_tick = current_tick;
                can_debug_packet_t debug_packet;
                debug_packet.current_state = vmu_current_state->current_state;
                debug_packet.CAN_ok = vmu_current_state->CAN_ok;
                debug_packet.IMU_ok = vmu_current_state->IMU_ok;
                debug_packet.boot_tries = vmu_current_state->boot_tries;
                debug_packet.RPM_warning = vmu_current_state->RPM_warning;
                CAN_Send_Debug(&debug_packet);
            }

            /* Heartbeat LED blink: Keep off to save power if low_voltage is active */
            if (!vmu_current_state->low_voltage) {
                if (current_tick - last_led_tick >= 100){
                    last_led_tick = current_tick;
                    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                }
            } else {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            }

            break;

        case STATE_ERROR:
            /* Tenta reiniciar o processo de boot até 3 vezes */
            if (vmu_current_state->boot_tries < 3) {
                HAL_Delay(1000); // Dá 1 segundo para a elétrica "respirar"
                StateMachine_Init(vmu_current_state); // Reinicia a máquina
            } else {
                /* Hard Fault: O sistema não consegue se recuperar. 
                   Aqui você deve acender um LED vermelho na sua PCB customizada
                   para o piloto ou os boxes saberem que a ECU morreu. */

                // Exemplo (ajuste para o pino do seu LED):
                // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); 
                // HAL_Delay(200);
            }
            break;
    }
}
