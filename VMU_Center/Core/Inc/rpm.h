#ifndef RPM_H
#define RPM_H

#include <stdint.h>
#include <stdbool.h>

#define RPM_DELAY 100

void RPM_Init(void);
uint16_t RPM_Read(void);
bool RPM_CheckWarning(uint16_t current_rpm);
void RPM_EXTI_Callback(void);

#endif /* RPM_H */
