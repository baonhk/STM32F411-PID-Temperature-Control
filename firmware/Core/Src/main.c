#include "main.h"

#include "lcd.h"
#include <stdio.h>
#include "dht22.h"
#include "proto.h"
#include "w25qxx.h"
#include <string.h>

SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim9;
TIM_HandleTypeDef htim10;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart6_rx;
DMA_HandleTypeDef hdma_usart6_tx;

#define RxBuf_SIZE 40
uint8_t RxBuf[RxBuf_SIZE];
uint8_t Receive[RxBuf_SIZE];
uint8_t rx_tmp_buf[RxBuf_SIZE];
uint16_t rx_tmp_len = 0;

typedef struct {
    uint8_t RX : 1;
    uint8_t Temp : 1;
    uint8_t PID : 1;
    uint8_t LCD : 1;
    uint8_t rx_pending : 1;
} Flags;
volatile Flags flag;

char Char_temp[6];
char Char_set[6];
char Char_PWM[6];

int16_t outPID;
int16_t LastOutput;

uint8_t Data[4] = {0};
uint16_t SUM, TEMP;
float Temperature;
uint8_t Presence;

float T = 2;
uint16_t T_PID;
float pid, anpha, beta, gama, delta, deltaPWM;
float e, e1, e2;

float PID_Kp = 10.0f;
float PID_Ki = 13.0f;
float PID_Kd = 50.0f;
float temperature_current = 30.0f;
float setpoint_current = 33.0f;

uint32_t ID;
#define ADD_SET 0x010000
#define ADD_PAR 0x020000

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_TIM10_Init(void);
static void MX_TIM9_Init(void);
static void MX_SPI1_Init(void);

void OutPWM(int16_t duty)
{
    if (duty < 0) duty = 0;
    if (duty > 999) duty = 999;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
}

void resetPID(void)
{
    e = 0; e1 = 0; e2 = 0;
    LastOutput = 0;
    outPID = 0;
    OutPWM(0);
}

void PID_init(float Kp, float Ki, float Kd)
{
    T_PID = (int16_t)(T * 100 - 1);
    resetPID();
    anpha = 2*T*Kp + Ki*T*T + 2*Kd;
    beta = T*T*Ki - 4*Kd - 2*T*Kp;
    gama = 2*Kd;
    delta = 2*T;
}

void ComputePID(float set)
{
    if (outPID > 999) resetPID();
    e = set - temperature_current;
    if (e > 20) resetPID();
    pid = (anpha*e + beta*e1 + gama*e2 + delta*LastOutput) / delta;
    outPID = (int16_t)pid;
    OutPWM(outPID);
    e2 = e1;
    e1 = e;
    LastOutput = outPID;
}

void GetSetpoint_Flash(float *value)
{
    uint8_t buf[4];
    W25_Read(ADD_SET, buf, 4);
    memcpy(value, buf, 4);
}

void SetSetpoint_Flash(float value)
{
    uint8_t buf[4];
    memcpy(buf, &value, 4);
    W25_EraseSector(ADD_SET);
    W25_Write(ADD_SET, buf, 4);
    PID_init(PID_Kp, PID_Ki, PID_Kd);
    flag.LCD = 1;
}

void GetParamPID_Flash(float *Kp, float *Ki, float *Kd)
{
    uint8_t buf[12];
    W25_Read(ADD_PAR, buf, 12);
    memcpy(Kp, buf, 4);
    memcpy(Ki, buf + 4, 4);
    memcpy(Kd, buf + 8, 4);
}

void SetParamPID_Flash(float Kp, float Ki, float Kd)
{
    uint8_t buf[12];
    memcpy(&buf[0], &Kp, 4);
    memcpy(&buf[4], &Ki, 4);
    memcpy(&buf[8], &Kd, 4);
    W25_EraseSector(ADD_PAR);
    W25_Write(ADD_PAR, buf, 12);
    PID_init(PID_Kp, PID_Ki, PID_Kd);
}

float GetTemps(void)
{
    DHT22_Start();
    Presence = DHT22_Check_Response();
    if (Presence != 1) return temperature_current;

    Data[0] = DHT22_Read();
    Data[1] = DHT22_Read();
    Data[2] = DHT22_Read();
    Data[3] = DHT22_Read();
    SUM = DHT22_Read();

    if (Data[0] == 0xFF || Data[1] == 0xFF || Data[2] == 0xFF ||
        Data[3] == 0xFF || SUM == 0xFF)
        return temperature_current;

    uint8_t checksum = (uint8_t)(Data[0] + Data[1] + Data[2] + Data[3]);
    if (checksum != (uint8_t)SUM)
        return temperature_current;

    TEMP = ((uint16_t)(Data[2] & 0x7F) << 8) | Data[3];
    Temperature = (float)TEMP / 10.0f;
    if (Data[2] & 0x80) Temperature = -Temperature;
    return Temperature;
}

