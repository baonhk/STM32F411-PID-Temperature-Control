/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
void Error_Handler(void);

#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define KEY_Pin GPIO_PIN_0
#define KEY_GPIO_Port GPIOA
#define F_CS_Pin GPIO_PIN_4
#define F_CS_GPIO_Port GPIOA
#define D7_LCD_Pin GPIO_PIN_12
#define D7_LCD_GPIO_Port GPIOB
#define D6_LCD_Pin GPIO_PIN_13
#define D6_LCD_GPIO_Port GPIOB
#define D5_LCD_Pin GPIO_PIN_14
#define D5_LCD_GPIO_Port GPIOB
#define D4_LCD_Pin GPIO_PIN_15
#define D4_LCD_GPIO_Port GPIOB
#define E_LCD_Pin GPIO_PIN_8
#define E_LCD_GPIO_Port GPIOA
#define RS_LCD_Pin GPIO_PIN_9
#define RS_LCD_GPIO_Port GPIOA
#define DHT22_Pin GPIO_PIN_10
#define DHT22_GPIO_Port GPIOA

#ifdef __cplusplus
}
#endif
#endif
