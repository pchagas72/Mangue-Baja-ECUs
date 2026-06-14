#include "../Inc/rpm.h"
#include "stm32f1xx_hal.h"

#define RPM_WARNING_THRESHOLD 3500

volatile uint32_t ultimo_tempo_pulso = 0;
volatile float rpm_filtrado = 0.0f;

void RPM_Init(void) {
    ultimo_tempo_pulso = 0;
    rpm_filtrado = 0.0f;
}

uint16_t RPM_Read(void) {
    // Se passar mais de 150ms sem pulso, o motor é considerado desligado
    if ((HAL_GetTick() - ultimo_tempo_pulso) > 150) {
        rpm_filtrado = 0.0f;
    }
    return (uint16_t)rpm_filtrado;
}

bool RPM_CheckWarning(uint16_t current_rpm) {
    return (current_rpm <= 1000);
}

// Esta função deve ser chamada dentro de HAL_GPIO_EXTI_Callback no main.c ou stm32f1xx_it.c
void RPM_EXTI_Callback(void) {
    uint32_t tempo_atual = HAL_GetTick();
    uint32_t delta_t = tempo_atual - ultimo_tempo_pulso;

    // Debounce de 3ms
    if (delta_t > 3) {
        float rpm_instantaneo = 60000.0f / (float)delta_t;
        // Filtro Passa-Baixa (Média Móvel Exponencial)
        rpm_filtrado = (0.3f * rpm_instantaneo) + (0.7f * rpm_filtrado);
        ultimo_tempo_pulso = tempo_atual;
    }
}
