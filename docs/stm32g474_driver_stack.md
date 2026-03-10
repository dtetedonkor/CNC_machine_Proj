# STM32G474 Driver Stack Layout

This target keeps the core CNC logic isolated from libopencm3 symbols by splitting STM32G474 code into board, peripheral, and HAL glue layers.

> Note: STM32G4 family peripherals are similar across parts (for example STM32G491 and STM32G474), but always verify final alternate-function pin mapping and instance availability against the STM32G474 datasheet/reference manual.

## Directory layout

```text
/core/                     -> parser, planner, protocol, kinematics, state machine
/drivers/stm32g474/
  board/                   -> pin map + board bring-up
  periph/                  -> UART, GPIO, timebase, input, stepper timer drivers
  hal/                     -> implementation of src/hal.h for the core
```

Current STM32G474 files in this repository:

```text
drivers/stm32g474/
  board/board_pins.h
  board/board_init.[ch]
  periph/drv_uart.[ch]
  periph/drv_timebase.[ch]
  periph/drv_gpio.[ch]
  periph/drv_inputs.[ch]
  periph/drv_stepper_timer.[ch]
  hal/hal_impl.c
```

## HAL boundary contract

`drivers/stm32g474/hal/hal_impl.c` implements `src/hal.h` and is the only place core code touches platform behavior.

Core modules should continue to use only `src/hal.h` and never include libopencm3 headers directly.

## Driver responsibilities (STM32G474)

The STM32G474 driver layer in this repository is intentionally a hardware adapter (not a CNC logic layer):

1. **Hardware initialization**
   - `hal_init()` wires boot order through board/peripheral init functions (`board_init_*`, `drv_*_init`).
2. **UART communication**
   - `drv_uart` handles USART1 setup, RX IRQ buffering, and TX write.
3. **Stepper signal control**
   - `drv_gpio` owns STEP/DIR/EN pin writes.
4. **Accurate step timing**
   - `drv_stepper_timer` (TIM6 tick ISR) clears previous pulses and emits next-step masks.
5. **Safety input monitoring**
   - Inputs are sampled via `drv_gpio_read_inputs_raw` / `drv_inputs`.
   - TIM6 ISR now also gates pulse generation on limit/E-stop activity and forces driver disable.

### What the driver does not do

The driver layer does **not** parse G-code, plan paths, or compute feed/acceleration math.
Those behaviors remain in core modules (`protocol`, `gcode`, planner/kinematics/stepper logic).

## Board pin map (`board/board_pins.h`)

Logical outputs exposed to the core:

- `X/Y/Z/A STEP`
- `X/Y/Z/A DIR`
- `DRV_EN`
- UART (`USART1`)
- Conditioned inputs (`E-stop`, `limit x/y/z`, `hand`)

The board map is centralized in `board_pins.h` so core code uses axis/input abstractions, not raw MCU pin names.

## UART/G-code streaming plan

The firmware path is designed as:

1. UART IRQ receives host bytes into a ring buffer (`drv_uart`).
2. Main loop/protocol assembles full lines (`\n`/`\r\n`).
3. Parser converts lines to canonical motion/spindle/control commands.
4. Planner queues segments.
5. Step generator consumes segments and emits step pulses.

The current UART layer is non-blocking on RX and provides stream-oriented APIs suitable for GRBL-style `ok/error` protocol handling.

## Step timing strategy (v1 implemented)

The initial implementation uses a fixed tick timer (`TIM6`) with `BOARD_STEP_TICK_HZ = 25 kHz`:

1. ISR clears any previously asserted step pins.
2. ISR calls `core_step_tick_isr()` callback to get a step bitmask.
3. ISR raises required step pins.
4. Pins are lowered on the next tick, guaranteeing pulse width equal to one tick period.

This matches the recommended v1 strategy and keeps ISR work bounded.

## Step generation evolution path

- **v1 (current):** fixed-tick interrupt DDA style step scheduling.
- **v2 (future):** variable-interval compare scheduling for smoother/high-speed motion.
- **v3 (future):** timer+DMA compare update for maximal determinism.

## Safety/state behavior

Drivers should preserve these runtime expectations:

- `DRV_EN` defaults disabled at boot.
- E-stop and limits are sampled/debounced through input driver.
- Step timer callback can be gated by state machine (IDLE/RUN/HOLD/ALARM policy in core/state modules).
- Alarm path should quickly prevent further pulse emission and disable motor enables.

## Boot/init order checklist

Recommended order for STM32G474 bring-up:

1. Reset/clock setup (`board_init_clocks`)
2. GPIO safe-state setup (`drv_gpio_init_safe`)
3. UART pin/peripheral setup (`drv_uart_init`)
4. Timebase setup (`drv_timebase_init`)
5. Input init/debounce state (`drv_inputs_init`)
6. Step timer setup/start (`drv_stepper_timer_init` / `drv_stepper_timer_start`)
7. Enter protocol/planner loop

## Bring-up milestones

1. UART boot banner + line echo.
2. `ok/error` framing for command lines.
3. Motor enable/disable command path.
4. Single-axis jog from internal test command.
5. Multi-axis interpolation and safety-triggered abort.
