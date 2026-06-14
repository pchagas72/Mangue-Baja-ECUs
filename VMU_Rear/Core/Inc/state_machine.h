#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "main.h"
#include <stdbool.h>

enum TCU_STATES {
    STATE_BOOT = 0,
    STATE_SELF_CHECK = 1,
    STATE_RUNNING = 2,
    STATE_ERROR = -1,
};

typedef struct tcu_state {
    enum TCU_STATES current_state;
    int boot_tries;
    bool CAN_initialized;
    bool CAN_ok;
    bool MLX_initialized;
    bool MLX_ok;
    bool low_voltage; // Agora é ativamente atualizada
} tcu_state_t;

void StateMachine_Init(tcu_state_t *tcu_current_state);
void StateMachine_Update(tcu_state_t *tcu_current_state);

#endif // STATE_MACHINE_H
