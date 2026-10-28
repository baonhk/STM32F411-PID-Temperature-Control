# Serial Protocol Overview

The STM32 firmware and Python GUI use a small command-based binary protocol over USART.

## Link configuration

- Peripheral: **USART6**
- Baud rate: **115200**
- Firmware receive path: **HAL Receive-to-Idle DMA**

## Frame format

~~~text
HEADER | COMMAND | PAYLOAD | CRC16 | TAIL
~~~

Delimiters used by the implementation:

~~~text
HEADER : AA 55
TAIL   : 55 AA
~~~

The protocol includes **CRC16-Modbus** for integrity checking and **byte stuffing** to protect reserved framing bytes.

## Representative commands

| Command | Purpose |
|---|---|
| 0x0A | Connect / handshake |
| 0x0B | Read PID parameters |
| 0x0C | Read temperature and setpoint |
| 0x0D | Save temperature setpoint |
| 0x0F | Save PID parameters |

## Design intent

The protocol keeps the MCU/PC interface deterministic and lightweight while allowing the desktop application to request telemetry and update controller configuration.

For implementation details, review the firmware communication source and the Python serial/protocol module in **pc-gui**.
