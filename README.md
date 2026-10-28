# STM32F411 PID Temperature Control System

![STM32](https://img.shields.io/badge/MCU-STM32F411-03234B?logo=stmicroelectronics&logoColor=white)
![Embedded C](https://img.shields.io/badge/Firmware-Embedded%20C-00599C)
![Python](https://img.shields.io/badge/Host-Python%20GUI-3776AB?logo=python&logoColor=white)
![Control](https://img.shields.io/badge/Control-PID-orange)

A portfolio-oriented embedded control project built around the **STM32F411**.  
The system acquires temperature data, executes a timer-driven discrete PID controller, generates PWM for an external heater/actuator stage, stores configuration in W25Qxx SPI NOR Flash, and communicates with a Python desktop application for monitoring and parameter tuning.

> **Recruiter note:** this repository focuses on firmware architecture, peripheral integration, control logic, serial protocol design, and PC-to-MCU integration. Performance figures such as overshoot, settling time, or temperature accuracy are intentionally not claimed because the repository does not include a complete hardware test dataset.

## Recruiter Snapshot

| Area | Implementation |
|---|---|
| MCU | STM32F411 |
| Firmware | Embedded C, STM32 HAL |
| Control | Discrete PID, timer-scheduled control loop |
| Actuation | TIM3 PWM output |
| Sensor | DHT22 temperature acquisition |
| Local UI | 16x2 LCD, 4-bit interface |
| Non-volatile storage | W25Qxx SPI NOR Flash |
| PC communication | USART6, 115200 baud, Receive-to-Idle DMA |
| Protocol | Custom binary frames, CRC16-Modbus, byte stuffing |
| Desktop tooling | Python, Tkinter, PySerial, Matplotlib |

## Demo

![Python GUI demo](docs/gui-demo.png)

**Demo-data disclosure:** the screenshot above demonstrates the GUI and plotting workflow using synthetic/demo temperature data. It is not presented as a measured hardware performance result.

## System Architecture

~~~mermaid
flowchart LR
    S[DHT22<br/>Temperature Sensor] -->|Temperature feedback| MCU[STM32F411<br/>Firmware]
    MCU --> PID[Discrete PID]
    PID --> PWM[TIM3 PWM]
    PWM --> H[External Heater / Actuator Stage]

    MCU --> LCD[16x2 LCD]
    MCU <-->|SPI| FLASH[W25Qxx<br/>SPI NOR Flash]

    PC[Python GUI<br/>Tkinter + Matplotlib] <-->|USART6 115200<br/>Receive-to-Idle DMA| MCU

    PC -. Setpoint / PID gains .-> MCU
    MCU -. Temperature / status .-> PC
~~~

A recruiter-friendly static version is also available below.\n\n![System architecture](docs/system-architecture.svg)

## How It Works

1. **Acquire temperature** from the DHT22 sensor.
2. **Compare** the measured value with the configured temperature setpoint.
3. **Execute the discrete PID controller** on a periodic timer-driven schedule.
4. **Apply the controller output** through TIM3 PWM to the external actuator/heater-control stage.
5. **Display local information** on the 16x2 LCD.
6. **Exchange data with the PC application** over USART6 while setpoint and PID gains can be persisted to W25Qxx Flash.

The control loop in the current firmware is scheduled at approximately **2 seconds**.

## Firmware Highlights

### Timer-driven control

The application separates periodic work from the main loop using timer events/software flags. Temperature acquisition and PID execution are therefore scheduled rather than implemented as a blocking desktop-style loop.

### PID and PWM

The controller calculates the error between the configured setpoint and measured temperature and produces a bounded control output. The current implementation maps this output to the TIM3 PWM range used by the firmware.

### UART with DMA

PC communication uses STM32 HAL Receive-to-Idle DMA reception. This reduces polling overhead and provides a practical foundation for framed serial communication.

### Persistent configuration

Setpoint and PID gains are stored in an external **W25Qxx SPI NOR Flash**, allowing configuration to survive a power cycle.

### Local display

A 16x2 character LCD provides local feedback for temperature and setpoint without requiring the PC application.

## Serial Protocol

The firmware and Python application exchange command-based binary frames.

~~~text
HEADER | COMMAND | PAYLOAD | CRC16 | TAIL
~~~

Frame delimiters used by the implementation:

~~~text
HEADER : AA 55
TAIL   : 55 AA
~~~

Integrity and framing features:

- CRC16-Modbus validation
- Byte stuffing
- Command-based request/response handling

Representative commands in the project:

| Command | Purpose |
|---|---|
| 0x0A | Connect / handshake |
| 0x0B | Read PID parameters |
| 0x0C | Read temperature and setpoint |
| 0x0D | Save temperature setpoint |
| 0x0F | Save PID parameters |

See **docs/PROTOCOL.md** for a concise protocol overview.

## Python Desktop Application

The host application is implemented with:

- **Tkinter** for the desktop UI
- **PySerial** for serial communication
- **Matplotlib** for real-time plotting

Main functions include:

- COM-port selection
- Connect / disconnect
- Temperature and setpoint monitoring
- Real-time plotting
- Setpoint update
- PID gain read/write

## Repository Structure

~~~text
.
├── firmware/              STM32F411 firmware project
├── pc-gui/                Python desktop application
├── docs/
│   ├── gui-demo.png
│   ├── system-architecture.svg
│   ├── PROTOCOL.md
│   └── TESTING.md
├── CV_PROJECT_SUMMARY.md
├── .gitignore
└── README.md
~~~

Build products, IDE metadata, Python cache files, and other generated artifacts are excluded where appropriate so the repository remains reviewable.

## Build and Run

### Firmware

1. Open the firmware project in **STM32CubeIDE**.
2. Review the CubeMX/IOC peripheral configuration.
3. Build the project for the STM32F411 target.
4. Flash the board using the normal STM32CubeIDE/ST-LINK workflow.
5. Connect the UART interface to the PC before starting the desktop application.

### Python GUI

Create a virtual environment if desired, then install the Python dependencies:

~~~bash
pip install -r pc-gui/requirements.txt
~~~

Run the GUI from the **pc-gui** directory using the project entry script included there.

Configure the serial port for the connected STM32 interface. The firmware uses **115200 baud**.

## Engineering Validation

This repository distinguishes between **implemented functionality** and **measured performance**.

Implemented in source:

- DHT22 acquisition
- Discrete PID logic
- Timer/PWM control
- USART6 Receive-to-Idle DMA
- Custom framed protocol with CRC16
- W25Qxx parameter storage
- LCD output
- Python serial GUI and plotting

Not claimed without additional hardware logs:

- absolute temperature accuracy
- overshoot percentage
- settling time
- steady-state error
- heater power or thermal efficiency

See **docs/TESTING.md** for the recommended hardware validation checklist.

## What This Project Demonstrates

For an Embedded Software / Firmware role, this project demonstrates practical experience with:

- MCU peripheral configuration and integration
- GPIO, timers, PWM, UART, DMA, and SPI
- sensor-driver integration
- closed-loop control logic
- serial protocol design and CRC validation
- non-volatile configuration storage
- host-side tooling for embedded systems
- debugging across firmware and desktop software boundaries

## CV Summary

**STM32F411 PID Temperature Control System**  
STM32F411 · Embedded C · PID · PWM · UART/DMA · SPI Flash · Python

- Developed a closed-loop temperature controller on STM32F411, integrating DHT22 sensing, timer-driven PID control, and TIM3 PWM output for actuator control.
- Implemented USART6 Receive-to-Idle DMA and a custom binary protocol with CRC16-Modbus and byte stuffing; persisted setpoint and PID parameters in W25Qxx SPI NOR Flash.
- Built a Python Tkinter/Matplotlib desktop application for temperature monitoring, plotting, setpoint configuration, and PID gain tuning over serial communication.

---

**Target roles:** Embedded Software Intern · Firmware Intern · Embedded Systems Intern