static void uart6_rx_cb(uint8_t *data, uint16_t len)
{
    HandleCMD(data, len);
    flag.rx_pending = 1;
    memcpy(rx_tmp_buf, data, len);
    rx_tmp_len = len;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_TIM3_Init();
    MX_USART6_UART_Init();
    MX_TIM10_Init();
    MX_TIM9_Init();
    MX_SPI1_Init();

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_Base_Start(&htim10);
    HAL_TIM_Base_Start_IT(&htim9);

    flag.Temp = 0;
    flag.PID = 0;
    flag.LCD = 0;
    flag.rx_pending = 0;

    GetSetpoint_Flash(&setpoint_current);
    GetParamPID_Flash(&PID_Kp, &PID_Ki, &PID_Kd);

    UARTDMA_Init(&huart6, uart6_rx_cb, RxBuf, RxBuf_SIZE);
    W25Q_Reset();
    ID = W25Q_ReadID();

    PID_init(PID_Kp, PID_Ki, PID_Kd);

    LCD_Init();
    LCD_Goto(0,0); LCD_Print("SetPoint");
    LCD_Goto(0,14); LCD_Data(0xDF);
    LCD_Goto(0,15); LCD_Print("C");
    LCD_Goto(1,0); LCD_Print("Nhiet Do");
    LCD_Goto(1,14); LCD_Data(0xDF);
    LCD_Goto(1,15); LCD_Print("C");
    f2str(setpoint_current, Char_set, sizeof(Char_set));
    LCD_Goto(0,9); LCD_Print(Char_set);

    while (1)
    {
        if (flag.Temp)
        {
            temperature_current = GetTemps();
            f2str(temperature_current, Char_temp, sizeof(Char_temp));
            LCD_Goto(1,9);
            LCD_Print(Char_temp);
            flag.Temp = 0;
        }

        if (flag.rx_pending && flag.Temp == 0)
            flag.rx_pending = 0;

        if (flag.PID)
        {
            ComputePID(setpoint_current);
            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
            flag.PID = 0;
        }

        if (flag.LCD)
        {
            flag.LCD = 0;
            f2str(setpoint_current, Char_set, sizeof(Char_set));
            LCD_Goto(0,9);
            LCD_Print(Char_set);
        }
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 12;
    RCC_OscInitStruct.PLL.PLLN = 96;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
        Error_Handler();
}

static void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
}

static void MX_TIM3_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 19;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 999;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) Error_Handler();

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) Error_Handler();
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) Error_Handler();

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) Error_Handler();

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) Error_Handler();

    HAL_TIM_MspPostInit(&htim3);
}

static void MX_TIM9_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    htim9.Instance = TIM9;
    htim9.Init.Prescaler = 4999;
    htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim9.Init.Period = 99;
    htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim9) != HAL_OK) Error_Handler();

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim9, &sClockSourceConfig) != HAL_OK) Error_Handler();
}

static void MX_TIM10_Init(void)
{
    htim10.Instance = TIM10;
    htim10.Init.Prescaler = 50 - 1;
    htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim10.Init.Period = 65535;
    htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim10) != HAL_OK) Error_Handler();
}

static void MX_USART6_UART_Init(void)
{
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart6) != HAL_OK) Error_Handler();
}

static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, F_CS_Pin | E_LCD_Pin | RS_LCD_Pin | DHT22_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, D7_LCD_Pin | D6_LCD_Pin | D5_LCD_Pin | D4_LCD_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = LED_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = KEY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(KEY_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = F_CS_Pin | E_LCD_Pin | RS_LCD_Pin | DHT22_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = D7_LCD_Pin | D6_LCD_Pin | D5_LCD_Pin | D4_LCD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint8_t cnta = 0;
    static uint8_t cntb = 0;

    if (htim->Instance == TIM9)
    {
        if (++cnta > 49) {
            flag.Temp = 1;
            cnta = 0;
        }
        if (++cntb > T_PID) {
            flag.PID = 1;
            cntb = 0;
        }
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
