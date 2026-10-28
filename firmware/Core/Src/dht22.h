#ifndef __DHT22_H__
#define __DHT22_H__

#include "stm32f4xx_hal.h"

#define DHT22_PORT GPIOA
#define DHT22_PIN GPIO_PIN_10
#define DHT22_TIMEOUT_US 120

void DHT22_Start(void);
uint8_t DHT22_Check_Response(void);
uint8_t DHT22_Read(void);
float GetTemp(void);

#endif
