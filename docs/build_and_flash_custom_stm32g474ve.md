# Build and Flash Guide (Custom STM32G474VE + TMC2209)

This guide documents the step-by-step process for:

1. building the **core library**
2. building the **STM32 driver firmware**
3. flashing firmware onto a **custom STM32G474VE board** over SWD

---

## 0) Hardware prerequisites (custom board)

Because this is a custom board (not a Nucleo), expose these SWD signals on a header/test pads:

- `SWDIO`
- `SWCLK`
- `NRST`
- `GND`
- `3.3V` (VTref/sense)

Typical ST-LINK wiring:

- ST-LINK `SWDIO` -> board `SWDIO`
- ST-LINK `SWCLK` -> board `SWCLK`
- ST-LINK `NRST`  -> board `NRST`
- ST-LINK `GND`   -> board `GND`
- ST-LINK `VTref` -> board `3.3V`

Bring-up checks before flashing:

- MCU 3.3V rail is present/stable.
- SWD pins are not overloaded by external circuitry.
- Programmer and board share ground.
- `NRST` is accessible.

---

## 1) Install toolchain (Linux)

```bash
sudo apt update
sudo apt install -y git make openocd gcc-arm-none-eabi binutils-arm-none-eabi
```

Verify tools:

```bash
arm-none-eabi-gcc --version
openocd --version
```

---

## 2) Initialize repository

From repository root:

```bash
cd <repository_root>
```

---

## 3) Build the core library

You can build the core support library from repository root:

```bash
cd <repository_root>
cmake -S . -B /tmp/cnc-core-build
cmake --build /tmp/cnc-core-build
```

This builds the host-side static core library.

---

## 4) Build the STM32 HAL firmware

The firmware source in this repository is at:

- `<repository_root>/drivers/main.c`

Build this file using your STM32 HAL project/tooling (for example STM32CubeIDE or a CubeMX-generated HAL project configured for STM32G491RE/STM32G474).

---

## 5) Flash firmware to STM32G474VE using ST-LINK (SWD)

Connect ST-LINK to board SWD signals, then flash the HAL project output (`.elf`/`.bin`) using your normal STM32 flow (STM32CubeProgrammer, STM32CubeIDE, or OpenOCD).

If Linux permission errors occur, add ST-LINK udev rule (see `docs/driversetup.md`) and reconnect ST-LINK.

---

## 6) Post-flash smoke test (recommended)

Use UART as a separate connection from SWD.

1. Open serial terminal at `115200 8N1`.
2. Reset/power cycle board.
3. Confirm shell banner appears:
   - `STM32 shell ready`
4. Confirm prompt appears:
   - `> `
5. Type `help` and press Enter, then verify:
   - echo line `[RX] help`
   - `commands: help, $, gcode lines`
   - `ok`

---

## 7) Recommended first-board validation sequence

Before full machining flow, validate in this order:

1. SWD flash works repeatedly.
2. UART terminal communication is stable.
3. UART output repeats once per second.
4. Clock and reset behavior are stable across power cycles.
