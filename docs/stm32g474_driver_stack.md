# STM32G474 Driver Stack Layout

This target keeps the core CNC logic isolated from libopencm3 symbols by splitting STM32G474 code into board, peripheral, and HAL glue layers.

## Directory layout

```text
/drivers/stm32g474/
  board/   -> pin map + board bring-up
  periph/  -> UART, GPIO, timebase, input, stepper timer drivers
  hal/     -> implementation of src/hal.h for the core
```

## Board pin map (`board/board_pins.h`)

Logical outputs exposed to the core:

- `X/Y/Z/A STEP`
- `X/Y/Z/A DIR`
- `DRV_EN`
- UART (`USART2`)
- Conditioned inputs (`E-stop`, `limit x/y/z`, `hand`)

The board map is centralized in `board_pins.h` so core code uses axis/input abstractions, not raw MCU pin names.

## Step timing strategy

The initial implementation uses a fixed tick timer (`TIM6`) with `BOARD_STEP_TICK_HZ = 25 kHz`:

1. ISR clears any previously asserted step pins.
2. ISR calls `core_step_tick_isr()` callback to get a step bitmask.
3. ISR raises required step pins.
4. Pins are lowered on the next tick, guaranteeing pulse width equal to one tick period.

This matches the recommended v1 strategy and keeps ISR work bounded.

## HAL glue boundary

`drivers/stm32g474/hal/hal_impl.c` implements `src/hal.h` and is the only place the core touches platform behavior.

Core modules should continue to use only `src/hal.h`.
