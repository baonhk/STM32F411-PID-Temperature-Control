import time
from collections import deque

import tkinter as tk
from tkinter import ttk, messagebox

from rs485 import Rs485Client
from plotwindow import PlotWindow

class DataModel:
    def __init__(self):
        self.ts_temp = deque()
        self.val_temp = deque()
        self.ts_set = deque()
        self.val_set = deque()
class IncubatorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("STM32F411 PID Temperature Control System")
        self.rs485 = Rs485Client()
        self.data_model = DataModel()
        self.poll_interval_ms = 500  # chu kỳ hỏi GetData
        self._build_ui()
        self._schedule_poll()
    # ----------------- UI -----------------
    def _build_ui(self):
        self.notebook = ttk.Notebook(self.root)
        self.tab_monitor = ttk.Frame(self.notebook)
        self.tab_settings = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_monitor, text="Monitoring")
        self.notebook.add(self.tab_settings, text="PID Settings")
        self.notebook.pack(fill=tk.BOTH, expand=True)
        self._build_tab_monitor()
        self._build_tab_settings()

    def _build_tab_monitor(self):
        top_frame = ttk.Frame(self.tab_monitor)
        top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        ttk.Label(top_frame, text="COM Port:").pack(side=tk.LEFT)
        self.combo_port = ttk.Combobox(top_frame, width=15, state="readonly")
        self._refresh_ports()
        self.combo_port.pack(side=tk.LEFT, padx=2)
        btn_refresh = ttk.Button(top_frame, text="Refresh", command=self._refresh_ports)
        btn_refresh.pack(side=tk.LEFT, padx=2)
        self.btn_connect = ttk.Button(top_frame, text="Connect", command=self._on_connect_clicked)
        self.btn_connect.pack(side=tk.LEFT, padx=5)
        ttk.Label(top_frame, text="Setpoint (°C):").pack(side=tk.LEFT, padx=(20, 2))
        self.entry_setpoint = ttk.Entry(top_frame, width=10)
        self.entry_setpoint.pack(side=tk.LEFT)
        btn_set_sp = ttk.Button(top_frame, text="Save Setpoint to Flash", command=self._on_save_setpoint)
        btn_set_sp.pack(side=tk.LEFT, padx=5)
        self.label_current = ttk.Label(top_frame, text="Temperature: -- °C, SP: -- °C")
        self.label_current.pack(side=tk.LEFT, padx=10)
        plot_frame = ttk.Frame(self.tab_monitor)
        plot_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.plot_window = PlotWindow(
            reader=self.data_model,
            master=plot_frame,
            window=20 * 60,
            interval_ms=200,
            startup_query=False
        )

    def _build_tab_settings(self):
        pad = 5
        frame = ttk.Frame(self.tab_settings)
        frame.pack(fill=tk.BOTH, expand=True, padx=pad, pady=pad)
        row = 0
        ttk.Label(frame, text="Kp:").grid(row=row, column=0, sticky="e", padx=2, pady=2)
        self.entry_kp = ttk.Entry(frame, width=10)
        self.entry_kp.grid(row=row, column=1, sticky="w", padx=2, pady=2)
        ttk.Label(frame, text="Ki:").grid(row=row, column=2, sticky="e", padx=2, pady=2)
        self.entry_ki = ttk.Entry(frame, width=10)
        self.entry_ki.grid(row=row, column=3, sticky="w", padx=2, pady=2)
        ttk.Label(frame, text="Kd:").grid(row=row, column=4, sticky="e", padx=2, pady=2)
        self.entry_kd = ttk.Entry(frame, width=10)
        self.entry_kd.grid(row=row, column=5, sticky="w", padx=2, pady=2)
        row += 1
        btn_read_pid = ttk.Button(frame, text="Read PID from STM32", command=self._on_read_pid)
        btn_read_pid.grid(row=row, column=0, columnspan=2, sticky="w", padx=2, pady=5)
        btn_save_pid = ttk.Button(frame, text="Save PID to Flash", command=self._on_save_pid)
        btn_save_pid.grid(row=row, column=2, columnspan=3, sticky="w", padx=2, pady=5)
        row += 1
        self.label_status_settings = ttk.Label(frame, text="PID parameters can be read from or saved to STM32 flash.")
        self.label_status_settings.grid(row=row, column=0, columnspan=6, sticky="w", padx=2, pady=5)

    # ----------------- COM / Connect -----------------
    def _refresh_ports(self):
        ports = Rs485Client.list_ports()
        self.combo_port["values"] = ports
        if ports:
            self.combo_port.current(0)

    def _on_connect_clicked(self):
        if self.rs485.is_connected:
            self.rs485.disconnect()
            self.btn_connect.config(text="Connect")
            messagebox.showinfo("Serial", "Disconnected.")
            return

        port = self.combo_port.get().strip()
        if not port:
            messagebox.showerror("Error", "No COM port selected.")
            return

        try:
            self.rs485.connect(port=port)
            self.btn_connect.config(text="Disconnect")
            messagebox.showinfo("Serial", f"Connected: {port}")
        except Exception as e:
            messagebox.showerror("Connection error", str(e))

    # ----------------- Setpoint -----------------
    def _on_save_setpoint(self):
        if not self.rs485.is_connected:
            messagebox.showerror("Error", "Serial connection is not active.")
            return
        try:
            sp = float(self.entry_setpoint.get().strip())
        except ValueError:
            messagebox.showerror("Error", "Invalid setpoint.")
            return

        try:
            self.rs485.cmd_save_setpoint(sp)
            messagebox.showinfo("Setpoint", f"Setpoint {sp:.3f} °C saved to W25Qxx SPI Flash.")
        except Exception as e:
            messagebox.showerror("Setpoint error", str(e))

    # ----------------- PID Settings -----------------
    def _on_read_pid(self):
        if not self.rs485.is_connected:
            messagebox.showerror("Error", "Serial connection is not active.")
            return
        try:
            kp, ki, kd = self.rs485.cmd_get_param()
            self.entry_kp.delete(0, tk.END)
            self.entry_kp.insert(0, f"{kp:.3f}")
            self.entry_ki.delete(0, tk.END)
            self.entry_ki.insert(0, f"{ki:.3f}")
            self.entry_kd.delete(0, tk.END)
            self.entry_kd.insert(0, f"{kd:.3f}")
            self.label_status_settings.config(text="PID parameters read from STM32.")
        except Exception as e:
            messagebox.showerror("PID read error", str(e))

    def _on_save_pid(self):
        if not self.rs485.is_connected:
            messagebox.showerror("Error", "Serial connection is not active.")
            return
        try:
            kp = float(self.entry_kp.get().strip())
            ki = float(self.entry_ki.get().strip())
            kd = float(self.entry_kd.get().strip())
        except ValueError:
            messagebox.showerror("Error", "Invalid Kp, Ki or Kd value.")
            return

        try:
            self.rs485.cmd_save_param(kp, ki, kd)
            self.label_status_settings.config(text="PID parameters saved to W25Qxx SPI Flash.")
            messagebox.showinfo("PID", "PID parameters saved to W25Qxx SPI Flash.")
        except Exception as e:
            messagebox.showerror("PID save error", str(e))

    # ----------------- Poll GetData -----------------
    def _schedule_poll(self):
        if self.rs485.is_connected:
            try:
                sp, val = self.rs485.cmd_get_data()
                now = time.time()
                self.data_model.ts_temp.append(now)
                self.data_model.val_temp.append(val)
                self.data_model.ts_set.append(now)
                self.data_model.val_set.append(sp)
                self.label_current.config(text=f"Temperature: {val:.2f} °C, SP: {sp:.2f} °C")
            except Exception:
                pass
        self.root.after(self.poll_interval_ms, self._schedule_poll)

def main():
    root = tk.Tk()
    app = IncubatorApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
