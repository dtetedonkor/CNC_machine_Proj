from __future__ import annotations

from pathlib import Path
import sys
import time
from datetime import datetime
import argparse

from streaming import GrblStreamer, StreamError, StreamState


# ------------------ CONFIG: CHANGE THESE ------------------
GCODE_FILE = "output.gcode"  # can be "output.gcode" (if you run from software/), or an absolute path
PORT = "COM12"              # e.g. "COM11" on Windows, "/dev/ttyACM0" on Linux
BAUDRATE = 115200

# Optional GRBL/grblHAL preamble:
# NOTE: This is PREPENDED before the file contents.
# For debugging connectivity, consider setting PREAMBLE = ["?"] or PREAMBLE = [].
PREAMBLE = ["$X", "G21", "G90"]

STARTUP_DRAIN_TIME = 2.0
TIMEOUT_PER_LINE = 5.0
# ----------------------------------------------------------


def _default_log_path() -> Path:
    ts = datetime.now().strftime("%Y%m%d-%H%M%S")
    here = Path(__file__).resolve().parent
    return here / "logs" / f"stream-{ts}.log"


def main() -> int:
    ap = argparse.ArgumentParser(description="Standalone GRBL streaming test with file logging.")
    ap.add_argument("--file", default=GCODE_FILE, help="Path to .gcode file")
    ap.add_argument("--port", default=PORT, help="COM12 (Windows) or /dev/ttyACM0 (Linux)")
    ap.add_argument("--baud", type=int, default=BAUDRATE, help="Baudrate (e.g. 115200)")
    ap.add_argument("--timeout", type=float, default=TIMEOUT_PER_LINE, help="Seconds to wait for OK per line")
    ap.add_argument("--startup-delay", type=float, default=STARTUP_DRAIN_TIME, help="Delay after opening port")
    ap.add_argument(
        "--log-file",
        default="",
        help="Optional log file path. If omitted, writes to software/logs/stream-YYYYMMDD-HHMMSS.log",
    )
    args = ap.parse_args()

    gcode_path = Path(args.file)

    # If user runs this script from a different working directory,
    # try to resolve relative path relative to *this file's* folder.
    if not gcode_path.is_absolute():
        here = Path(__file__).resolve().parent
        candidate = here / gcode_path
        if candidate.exists():
            gcode_path = candidate

    if not gcode_path.exists():
        print(f"ERROR: G-code file not found: {gcode_path}", file=sys.stderr)
        return 2

    if gcode_path.suffix.lower() != ".gcode":
        print(f"ERROR: Not a .gcode file: {gcode_path}", file=sys.stderr)
        return 2

    try:
        with open(gcode_path, "r", encoding="utf-8", errors="replace") as fh:
            file_lines = [ln.rstrip("\n") for ln in fh]
    except Exception as e:
        print(f"ERROR: Could not read {gcode_path}: {e}", file=sys.stderr)
        return 2

    lines = list(PREAMBLE) + file_lines

    log_path = Path(args.log_file) if args.log_file else _default_log_path()
    log_path.parent.mkdir(parents=True, exist_ok=True)

    log_lines: list[str] = []

    def emit(line: str) -> None:
        # Print to terminal immediately
        print(line)
        # Buffer for file
        log_lines.append(line)

    def emit_err(line: str) -> None:
        # Print to terminal immediately
        print(line, file=sys.stderr)
        log_lines.append(line)

    emit("----- Standalone GRBL Streaming Test -----")
    emit(f"File: {gcode_path}")
    emit(f"Port: {args.port}")
    emit(f"Baud: {args.baud}")
    emit(f"Preamble: {PREAMBLE}")
    emit(f"Startup delay: {args.startup_delay}")
    emit(f"Timeout per line: {args.timeout}")
    emit(f"Log file: {log_path}")
    emit("------------------------------------------")

    had_error = {"value": False}

    # Callbacks
    def on_state(state: StreamState) -> None:
        emit(f"[STATE] {state.name}")

    def on_error(err: StreamError) -> None:
        had_error["value"] = True
        if err.line_index >= 0:
            emit_err(f"[ERROR] Line {err.line_index + 1}: {err.line_text}")
            emit_err(f"[ERROR] Controller: {err.raw_line}")
        else:
            emit_err(f"[ERROR] {err.raw_line}")

    def on_log(text: str) -> None:
        # text is already formatted like ">> ..." or "<< ..."
        emit(text)

    streamer = GrblStreamer(
        port=args.port,
        baudrate=args.baud,
        lines=lines,
        state_callback=on_state,
        error_callback=on_error,
        log_callback=on_log,
        startup_drain_time=args.startup_delay,
        timeout_per_line=args.timeout,
    )

    streamer.run()

    # Give stdout a moment to flush in some terminals
    time.sleep(0.05)

    # Always write the log file at the end
    try:
        log_path.write_text("\n".join(log_lines) + "\n", encoding="utf-8")
    except Exception as e:
        print(f"ERROR: Failed to write log file {log_path}: {e}", file=sys.stderr)
        return 2

    if had_error["value"]:
        emit("Done (with errors).")
        return 1

    emit("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())