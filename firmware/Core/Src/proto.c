#include "proto.h"
#include <string.h>

static uint8_t s_tx_frame[MAX_FRAME];
static UART_HandleTypeDef *s_huart = NULL;
static uint8_t *s_rx_buf = NULL;
static uint16_t s_rx_bufsize = 0;
static uart_rx_callback_t s_rx_cb = NULL;

void UARTDMA_Init(UART_HandleTypeDef *huart, uart_rx_callback_t cb,
                  uint8_t *rx_buf, uint16_t rx_bufsize)
{
    s_huart = huart;
    s_rx_cb = cb;
    s_rx_buf = rx_buf;
    s_rx_bufsize = rx_bufsize;

    HAL_UARTEx_ReceiveToIdle_DMA(s_huart, s_rx_buf, s_rx_bufsize);
    if (s_huart->hdmarx)
        __HAL_DMA_DISABLE_IT(s_huart->hdmarx, DMA_IT_HT);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart != s_huart) return;
    if (Size > s_rx_bufsize) Size = s_rx_bufsize;

    if (s_rx_cb)
        s_rx_cb(s_rx_buf, Size);

    HAL_UARTEx_ReceiveToIdle_DMA(s_huart, s_rx_buf, s_rx_bufsize);
    if (s_huart->hdmarx)
        __HAL_DMA_DISABLE_IT(s_huart->hdmarx, DMA_IT_HT);
}

static uint16_t CRC16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
    return crc;
}

static int Stuff(const uint8_t *in, uint16_t len, uint8_t *out)
{
    int k = 0;
    for (uint16_t i = 0; i < len; i++) {
        out[k++] = in[i];
        if (in[i] == 0xAA) out[k++] = 0xAA;
    }
    return k;
}

static int Destuff(const uint8_t *in, uint16_t len, uint8_t *out)
{
    int k = 0;
    for (uint16_t i = 0; i < len; i++) {
        out[k++] = in[i];
        if (in[i] == 0xAA && i + 1 < len && in[i + 1] == 0xAA) i++;
    }
    return k;
}

void PROTO_SendFrame(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    uint8_t body[MAX_BODY];
    uint8_t stuffed[MAX_STUFFED];
    uint16_t p = 0;

    body[p++] = cmd;
    if (payload && len) {
        if (len > (MAX_BODY - 3)) len = MAX_BODY - 3;
        memcpy(&body[p], payload, len);
        p += len;
    }

    uint16_t crc = CRC16(body, p);
    body[p++] = crc & 0xFF;
    body[p++] = crc >> 8;

    uint16_t s_len = Stuff(body, p, stuffed);
    if (s_len > MAX_STUFFED) return;

    uint16_t f = 0;
    s_tx_frame[f++] = HEADER0;
    s_tx_frame[f++] = HEADER1;
    memcpy(&s_tx_frame[f], stuffed, s_len);
    f += s_len;
    s_tx_frame[f++] = TAIL0;
    s_tx_frame[f++] = TAIL1;

    HAL_UART_Transmit_DMA(s_huart, s_tx_frame, f);
}

extern float PID_Kp, PID_Ki, PID_Kd;
extern float temperature_current, setpoint_current;
extern void SetSetpoint_Flash(float value);
extern void GetParamPID_Flash(float *Kp, float *Ki, float *Kd);
extern void SetParamPID_Flash(float Kp, float Ki, float Kd);

static void PROTO_User_GetPID(float *kp, float *ki, float *kd)
{
    GetParamPID_Flash(&PID_Kp, &PID_Ki, &PID_Kd);
    *kp = PID_Kp; *ki = PID_Ki; *kd = PID_Kd;
}

static void PROTO_User_GetData(float *sp, float *val)
{
    *sp = setpoint_current;
    *val = temperature_current;
}

static void PROTO_User_SetSetpoint(float sp)
{
    setpoint_current = sp;
    SetSetpoint_Flash(sp);
}

static void PROTO_User_SetPID(float kp, float ki, float kd)
{
    PID_Kp = kp; PID_Ki = ki; PID_Kd = kd;
    SetParamPID_Flash(PID_Kp, PID_Ki, PID_Kd);
}

void PROTO_Handle(const uint8_t *stf, uint16_t len_stf)
{
    uint8_t body[MAX_BODY];
    uint16_t blen = Destuff(stf, len_stf, body);
    if (blen < 3) return;

    uint8_t cmd = body[0];
    uint16_t crc_recv = body[blen - 2] | (body[blen - 1] << 8);
    uint16_t crc_calc = CRC16(body, blen - 2);
    if (crc_recv != crc_calc) return;

    const uint8_t *payload = &body[1];
    uint16_t plen = blen - 3;

    switch (cmd)
    {
    case CMD_CONNECT: {
        uint8_t pl[1] = {0xFF};
        PROTO_SendFrame(CMD_CONNECT, pl, 1);
        break;
    }
    case CMD_GET_PARAM: {
        float kp, ki, kd;
        PROTO_User_GetPID(&kp, &ki, &kd);
        int32_t ki_i = (int32_t)(ki * 1000);
        int32_t kp_i = (int32_t)(kp * 1000);
        int32_t kd_i = (int32_t)(kd * 1000);
        uint8_t pl[12];
        memcpy(&pl[0], &ki_i, 4);
        memcpy(&pl[4], &kp_i, 4);
        memcpy(&pl[8], &kd_i, 4);
        PROTO_SendFrame(CMD_GET_PARAM, pl, 12);
        break;
    }
    case CMD_GET_DATA: {
        float sp, val;
        PROTO_User_GetData(&sp, &val);
        int32_t sp_i = (int32_t)(sp * 1000);
        int32_t val_i = (int32_t)(val * 1000);
        uint8_t pl[8];
        memcpy(&pl[0], &sp_i, 4);
        memcpy(&pl[4], &val_i, 4);
        PROTO_SendFrame(CMD_GET_DATA, pl, 8);
        break;
    }
    case CMD_SAVE_SETPOINT:
        if (plen == 4) {
            int32_t sp_i;
            uint8_t pl[1] = {0xFF};
            memcpy(&sp_i, payload, 4);
            PROTO_User_SetSetpoint(sp_i / 1000.0f);
            PROTO_SendFrame(CMD_SAVE_SETPOINT, pl, 1);
        }
        break;

    case CMD_SAVE_PARAM:
        if (plen == 12) {
            int32_t ki_i, kp_i, kd_i;
            uint8_t pl[1] = {0xAB};
            memcpy(&ki_i, payload, 4);
            memcpy(&kp_i, payload + 4, 4);
            memcpy(&kd_i, payload + 8, 4);
            PROTO_User_SetPID(kp_i / 1000.0f, ki_i / 1000.0f, kd_i / 1000.0f);
            PROTO_SendFrame(CMD_SAVE_PARAM, pl, 1);
        }
        break;
    default:
        break;
    }
}

void HandleCMD(const uint8_t *frame, uint16_t len)
{
    if (len < 4) return;
    if (frame[0] != HEADER0 || frame[1] != HEADER1) return;
    if (frame[len - 2] != TAIL0 || frame[len - 1] != TAIL1) return;

    uint16_t payload_len = len - 4;
    if (payload_len > MAX_PAYLOAD) return;

    uint8_t buf_Frame[MAX_PAYLOAD];
    for (uint16_t i = 0; i < payload_len; i++)
        buf_Frame[i] = frame[i + 2];

    PROTO_Handle(buf_Frame, payload_len);
}
