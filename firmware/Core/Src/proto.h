#ifndef __PROTO_H__
#define __PROTO_H__

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define HEADER0 0xAA
#define HEADER1 0x55
#define TAIL0   0x55
#define TAIL1   0xAA

#define CMD_CONNECT        0x0A
#define CMD_GET_PARAM      0x0B
#define CMD_GET_DATA       0x0C
#define CMD_SAVE_SETPOINT  0x0D
#define CMD_SAVE_PARAM     0x0F

#define MAX_BODY     64
#define MAX_STUFFED  128
#define MAX_FRAME    (2 + MAX_STUFFED + 2)
#define MAX_PAYLOAD  64

typedef void (*uart_rx_callback_t)(uint8_t *data, uint16_t len);

void UARTDMA_Init(UART_HandleTypeDef *huart, uart_rx_callback_t cb,
                  uint8_t *rx_buf, uint16_t rx_bufsize);
void PROTO_SendFrame(uint8_t cmd, const uint8_t *payload, uint16_t len);
void PROTO_Handle(const uint8_t *stf, uint16_t len_stf);
void HandleCMD(const uint8_t *frame, uint16_t len);

#endif
