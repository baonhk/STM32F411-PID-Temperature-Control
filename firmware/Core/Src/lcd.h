#ifndef __LCD_H__
#define __LCD_H__

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define LCD_RS_GPIO_Port GPIOA
#define LCD_RS_Pin GPIO_PIN_9
#define LCD_E_GPIO_Port GPIOA
#define LCD_E_Pin GPIO_PIN_8
#define LCD_D4_GPIO_Port GPIOB
#define LCD_D4_Pin GPIO_PIN_15
#define LCD_D5_GPIO_Port GPIOB
#define LCD_D5_Pin GPIO_PIN_14
#define LCD_D6_GPIO_Port GPIOB
#define LCD_D6_Pin GPIO_PIN_13
#define LCD_D7_GPIO_Port GPIOB
#define LCD_D7_Pin GPIO_PIN_12

void LCD_Init(void);
void LCD_Cmd(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_Print(char *str);
void LCD_Goto(uint8_t row, uint8_t col);
bool f2str(float x, char *out, size_t outlen);

#endif
