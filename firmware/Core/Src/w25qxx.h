#ifndef W25QXX_H
#define W25QXX_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define CS_PIN GPIO_PIN_4
#define CS_PORT GPIOA

void W25_Read(uint32_t addr, uint8_t *buf, uint16_t len);
void W25_Write(uint32_t addr, uint8_t *data, uint16_t len);
void W25_EraseSector(uint32_t addr);
uint32_t W25Q_ReadID(void);
void W25Q_Reset(void);
void W25_WriteEnable(void);
void W25_WaitBusy(void);

#endif
