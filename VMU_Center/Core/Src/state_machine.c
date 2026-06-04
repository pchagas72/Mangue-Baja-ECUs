#include "../Inc/state_machine.h"

/* Defining  ticks for each activity */

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
    vmu_current_state->boot_tries = 0;
}

void StateMachine_Update(vmu_state_t *vmu_current_state){

    /* Superloop */

    uint32_t current_tick = HAL_GetTick();

    switch (vmu_current_state->current_state) {

        case STATE_BOOT:

            /*
             * @brief: Boot state starts every sensor that needs to be 
             * initialized (I²C) and also the CAN Network
             */

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

            /* Transition logic */
            if (vmu_current_state->CAN_initialized && vmu_current_state->IMU_initialized) {
                // All OK.
                vmu_current_state->current_state = STATE_SELF_CHECK;
            }
            else if (vmu_current_state->boot_tries > 50) {
                // Waits ~500ms for the LSM6DS3 to init, if that is not the case proceed to only
                // acquire RPM data.
                if (vmu_current_state->CAN_initialized) {
                    vmu_current_state->current_state = STATE_SELF_CHECK;
                } else {
                    vmu_current_state->current_state = STATE_ERROR; // Without CAN the ECU is useless
                }
            }

            // 10ms for the hardware to breath a little
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

            if (current_tick - last_led_tick >= 100){
                /* Blink LED */
                last_led_tick = current_tick;
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            }

            break;

        case STATE_ERROR:
            /* Tries to restart the boot process 10 times */
            /* Read lines 74-86 */
            if (vmu_current_state->boot_tries < 500) {
                HAL_Delay(1000); // 1 second for the hardware
                StateMachine_Init(vmu_current_state); // Restarts the state machine
            } else {
                /* 
                 * Study what can be done in case of a hard fault.
                 * If nothing is found just turn ECU off.
                 */
            }
            break;
    }
}
