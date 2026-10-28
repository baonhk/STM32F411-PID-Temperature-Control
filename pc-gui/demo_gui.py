"""Runnable portfolio GUI demo for the STM32F411 PID temperature-control project.

This script intentionally uses synthetic temperature data so it can be launched
without hardware. It demonstrates the desktop monitoring/tuning layout only.
Replace the demo data path with the project's validated serial protocol when the
STM32 hardware is connected.
"""

import math
import random
import time
import tkinter as tk
from tkinter import ttk

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure


class TemperatureGuiDemo(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("STM32F411 PID Temperature Control — Demo")
        self.geometry("1100x680")
        self.minsize(900, 600)

        self.start_time = time.time()
        self.setpoint = tk.DoubleVar(value=37.0)
        self.kp = tk.DoubleVar(value=2.0)
        self.ki = tk.DoubleVar(value=0.15)
        self.kd = tk.DoubleVar(value=1.0)
        self.temp = tk.DoubleVar(value=34.0)
        self.pwm = tk.DoubleVar(value=0.0)
        self.times = []
        self.temps = []

        self._build_ui()
        self.after(400, self._tick)

    def _build_ui(self):
        outer = ttk.Frame(self, padding=18)
        outer.pack(fill="both", expand=True)

        header = ttk.Frame(outer)
        header.pack(fill="x", pady=(0, 14))
        ttk.Label(
            header,
            text="STM32F411 PID Temperature Control",
            font=("Segoe UI", 20, "bold"),
        ).pack(side="left")
        ttk.Label(
            header,
            text="DEMO MODE · SYNTHETIC DATA",
            foreground="#b91c1c",
            font=("Segoe UI", 10, "bold"),
        ).pack(side="right")

        body = ttk.Frame(outer)
        body.pack(fill="both", expand=True)

        controls = ttk.LabelFrame(body, text="Control & PID parameters", padding=14)
        controls.pack(side="left", fill="y", padx=(0, 14))

        ttk.Label(controls, text="Setpoint (°C)").grid(row=0, column=0, sticky="w")
        ttk.Entry(controls, textvariable=self.setpoint, width=12).grid(
            row=1, column=0, sticky="ew", pady=(3, 12)
        )

        for row, (label, var) in enumerate(
            (("Kp", self.kp), ("Ki", self.ki), ("Kd", self.kd)), start=2
        ):
            ttk.Label(controls, text=label).grid(row=row, column=0, sticky="w", pady=(6, 0))
            ttk.Entry(controls, textvariable=var, width=12).grid(row=row, column=1, padx=(8, 0))

        ttk.Button(controls, text="Apply parameters", command=self._apply).grid(
            row=5, column=0, columnspan=2, sticky="ew", pady=(18, 8)
        )
        ttk.Separator(controls).grid(row=6, column=0, columnspan=2, sticky="ew", pady=12)

        ttk.Label(controls, text="PWM output").grid(row=7, column=0, sticky="w")
        self.pwm_bar = ttk.Progressbar(controls, maximum=100, variable=self.pwm, length=210)
        self.pwm_bar.grid(row=8, column=0, columnspan=2, sticky="ew", pady=(5, 4))
        self.pwm_label = ttk.Label(controls, text="0 %")
        self.pwm_label.grid(row=9, column=0, sticky="w")

        ttk.Label(
            controls,
            text="Portfolio demo only\n(no hardware required)",
            foreground="#64748b",
        ).grid(row=10, column=0, columnspan=2, sticky="w", pady=(28, 0))

        right = ttk.Frame(body)
        right.pack(side="left", fill="both", expand=True)

        telemetry = ttk.LabelFrame(right, text="Telemetry", padding=14)
        telemetry.pack(fill="x")

        self.temp_label = ttk.Label(
            telemetry, text="34.0 °C", font=("Segoe UI", 28, "bold")
        )
        self.temp_label.pack(side="left")

        self.sp_label = ttk.Label(
            telemetry, text="Setpoint: 37.0 °C", font=("Segoe UI", 14)
        )
        self.sp_label.pack(side="right")

        chart = ttk.LabelFrame(right, text="Temperature response", padding=8)
        chart.pack(fill="both", expand=True, pady=(14, 0))

        self.fig = Figure(figsize=(7.0, 4.6), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_xlabel("Time (s)")
        self.ax.set_ylabel("Temperature (°C)")
        self.ax.grid(True, alpha=0.25)
        (self.temp_line,) = self.ax.plot([], [], label="Temperature")
        (self.sp_line,) = self.ax.plot([], [], linestyle="--", label="Setpoint")
        self.ax.legend(loc="lower right")

        self.canvas = FigureCanvasTkAgg(self.fig, master=chart)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

    def _apply(self):
        self.sp_label.configure(text=f"Setpoint: {self.setpoint.get():.1f} °C")

    def _tick(self):
        now = time.time() - self.start_time
        sp = self.setpoint.get()
        current = self.temp.get()

        # Simple first-order synthetic thermal response plus a small disturbance.
        error = sp - current
        pwm = max(0.0, min(100.0, 13.0 * error + 18.0))
        current += 0.055 * error + random.uniform(-0.03, 0.03)

        # Small settling ripple so the demo looks like a real sampled signal.
        current += 0.015 * math.sin(now / 2.7)

        self.temp.set(current)
        self.pwm.set(pwm)
        self.temp_label.configure(text=f"{current:.2f} °C")
        self.pwm_label.configure(text=f"{pwm:.0f} %")

        self.times.append(now)
        self.temps.append(current)
        if len(self.times) > 150:
            self.times = self.times[-150:]
            self.temps = self.temps[-150:]

        self.temp_line.set_data(self.times, self.temps)
        self.sp_line.set_data(self.times, [sp] * len(self.times))
        self.ax.relim()
        self.ax.autoscale_view()
        self.ax.set_ylim(min(self.temps + [sp]) - 0.6, max(self.temps + [sp]) + 0.6)
        self.canvas.draw_idle()

        self.after(400, self._tick)


if __name__ == "__main__":
    app = TemperatureGuiDemo()
    app.mainloop()
