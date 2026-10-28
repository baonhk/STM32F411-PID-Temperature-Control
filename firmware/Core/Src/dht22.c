#include "dht22.h"
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim10;

void delay(uint16_t time)
{
    if (time > 2000) time = 2000;
    __HAL_TIM_SET_COUNTER(&htim10, 0);
    while (__HAL_TIM_GET_COUNTER(&htim10) < time);
}

void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void DHT22_Start(void)
{
    Set_Pin_Output(DHT22_PORT, DHT22_PIN);
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    delay(1800);
    Set_Pin_Input(DHT22_PORT, DHT22_PIN);
}

uint8_t DHT22_Check_Response(void)
{
    uint8_t response = 0;
    delay(40);
    if (!HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN)) {
        delay(80);
        response = HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) ? 1 : 0;
    }
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN));
    return response;
}

static uint8_t wait_pin_state(GPIO_PinState state, uint16_t timeout_us)
{
    __HAL_TIM_SET_COUNTER(&htim10, 0);
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) != state) {
        if (__HAL_TIM_GET_COUNTER(&htim10) > timeout_us)
            return 0;
    }
    return 1;
}

uint8_t DHT22_Read(void)
{
    uint8_t value = 0;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (!wait_pin_state(GPIO_PIN_SET, DHT22_TIMEOUT_US))
            return 0xFF;

        delay(40);
        if (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN))
            value |= (1U << (7 - bit));

        if (!wait_pin_state(GPIO_PIN_RESET, DHT22_TIMEOUT_US))
            return 0xFF;
    }
    return value;
}

float GetTemp(void)
{
    uint8_t data[4] = {0};
    uint8_t checksum;
    uint8_t presence;

    DHT22_Start();
    presence = DHT22_Check_Response();
    if (presence != 1) return 0.0f;

    data[0] = DHT22_Read();
    data[1] = DHT22_Read();
    data[2] = DHT22_Read();
    data[3] = DHT22_Read();
    checksum = DHT22_Read();

    if (data[0] == 0xFF || data[1] == 0xFF || data[2] == 0xFF ||
        data[3] == 0xFF || checksum == 0xFF)
        return 0.0f;

    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != checksum)
        return 0.0f;

    uint16_t raw = ((uint16_t)(data[2] & 0x7F) << 8) | data[3];
    float temperature = raw / 10.0f;
    if (data[2] & 0x80) temperature = -temperature;
    return temperature;
}
