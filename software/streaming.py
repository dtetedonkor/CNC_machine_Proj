# streaming.py
import argparse
import time
from dataclasses import dataclass
from enum import Enum, auto
from threading import Thread
from typing import Callable, Optional, Sequence

import serial

SETUP_ERROR_INDEX = -1


class StreamState(Enum):
    IDLE = auto()
    SENDING = auto()
    DONE = auto()
    ERROR = auto()


@dataclass
class StreamError:
    line_index: int
    line_text: str
    raw_line: str


class GrblStreamer(Thread):
    def __init__(
        self,
        port: str,
        baudrate: int,
        lines: Sequence[str],
        state_callback: Optional[Callable[[StreamState], None]] = None,
        error_callback: Optional[Callable[[StreamError], None]] = None,
        log_callback: Optional[Callable[[str], None]] = None,
        timeout_per_line: float = 15.0,
        startup_drain_time: float = 1.0,
        read_timeout: float = 0.1,
    ) -> None:
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.lines = lines
        self.state_callback = state_callback
        self.error_callback = error_callback
        self.log_callback = log_callback
        self.timeout_per_line = timeout_per_line
        self.startup_drain_time = startup_drain_time
        self.read_timeout = read_timeout

    def _emit_state(self, state: StreamState) -> None:
        if self.state_callback:
            self.state_callback(state)

    def _emit_log(self, text: str) -> None:
        if self.log_callback:
            self.log_callback(text)

    def _emit_error(self, line_index: int, line_text: str, raw_line: str) -> None:
        self._emit_state(StreamState.ERROR)
        if self.error_callback:
            self.error_callback(
                StreamError(
                    line_index=line_index,
                    line_text=line_text,
                    raw_line=raw_line,
                )
            )

    def _read_controller_lines(self, ser: serial.Serial, deadline: float, carry: str = ""):
        """
        Read raw bytes from serial until deadline and yield complete lines.
        Returns (yielded_lines, remaining_carry).
        """
        lines_out = []

        while time.monotonic() < deadline:
            chunk = ser.read(ser.in_waiting or 1)
            if not chunk:
                continue

            text = chunk.decode("utf-8", errors="replace")
            carry += text

            while "\n" in carry:
                line, carry = carry.split("\n", 1)
                line = line.strip()
                if line:
                    lines_out.append(line)

            if lines_out:
                break

        return lines_out, carry

    def _controller_alive_after_timeout(self, ser: serial.Serial, carry: str):
        """
        Poll controller status after an ACK timeout.
        If controller responds with <Idle|...> or <Run|...>, assume the command
        likely executed and the OK was missed.
        Returns (alive_ok, updated_carry, status_text_or_none)
        """
        try:
            ser.write(b"?")
            ser.flush()
        except Exception:
            return False, carry, None

        status_deadline = time.monotonic() + 1.0

        while time.monotonic() < status_deadline:
            lines_in, carry = self._read_controller_lines(ser, status_deadline, carry)
            if not lines_in:
                continue

            for text in lines_in:
                self._emit_log(f"<< {text}")
                upper = text.upper()

                if text.startswith("<"):
                    if "IDLE" in upper or "RUN" in upper:
                        return True, carry, text
                    if "ALARM" in upper:
                        return False, carry, text

                if upper.startswith("ERROR"):
                    return False, carry, text

        return False, carry, None

    def run(self) -> None:
        self._emit_state(StreamState.SENDING)
        try:
            with serial.Serial(self.port, self.baudrate, timeout=self.read_timeout) as ser:
                time.sleep(self.startup_drain_time)
                ser.reset_input_buffer()

                carry = ""

                for line_index, raw in enumerate(self.lines):
                    if is_comment_or_empty(raw):
                        continue

                    cmd = strip_inline_comments(raw)
                    if not cmd:
                        continue

                    try:
                        payload = (cmd + "\n").encode("ascii")
                    except UnicodeEncodeError as exc:
                        self._emit_error(line_index, cmd, f"Encoding error for '{cmd}': {exc}")
                        return

                    self._emit_log(f">> {cmd}")
                    ser.write(payload)
                    ser.flush()

                    deadline = time.monotonic() + self.timeout_per_line
                    got_ok = False

                    while time.monotonic() < deadline:
                        lines_in, carry = self._read_controller_lines(ser, deadline, carry)

                        if not lines_in:
                            continue

                        for text in lines_in:
                            normalized = text.upper()

                            # Do not spam UI with every plain OK if you don't want to.
                            # Keep it if you still want visibility.
                            self._emit_log(f"<< {text}")

                            if "OK" in normalized:
                                got_ok = True
                                break

                            if normalized.startswith("ERROR"):
                                self._emit_error(line_index, cmd, text)
                                return

                            # Ignore status reports/info lines as acknowledgements:
                            # <Idle|...>
                            # [MSG:...]
                            # startup banners, etc.

                        if got_ok:
                            break

                    if not got_ok:
                        recovered, carry, status_text = self._controller_alive_after_timeout(ser, carry)
                        if recovered:
                            self._emit_log(
                                f"[WARN] Missed OK for line {line_index + 1}; "
                                f"controller reported alive: {status_text}"
                            )
                            continue

                        if status_text:
                            self._emit_error(
                                line_index,
                                cmd,
                                f"Timeout waiting for OK; controller status: {status_text}"
                            )
                        else:
                            self._emit_error(line_index, cmd, "Timeout waiting for OK")
                        return

        except serial.SerialException as exc:
            self._emit_error(SETUP_ERROR_INDEX, "", f"Serial connection failed: {exc}")
            return
        except Exception as exc:
            self._emit_error(SETUP_ERROR_INDEX, "", str(exc))
            return

        self._emit_state(StreamState.DONE)


