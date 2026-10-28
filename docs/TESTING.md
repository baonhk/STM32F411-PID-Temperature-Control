# Hardware Validation Checklist

The repository contains the implemented firmware and host application. The GUI screenshot in the README uses **synthetic/demo data** and should not be interpreted as a measured thermal-response result.

## Current source-level validation

- Firmware contains DHT22 acquisition, PID logic, PWM control, LCD output, UART/DMA communication, CRC16 framing, and W25Qxx storage integration.
- Python GUI source is included for serial monitoring, plotting, setpoint control, and PID tuning.
- Generated build folders and temporary files are excluded from the portfolio repository.

## Recommended real-hardware test plan

When the physical setup is available, capture a CSV/log containing timestamp, measured temperature, setpoint, and PWM output.

Measure and document:

1. **Rise time** — time required to move from the initial temperature toward the target.
2. **Overshoot** — maximum temperature above the setpoint.
3. **Settling time** — time until temperature remains within a defined tolerance band.
4. **Steady-state error** — final difference between setpoint and measured temperature.
5. **Power-cycle persistence** — verify setpoint and PID gains are restored from W25Qxx Flash.
6. **Serial robustness** — test invalid CRC frames, partial frames, reconnects, and repeated command traffic.
7. **Sensor fault behavior** — verify the application handles invalid/missing DHT22 readings safely.

## Evidence to add later

A recruiter-ready hardware-results update should ideally include:

- one photo of the physical setup
- one real GUI screenshot
- one CSV or plot from an actual heating cycle
- measured PID response metrics
- a short demo video

Keeping measured results separate from demo data makes the repository technically credible and easy to discuss in an interview.
