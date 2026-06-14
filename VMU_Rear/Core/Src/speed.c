#include "../Inc/speed.h"

void Speed_Task(uint32_t current_tick, bool is_low_battery, uint32_t contador_pulsos_indutivo, uint32_t last_speed_tick) {
    uint32_t current_delay = is_low_battery ? SPEED_LB_DELAY : SPEED_DELAY;

    if (current_tick - last_speed_tick >= current_delay) {
        uint32_t dt_ms = current_tick - last_speed_tick;
        last_speed_tick = current_tick;

        __disable_irq();
        uint32_t pulsos_locais = contador_pulsos_indutivo;
        contador_pulsos_indutivo = 0;
        __enable_irq();

        float speed_hz = 0;
        if (dt_ms > 0) {
            speed_hz = (pulsos_locais * 1000.0f) / (float)dt_ms;
        }
        float speed_kmh = (3.6f * PI * WHEEL_DIAMETER * speed_hz) / WHEEL_HOLES_NUMBER_REAR;

        CAN_Send_Speed((uint16_t)speed_kmh);
    }
}
