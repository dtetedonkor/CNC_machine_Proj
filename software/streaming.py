#streaming.py
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
        timeout_per_line: float = 5.0,
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

    def run(self) -> None:
        self._emit_state(StreamState.SENDING)
        try:
            with serial.Serial(self.port, self.baudrate, timeout=self.read_timeout) as ser:
                time.sleep(self.startup_drain_time)
                ser.reset_input_buffer()

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

                    # Log what we are sending
                    self._emit_log(f">> {cmd}")
                    ser.write(payload)

                    deadline = time.time() + self.timeout_per_line
                    while time.time() < deadline:
                        resp = ser.readline()
                        if not resp:
                            continue

                        text = resp.decode("utf-8", errors="replace").strip()
                        if not text:
                            continue

                        # Log every controller line we receive
                        self._emit_log(f"<< {text}")

                        normalized = text.upper()
                        if normalized == "OK":
                            break
                        if normalized.startswith("ERROR"):
                            self._emit_error(line_index, cmd, text)
                            return
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
    # Remove ';' comments
    if ";" in line:
        line = line.split(";", 1)[0]
    # Remove bracket comments like (comment) - simple version
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
    ap.add_argument("--timeout", type=float, default=5.0, help="Seconds to wait for OK per line")
    ap.add_argument("--startup-delay", type=float, default=1.0, help="Delay after opening port")
    args = ap.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.1) as ser:
        # Give MCU time to reset after opening port (common on USB-serial)
        time.sleep(args.startup_delay)
        ser.reset_input_buffer()

        line_num = 0
        with open(args.file, "r", encoding="utf-8", errors="replace") as f:
            for raw in f:
                line_num += 1
                if is_comment_or_empty(raw):
                    continue

                cmd = strip_inline_comments(raw)
                if not cmd:
                    continue

                # Send with newline. Your firmware treats CR as LF, but LF is fine too.
                payload = (cmd + "\n").encode("ascii", errors="ignore")
                ser.write(payload)

                # Wait for OK
                deadline = time.time() + args.timeout
                got_ok = False
                while time.time() < deadline:
                    resp = ser.readline()
                    if not resp:
                        continue
                    text = resp.decode("utf-8", errors="replace").strip()
                    if not text:
                        continue
                    print(f"<< {text}")

                    # accept OK or ok (some firmwares vary)
                    if text.upper() == "OK":
                        got_ok = True
                        break

                if not got_ok:
                    raise RuntimeError(f"Timeout waiting for OK on line {line_num}: {cmd}")

                print(f">> {cmd}")

    print("Done.")


if __name__ == "__main__":
    main()