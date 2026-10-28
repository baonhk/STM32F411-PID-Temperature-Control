# Firmware

This directory contains the application firmware for the STM32F411 temperature-control project.

## Source of truth

The application code was consolidated from the uploaded STM32 projects:

- `PID_LoNhiet_STM32F411-main` — integrated PID application
- `ThuVienLoNhiet_STM32F411-main` — temperature-control library/reference project
- `Flash_STM32-main` — W25Qxx SPI Flash reference project

The integrated PID project is the main firmware baseline. The other projects are development/reference sources and are not duplicated as separate applications in this portfolio repository.

## MCU and peripherals

- STM32F411CEUx
- HSE 25 MHz, PLL SYSCLK 100 MHz, HCLK 50 MHz
- DHT22 on PA10
- SPI1 on PA5/PA6/PA7, W25Qxx CS on PA4
- USART6 TX/RX on PA11/PA12
- USART6 RX/TX DMA on DMA2 Stream1/Stream6
- TIM3 CH1 PWM on PB4
- TIM9 periodic scheduler interrupt
- TIM10 microsecond timing for DHT22
- 16x2 LCD in 4-bit mode

## Recreate the CubeIDE project

The repository intentionally does not commit ST's full generated HAL/CMSIS vendor tree.

1. Open `PID_1.ioc` in STM32CubeMX or STM32CubeIDE.
2. Select **STM32CubeIDE** as the toolchain.
3. Generate code using STM32Cube FW_F4.
4. Keep/restore the application files under `Core/Src` and `Core/Inc`.
5. Build the Debug configuration.
6. Flash using ST-LINK.

The `.ioc`, application source, interrupt/MSP support and linker configuration are versioned so the project can be reconstructed without committing build output.

## Application modules

- `main.c` — scheduler, PID, PWM, application flow and persistent parameters
- `proto.c/.h` — UART Receive-to-Idle DMA, framed protocol, CRC16-Modbus and byte stuffing
- `dht22.c/.h` — sensor acquisition and checksum handling
- `w25qxx.c/.h` — SPI NOR Flash read/write/erase
- `lcd.c/.h` — local 16x2 LCD
- `stm32f4xx_hal_msp.c` — peripheral GPIO/DMA/interrupt setup
- `stm32f4xx_it.c/.h` — interrupt handlers

Generated folders such as `Debug/`, `Release/` and the ST vendor `Drivers/` tree are intentionally excluded from the portfolio.
