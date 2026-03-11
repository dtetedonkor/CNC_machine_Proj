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

## 2) Get libopencm3 submodule

From repository root:

```bash
cd /home/runner/work/CNC_machine_Proj/CNC_machine_Proj
git submodule update --init --recursive
```

---

## 3) Build the core library

You can build the core support library from repository root:

```bash
cd /home/runner/work/CNC_machine_Proj/CNC_machine_Proj
make libopencm3
```

This uses the top-level `Makefile` and builds the STM32G4 family target in:

- `lib/libopencm3`

Optional (host-side static core library via CMake):

```bash
cd /home/runner/work/CNC_machine_Proj/CNC_machine_Proj
cmake -S . -B /tmp/cnc-core-build
cmake --build /tmp/cnc-core-build
```

---

## 4) Build the STM32 driver firmware

```bash
cd /home/runner/work/CNC_machine_Proj/CNC_machine_Proj/driver
make clean
make
```

Expected outputs:

- `/home/runner/work/CNC_machine_Proj/CNC_machine_Proj/driver/build/cnc_uart_gcode.elf`
- `/home/runner/work/CNC_machine_Proj/CNC_machine_Proj/driver/build/cnc_uart_gcode.bin`

The linker script starts firmware at internal flash base address:

- `0x08000000`

---

## 5) Flash firmware to STM32G474VE using ST-LINK (SWD)

Connect ST-LINK to board SWD signals, then run:

```bash
cd /home/runner/work/CNC_machine_Proj/CNC_machine_Proj/driver
make flash
```

This runs OpenOCD and programs/verifies/resets the target.

If Linux permission errors occur, add ST-LINK udev rule (see `docs/driversetup.md`) and reconnect ST-LINK.

---

## 6) Post-flash smoke test (recommended)

Use UART as a separate connection from SWD for G-code commands.

1. Open serial terminal at `115200 8N1`.
2. Reset/power cycle board.
3. Confirm startup message:
   - `CNC ready`
4. Send simple commands:
   - `M17` (enable drivers) -> expect `OK`
   - `G0 X1` -> expect `OK` and observe STEP/DIR activity
   - `M18` (disable drivers) -> expect `OK`

---

## 7) Recommended first-board validation sequence

Before full machining flow, validate in this order:

1. SWD flash works repeatedly.
2. UART terminal communication is stable.
3. Driver enable pin (`DRV_EN`) toggles correctly.
4. One axis direction pin toggles correctly.
5. One axis STEP pulse train is observable on scope/logic analyzer.
6. Then run multi-axis and full G-code tests.