def is_comment_or_empty(line: str) -> bool:
    s = line.strip()
    if not s:
        return True
    if s.startswith(";"):
        return True
    if s.startswith("(") and s.endswith(")"):
        return True
    return False


def strip_inline_comments(line: str) -> str:
    if ";" in line:
        line = line.split(";", 1)[0]

    out = []
    in_paren = 0
    for ch in line:
        if ch == "(":
            in_paren += 1
            continue
        if ch == ")" and in_paren:
            in_paren -= 1
            continue
        if not in_paren:
            out.append(ch)

    return "".join(out).strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True, help="COM12 (Windows) or /dev/ttyACM0 (Linux)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--file", required=True, help="Path to .gcode file")
    ap.add_argument("--timeout", type=float, default=15.0, help="Seconds to wait for OK per line")
    ap.add_argument("--startup-delay", type=float, default=1.0, help="Delay after opening port")
    args = ap.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.1) as ser:
        time.sleep(args.startup_delay)
        ser.reset_input_buffer()

        carry = ""
        line_num = 0

        def read_controller_lines(deadline: float, carry_in: str = ""):
            lines_out = []
            while time.monotonic() < deadline:
                chunk = ser.read(ser.in_waiting or 1)
                if not chunk:
                    continue

                text = chunk.decode("utf-8", errors="replace")
                carry_in += text

                while "\n" in carry_in:
                    line, carry_in = carry_in.split("\n", 1)
                    line = line.strip()
                    if line:
                        lines_out.append(line)

                if lines_out:
                    break

            return lines_out, carry_in

        def controller_alive_after_timeout(carry_in: str):
            ser.write(b"?")
            ser.flush()

            status_deadline = time.monotonic() + 1.0

            while time.monotonic() < status_deadline:
                lines_in, carry_in = read_controller_lines(status_deadline, carry_in)
                if not lines_in:
                    continue

                for text in lines_in:
                    print(f"<< {text}")
                    upper = text.upper()

                    if text.startswith("<"):
                        if "IDLE" in upper or "RUN" in upper:
                            return True, carry_in, text
                        if "ALARM" in upper:
                            return False, carry_in, text

                    if upper.startswith("ERROR"):
                        return False, carry_in, text

            return False, carry_in, None

        with open(args.file, "r", encoding="utf-8", errors="replace") as f:
            for raw in f:
                line_num += 1

                if is_comment_or_empty(raw):
                    continue

                cmd = strip_inline_comments(raw)
                if not cmd:
                    continue

                payload = (cmd + "\n").encode("ascii", errors="ignore")
                print(f">> {cmd}")
                ser.write(payload)
                ser.flush()

                deadline = time.monotonic() + args.timeout
                got_ok = False

                while time.monotonic() < deadline:
                    lines_in, carry = read_controller_lines(deadline, carry)

                    if not lines_in:
                        continue

                    for line in lines_in:
                        print(f"<< {line}")
                        normalized = line.upper()

                        if "OK" in normalized:
                            got_ok = True
                            break

                        if normalized.startswith("ERROR"):
                            raise RuntimeError(f"Controller error on line {line_num}: {cmd} -> {line}")

                    if got_ok:
                        break

                if not got_ok:
                    recovered, carry, status_text = controller_alive_after_timeout(carry)
                    if recovered:
                        print(
                            f"[WARN] Missed OK for line {line_num}; "
                            f"controller reported alive: {status_text}"
                        )
                        continue

                    if status_text:
                        raise RuntimeError(
                            f"Timeout waiting for OK on line {line_num}: {cmd} "
                            f"(controller status: {status_text})"
                        )
                    raise RuntimeError(f"Timeout waiting for OK on line {line_num}: {cmd}")

    print("Done.")


if __name__ == "__main__":
    main() 