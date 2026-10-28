import time
import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt

class PlotWindow:
    def __init__(self, reader, master=None,
                 window=20 * 60,
                 interval_ms=200,
                 startup_query=False,
                 query_bytes=b"\xAA\xFF",
                 query_delay_ms=80):
        self.reader = reader
        self.window = window
        self.interval_ms = interval_ms
        self.startup_query = startup_query
        self.query_bytes = query_bytes
        self.query_delay_ms = query_delay_ms

        if master is None:
            self.root = tk.Toplevel()
            self.root.title("Đồ thị nhiệt độ lò ấp trứng")
        else:
            self.root = master
        self.fig, self.ax = plt.subplots(figsize=(6, 4))
        self.ax.set_title("Nhiệt độ lò ấp trứng")
        self.ax.set_xlabel("Thời gian (s)")
        self.ax.set_ylabel("Nhiệt độ (°C)")
        self.ax.grid(True)
        self.ax.set_ylim(20, 100)
        self.line_temp, = self.ax.plot([], [], lw=1.5, label="Temp")
        self.line_set,  = self.ax.plot([], [], lw=1.5, linestyle="--", label="Setpoint")
        self.ax.legend()
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        self.status_var = tk.StringVar(value="")
        if isinstance(self.root, tk.Toplevel):
            self.status_lbl = tk.Label(self.root, textvariable=self.status_var, anchor="w")
            self.status_lbl.pack(fill=tk.X)
        else:
            self.status_lbl = None
        self.t0 = None
        if self.startup_query and isinstance(self.root, tk.Toplevel):
            self.root.after(self.query_delay_ms, self._send_startup_query)
        self.root.after(self.interval_ms, self.update_plot)

    def _send_startup_query(self):
        ser = getattr(self.reader, "ser", None)
        if ser is None:
            if self.status_var is not None:
                self.status_var.set("Serial chưa sẵn sàng để gửi query.")
            return
        try:
            try:
                ser.reset_input_buffer()
            except Exception:
                pass
            ser.write(self.query_bytes)
            ser.flush()
            if self.status_var is not None:
                self.status_var.set(
                    f"Đã gửi query khởi động: {self.query_bytes.hex(' ').upper()}"
                )
        except Exception as e:
            if self.status_var is not None:
                self.status_var.set(f"Lỗi gửi query: {e}")

    def update_plot(self):
        r = self.reader
        win = self.window
        if not (getattr(r, "ts_temp", None) or getattr(r, "ts_set", None)):
            self.canvas.draw_idle()
            self.root.after(self.interval_ms, self.update_plot)
            return
        last_t = None
        if r.ts_temp:
            last_t = r.ts_temp[-1]
        if r.ts_set:
            if last_t is None or r.ts_set[-1] > last_t:
                last_t = r.ts_set[-1]
        if last_t is None:
            self.canvas.draw_idle()
            self.root.after(self.interval_ms, self.update_plot)
            return
        if self.t0 is None:
            self.t0 = last_t
        elapsed = last_t - self.t0
        if elapsed > win:
            t_cut = last_t - win
            if r.ts_temp:
                while r.ts_temp and r.ts_temp[0] < t_cut:
                    r.ts_temp.popleft()
                    r.val_temp.popleft()
            if r.ts_set:
                while r.ts_set and r.ts_set[0] < t_cut:
                    r.ts_set.popleft()
                    r.val_set.popleft()
        if r.ts_temp:
            x_temp = [t - self.t0 for t in r.ts_temp]
            self.line_temp.set_data(x_temp, list(r.val_temp))
        if r.ts_set:
            x_set = [t - self.t0 for t in r.ts_set]
            self.line_set.set_data(x_set, list(r.val_set))
        if elapsed <= win:
            xmax = max(20.0, elapsed)
            self.ax.set_xlim(0.0, xmax)
        else:
            self.ax.set_xlim(elapsed - win, elapsed)
        self.canvas.draw_idle()
        self.root.after(self.interval_ms, self.update_plot)
