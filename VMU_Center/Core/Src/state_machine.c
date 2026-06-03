#include "../Inc/state_machine.h"

static uint32_t last_rpm_tick = 0;
static uint32_t last_imu_tick = 0;
static uint32_t last_debug_tick = 0;

void StateMachine_Init(vmu_state_t *vmu_current_state){
    vmu_current_state->current_state = STATE_BOOT;
    
    vmu_current_state->IMU_initialized = false;
    vmu_current_state->IMU_ok = false;
    vmu_current_state->CAN_initialized = false;
    vmu_current_state->CAN_ok = false;
    vmu_current_state->RPM_warning = false;
    vmu_current_state->boot_tries = 0;
}

void StateMachine_Update(vmu_state_t *vmu_current_state){

    /* Updating state machine */

    uint32_t current_tick = HAL_GetTick();

    switch (vmu_current_state->current_state) {

        case STATE_BOOT:

            /* Initializes communications with modules and CAN network */

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

                // Verifies success with HAL functions
                if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig) == HAL_OK &&
                    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING) == HAL_OK &&
                    HAL_CAN_Start(&hcan) == HAL_OK) {
                    
                    vmu_current_state->CAN_initialized = true;
                }
            }

            RPM_Init();

            /* State update */

            // If CAN network was able to start, transition to self check state
            if (vmu_current_state->CAN_initialized) {
                /* DEBUG: Blink LED 3 times fast and wait 1 second */
                vmu_current_state->current_state = STATE_SELF_CHECK;
            }
            // If CAN network wasn't able to start, go to error mode (the ECU cannot communicate)
            else{
                vmu_current_state->current_state = STATE_ERROR;
                /* DEBUG: Blink LED 3 times slow and waint 1 second */
            }

            vmu_current_state->boot_tries += 1;

            break;

        case STATE_SELF_CHECK:
            /* Self checks for capability of receiving IMU data and sending it through CAN-bus */
            /* We don't do the same for RPM since the engine can be off and that's ok, just flip a warning */
            if (LSM6DS3_Read(&hi2c1, &lsm6ds3_raw_data)){
                vmu_current_state->IMU_ok = true;
            }
            // Now do it for CAN bus
            // If need be, we can add this later when at least one more ECU is available to send and receive ping packets
            // For now we can just assume that the CAN bus is healthy
            vmu_current_state->CAN_ok = true;

            if (vmu_current_state->CAN_ok){
                /* DEBUG: Blink LED 3 times fast and wait 1 second */
                vmu_current_state->current_state = STATE_RUNNING;
            } else {
                /* DEBUG: Blink LED 3 times slow and wait 1 second */
                vmu_current_state->current_state = STATE_ERROR;
            }

            break;

        case STATE_RUNNING:
            /* 10ms task -> Read and send RPM */
            if (current_tick - last_rpm_tick >= RPM_DELAY) {
                last_rpm_tick = current_tick;

                uint16_t current_rpm = RPM_Read();
                vmu_current_state->RPM_warning = RPM_CheckWarning(current_rpm);

                CAN_Send_RPM(current_rpm);
            }

            /* 500ms task -> Process IMU (Kalman) and send */
            if (vmu_current_state->IMU_ok) {
                if (current_tick - last_imu_tick >= IMU_DELAY) {
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

            if (current_tick - last_debug_tick >= 100){
                /* Blink LED */
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); 
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
