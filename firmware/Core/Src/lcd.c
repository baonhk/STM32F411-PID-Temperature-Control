#include "lcd.h"
#include <string.h>

static void LCD_EnablePulse(void)
{
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}

static void LCD_Write4(uint8_t nibble)
{
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (nibble & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (nibble & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (nibble & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (nibble & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    LCD_EnablePulse();
}

static void LCD_Write(uint8_t rs, uint8_t data)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, rs ? GPIO_PIN_SET : GPIO_PIN_RESET);
    LCD_Write4(data >> 4);
    LCD_Write4(data & 0x0F);
    HAL_Delay(2);
}

void LCD_Cmd(uint8_t cmd) { LCD_Write(0, cmd); }
void LCD_Data(uint8_t data) { LCD_Write(1, data); }

void LCD_Init(void)
{
    HAL_Delay(50);
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
    LCD_Write4(0x03); HAL_Delay(5);
    LCD_Write4(0x03); HAL_Delay(5);
    LCD_Write4(0x03); HAL_Delay(1);
    LCD_Write4(0x02);
    LCD_Cmd(0x28);
    LCD_Cmd(0x0C);
    LCD_Cmd(0x06);
    LCD_Cmd(0x01); HAL_Delay(2);
}

void LCD_Print(char *str)
{
    while (*str) LCD_Data((uint8_t)*str++);
}

void LCD_Goto(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    LCD_Cmd(0x80 | (addr + col));
}

bool f2str(float x, char *out, size_t outlen)
{
    if (!out || outlen < 6) return false;
    bool neg = (x < 0.0f);
    float f = neg ? -x : x;
    uint32_t scaled = (uint32_t)(f * 100.0f + 0.5f);
    uint32_t ip = scaled / 100;
    uint32_t fp = scaled % 100;

    char tmp[32];
    size_t pos = 0;
    if (neg) tmp[pos++] = '-';

    if (ip < 10) {
        tmp[pos++] = '0';
        tmp[pos++] = (char)('0' + ip);
    } else {
        char rev[10];
        int n = 0;
        while (ip > 0) {
            rev[n++] = (char)('0' + (ip % 10));
            ip /= 10;
        }
        for (int i = n - 1; i >= 0; --i) tmp[pos++] = rev[i];
    }

    tmp[pos++] = '.';
    tmp[pos++] = (char)('0' + (fp / 10));
    tmp[pos++] = (char)('0' + (fp % 10));
    tmp[pos] = '\0';

    if (pos + 1 > outlen) return false;
    memcpy(out, tmp, pos + 1);
    return true;
}
