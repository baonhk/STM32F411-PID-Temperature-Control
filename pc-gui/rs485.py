import serial
import serial.tools.list_ports
from typing import Optional, Tuple


HEADER = b"\xAA\x55"
TAIL   = b"\x55\xAA"
STUFF_BYTE = 0xAA

# CMD codes
CMD_CONNECT      = 0x0A
CMD_GET_PARAM    = 0x0B
CMD_GET_DATA     = 0x0C
CMD_SAVE_SETPT   = 0x0D
CMD_SAVE_PARAM   = 0x0F


def crc16_modbus(buf: bytes) -> int:
    crc = 0xFFFF
    for b in buf:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def stuff_bytes(raw: bytes) -> bytes:
    out = bytearray()
    for b in raw:
        out.append(b)
        if b == STUFF_BYTE:
            out.append(STUFF_BYTE)
    return bytes(out)


def destuff_bytes(stuffed: bytes) -> bytes:
    out = bytearray()
    i = 0
    n = len(stuffed)
    while i < n:
        b = stuffed[i]
        out.append(b)
        if b == STUFF_BYTE and i + 1 < n and stuffed[i + 1] == STUFF_BYTE:
            i += 2
        else:
            i += 1
    return bytes(out)


class Rs485Client:
    def __init__(self, port: Optional[str] = None,
                 baudrate: int = 115200,
                 timeout: float = 0.2):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser: Optional[serial.Serial] = None

    @staticmethod
    def list_ports() -> list[str]:
        return [p.device for p in serial.tools.list_ports.comports()]

    @property
    def is_connected(self) -> bool:
        return self.ser is not None and self.ser.is_open

    def connect(self, port: Optional[str] = None, baudrate: Optional[int] = None) -> None:
        if port is not None:
            self.port = port
        if baudrate is not None:
            self.baudrate = baudrate
        if not self.port:
            raise ValueError("Chưa chọn cổng COM.")

        if self.is_connected:
            self.disconnect()

        self.ser = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
        try:
            self._send_frame(CMD_CONNECT, b"")
        except Exception:
            pass

    def disconnect(self) -> None:
        if self.ser is not None:
            try:
                self.ser.close()
            except Exception:
                pass
        self.ser = None

    HEADER = b"\xAA\x55"

    def _build_frame(self, cmd: int, payload: bytes = b"") -> bytes:
        if not (0 <= cmd <= 0xFF):
            raise ValueError("CMD phải là 1 byte.")
        body = bytes([cmd]) + payload
        crc = crc16_modbus(body)
        body_with_crc = body + crc.to_bytes(2, "little", signed=False)
        body_stuffed = stuff_bytes(body_with_crc)
        frame = HEADER + body_stuffed + TAIL
        return frame

    def _send_frame(self, cmd: int, payload: bytes = b"") -> None:
        if not self.is_connected:
            raise RuntimeError("Serial chưa kết nối.")
        frame = self._build_frame(cmd, payload)
        try:
            self.ser.reset_input_buffer()
        except Exception:
            pass
        self.ser.write(frame)
        self.ser.flush()

    def _recv_frame(self, expected_cmd: Optional[int] = None,
                    timeout: Optional[float] = None) -> Tuple[int, bytes]:
        if not self.is_connected:
            raise RuntimeError("Serial chưa kết nối.")

        ser = self.ser
        old_timeout = ser.timeout
        if timeout is not None:
            ser.timeout = timeout

        try:
            buf = bytearray()
            while True:
                b = ser.read(1)
                if not b:
                    raise TimeoutError("Timeout chờ HEADER.")
                buf += b
                if buf.endswith(HEADER):
                    break

            body_stuffed = bytearray()
            tail_len = len(TAIL)
            while True:
                b = ser.read(1)
                if not b:
                    raise TimeoutError("Timeout chờ TAIL.")
                body_stuffed += b
                if len(body_stuffed) >= tail_len and body_stuffed[-tail_len:] == TAIL:
                    break

            body_stuffed = body_stuffed[:-tail_len]
            body = destuff_bytes(body_stuffed)

            if len(body) < 3:
                raise ValueError("Frame quá ngắn.")

            cmd = body[0]
            data_crc = body[1:]
            payload = data_crc[:-2]
            crc_recv = int.from_bytes(data_crc[-2:], "little", signed=False)
            crc_calc = crc16_modbus(bytes([cmd]) + payload)
            if crc_recv != crc_calc:
                raise ValueError("CRC sai.")

            if expected_cmd is not None and cmd != expected_cmd:
                raise ValueError(f"CMD không khớp. Nhận {cmd:#x}, mong {expected_cmd:#x}")

            return cmd, payload
        finally:
            ser.timeout = old_timeout

    def _exchange(self, cmd: int, payload: bytes = b"",
                  wait_reply: bool = True,
                  expected_cmd: Optional[int] = None,
                  timeout: float = 0.5) -> Optional[bytes]:
        self._send_frame(cmd, payload)
        if not wait_reply:
            return None
        if expected_cmd is None:
            expected_cmd = cmd
        _, pl = self._recv_frame(expected_cmd=expected_cmd, timeout=timeout)
        return pl

    def cmd_connect(self) -> None:
        self._send_frame(CMD_CONNECT, b"")

    def cmd_get_param(self) -> Tuple[float, float, float]:
        pl = self._exchange(CMD_GET_PARAM, b"", wait_reply=True, expected_cmd=CMD_GET_PARAM)
        if len(pl) != 12:
            raise ValueError("Payload GetParam không đúng 12 byte.")
        ki_i = int.from_bytes(pl[0:4], "little", signed=True)
        kp_i = int.from_bytes(pl[4:8], "little", signed=True)
        kd_i = int.from_bytes(pl[8:12], "little", signed=True)
        return kp_i / 1000.0, ki_i / 1000.0, kd_i / 1000.0

    def cmd_get_data(self) -> Tuple[float, float]:
        pl = self._exchange(CMD_GET_DATA, b"", wait_reply=True, expected_cmd=CMD_GET_DATA)
        if len(pl) != 8:
            raise ValueError("Payload GetData không đúng 8 byte.")
        sp_i = int.from_bytes(pl[0:4], "little", signed=True)
        val_i = int.from_bytes(pl[4:8], "little", signed=True)
        return sp_i / 1000.0, val_i / 1000.0

    def cmd_save_setpoint(self, setpoint: float) -> None:
        sp_i = int(setpoint * 1000.0)
        pl = sp_i.to_bytes(4, "little", signed=True)
        self._exchange(CMD_SAVE_SETPT, pl, wait_reply=False)

    def cmd_save_param(self, kp: float, ki: float, kd: float) -> None:
        ki_i = int(ki * 1000.0)
        kp_i = int(kp * 1000.0)
        kd_i = int(kd * 1000.0)
        pl = (
            ki_i.to_bytes(4, "little", signed=True) +
            kp_i.to_bytes(4, "little", signed=True) +
            kd_i.to_bytes(4, "little", signed=True)
        )
        self._exchange(CMD_SAVE_PARAM, pl, wait_reply=False)
