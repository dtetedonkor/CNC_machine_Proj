from __future__ import annotations

from pathlib import Path
import sys
import time

from streaming import GrblStreamer, StreamError, StreamState


# ------------------ CONFIG: CHANGE THESE ------------------
GCODE_FILE = "dog.gcode"   # can be "dog.gcode" (if you run from software/), or an absolute path
PORT = "COM12"             # e.g. "COM11" on Windows, "/dev/ttyACM0" on Linux
BAUDRATE = 115200
# Optional GRBL/grblHAL preamble:
PREAMBLE = ["$X", "G21", "G90"]
STARTUP_DRAIN_TIME = 2.0
TIMEOUT_PER_LINE = 5.0
# ----------------------------------------------------------


def main() -> int:
    gcode_path = Path(GCODE_FILE)

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

    print("----- Standalone GRBL Streaming Test -----")
    print(f"File: {gcode_path}")
    print(f"Port: {PORT}")
    print(f"Baud: {BAUDRATE}")
    print(f"Preamble: {PREAMBLE}")
    print("------------------------------------------")

    # Callbacks
    def on_state(state: StreamState) -> None:
        print(f"[STATE] {state.name}")

    def on_error(err: StreamError) -> None:
        if err.line_index >= 0:
            print(
                "\n[ERROR]\n"
                f"Line {err.line_index + 1}: {err.line_text}\n"
                f"Controller: {err.raw_line}\n",
                file=sys.stderr,
            )
        else:
            print(f"\n[ERROR]\n{err.raw_line}\n", file=sys.stderr)

    def on_log(text: str) -> None:
        # text is already formatted like ">> ..." or "<< ..."
        print(text)

    streamer = GrblStreamer(
        port=PORT,
        baudrate=BAUDRATE,
        lines=lines,
        state_callback=on_state,
        error_callback=on_error,
        log_callback=on_log,
        startup_drain_time=STARTUP_DRAIN_TIME,
        timeout_per_line=TIMEOUT_PER_LINE,
    )

    # You can either call streamer.run() (blocking) or streamer.start() (thread).
    # For a simple standalone script, blocking is easiest:
    streamer.run()

    # Give stdout a moment to flush in some terminals
    time.sleep(0.05)

    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())