# TMC2209 firmware stack (STM32G474)

This project drives TMC2209 in **STEP/DIR/EN** mode.

## What firmware controls (and what it does not)

Firmware controls:
- `X_STEP`, `Y_STEP`, `Z_STEP`, `A_STEP`
- `X_DIR`, `Y_DIR`, `Z_DIR`, `A_DIR`
- `DRV_EN`

Firmware does **not** directly drive motor coils (`M1A/M1B/M2A/M2B`), and does not
implement commutation/chopping/microstep waveform generation. That is handled inside
each TMC2209 device.

## Implemented modules

1. **GPIO motor output**
   - `drivers/stm32g474/periph/drv_gpio.[ch]`
   - Configures STEP/DIR/EN GPIO and provides step-mask + direction + enable APIs.

2. **Timer step generator**
   - `drivers/stm32g474/periph/drv_stepper_timer.[ch]` (TIM6 ISR)
   - `drivers/stm32g474/hal/axis_motion.[ch]` (DDA pulse scheduling)
   - TIM6 calls `core_step_tick_isr()` and emits deterministic step masks.

3. **Axis motion driver**
   - `drivers/stm32g474/hal/axis_motion.[ch]`
   - Converts XY step requests into timer-ticked pulses (primary/minor axis stepping).

4. **UART receive driver**
   - `drivers/stm32g474/periph/drv_uart.[ch]`
   - `src/serial_uart.[ch]`
   - Receives host serial lines and provides buffered line parsing.

5. **G-code parser + translator**
   - `src/gcode.[ch]`
   - `src/serial_gcode_bridge.[ch]`
   - Parses command lines and converts movement commands to step-domain requests.

6. **Main integration**
   - `driver/main.c`
   - Registers a timer-driven motion backend and runs the UART/G-code command loop.

## First bring-up milestone flow

1. Initialize board/HAL and timer ISR.
2. Enable drivers with `M17`.
3. Send test move (example: `G0 X1`).
4. Confirm timer-driven STEP pulses appear on axis STEP pin(s).
5. Confirm driver disable with `M18`.

Optional future enhancement: TMC2209 UART/PDN configuration for current/diagnostics.
