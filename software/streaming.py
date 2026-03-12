import argparse
import time
import serial

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
