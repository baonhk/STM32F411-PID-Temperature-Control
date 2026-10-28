# CV Project Summary

## STM32F411 PID Temperature Control System

**Tech:** STM32F411, Embedded C, STM32 HAL, PID, TIM3 PWM, UART/DMA, SPI, W25Qxx Flash, DHT22, Python, Tkinter, PySerial, Matplotlib

### Recommended CV bullets

- Developed a closed-loop temperature controller on **STM32F411**, integrating DHT22 sensing, timer-driven PID control, and **TIM3 PWM** output for actuator control.
- Implemented **USART6 Receive-to-Idle DMA** and a custom binary protocol with **CRC16-Modbus** and byte stuffing; persisted setpoint and PID parameters in **W25Qxx SPI NOR Flash**.
- Built a **Python Tkinter/Matplotlib** desktop application for temperature monitoring, plotting, setpoint configuration, and PID gain tuning over serial communication.

### One-line version

Built an STM32F411 closed-loop temperature-control system combining PID/PWM firmware, UART-DMA communication, SPI Flash persistence, and a Python monitoring/tuning GUI.

### Interview talking points

- Why timer-driven control is preferable to an uncontrolled blocking loop.
- How PID output is bounded and converted to PWM.
- Why UART Receive-to-Idle DMA was selected for framed communication.
- How CRC16 and byte stuffing improve protocol robustness.
- How configuration is persisted in external SPI NOR Flash.
- How firmware and desktop software were debugged as one end-to-end system.

### Claims to avoid until measured on hardware

Do not add numerical claims for temperature accuracy, overshoot, settling time, steady-state error, heater efficiency, or reliability unless test logs are available to support them.
