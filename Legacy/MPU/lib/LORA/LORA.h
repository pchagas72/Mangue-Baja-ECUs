#ifndef LORA_H
#define LORA_H

#include <EBYTE.h>
#include "MPU_defs.h"

#define OPT_TP30 0b00		// 30 db
#define CAR_ID   MB_ID

uint8_t LORA_init(void);
bool LORA_SendStruct(void *TheStructure, uint16_t size_);

#endif
