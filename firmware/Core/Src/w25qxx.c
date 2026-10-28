#include "w25qxx.h"

#define W25_CS_LOW()  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET)
#define W25_CS_HIGH() HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET)

extern SPI_HandleTypeDef hspi1;

void W25Q_Reset(void)
{
    uint8_t data[2] = {0x66, 0x99};
    W25_CS_LOW();
    HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
    W25_CS_HIGH();
    HAL_Delay(100);
}

uint32_t W25Q_ReadID(void)
{
    uint8_t cmd = 0x9F;
    uint8_t data[3];
    W25_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, data, 3, 3000);
    W25_CS_HIGH();
    return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

void W25_WriteEnable(void)
{
    uint8_t cmd = 0x06;
    W25_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    W25_CS_HIGH();
}

void W25_Read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    uint8_t cmd[4] = {0x03, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};
    W25_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, buf, len, HAL_MAX_DELAY);
    W25_CS_HIGH();
}

void W25_Write(uint32_t addr, uint8_t *data, uint16_t len)
{
    if (len > 256) len = 256;
    uint8_t cmd[4] = {0x02, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};
    W25_WriteEnable();
    W25_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, data, len, HAL_MAX_DELAY);
    W25_CS_HIGH();
    W25_WaitBusy();
}

void W25_EraseSector(uint32_t addr)
{
    uint8_t cmd[4] = {0x20, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF};
    W25_WriteEnable();
    W25_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, HAL_MAX_DELAY);
    W25_CS_HIGH();
    W25_WaitBusy();
}

void W25_WaitBusy(void)
{
    uint8_t cmd = 0x05;
    uint8_t sr1 = 0;
    do {
        W25_CS_LOW();
        HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
        HAL_SPI_Receive(&hspi1, &sr1, 1, HAL_MAX_DELAY);
        W25_CS_HIGH();
    } while (sr1 & 0x01);
}
