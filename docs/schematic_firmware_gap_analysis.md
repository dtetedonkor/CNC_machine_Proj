# Engraver Schematic → Firmware/Library Gap Analysis

## Scope

The board mapping in `drivers/stm32g474/board/board_pins.h` matches a 4-axis STEP/DIR design with:

- `PA0..PA3` = `X/Y/Z/A STEP`
- `PA4..PA7` = `X/Y/Z/A DIR`
- `PB0` = shared `DRV_EN` (active-low)
- `USART1` on `PA9/PA10`
- safety inputs on `PC13` (E-stop), `PC0..PC2` (limits), `PC3` (hand)

The schematic PDF in `docs/Print Schematic - EngraverSchematic.pdf` is image-based in this environment, so this analysis is grounded on the implemented board mapping and the current driver/core behavior.

## Improvements implemented

### 1) Core library motion pulse alignment

Updated `src/serial_gcode_bridge.c` to use `hal_stepper_pulse_mask()` during XY interpolation.  
This emits same-tick multi-axis pulses (X+Y together) instead of per-axis sequential pulse calls.

**Why this improves hardware utilization**

- Better exploits the hardware abstraction for atomic multi-axis STEP edges.
- Reduces axis-edge skew on diagonal moves.
- Aligns with timer/bitmask style GPIO backends in `drivers/stm32g474`.

### 2) Core safety response on live input faults

`serial_gcode_bridge_process_line()` now returns:

- `error: safety input active`
- non-OK status (`GCODE_ERR_INVALID_TARGET`)

when `E-stop` or any limit input triggers during active motion.

**Why this matters**

- Previously the move could be interrupted but still return `OK`, which can mislead host software.
- The new behavior keeps host state synchronized with physical machine safety events.

### 3) Driver hardening for UART read API

Updated `drivers/stm32g474/periph/drv_uart.c`:

- `drv_uart_read()` now validates `dst != NULL` and `cap > 0`.

**Why this matters**

- Prevents null dereference in integration/misuse cases.
- Improves driver robustness without changing normal behavior.

## Potential hardware/firmware flaws to track

1. **Shared enable line (`DRV_EN`) disables all axes together**  
   Useful for cost/complexity, but prevents per-axis power management and fault isolation.

2. **Input policy currently stop-only during motion**  
   Inputs are sampled and can abort motion, but recovery behavior (clear/latch/unlock policy) is still simple and should be formalized per machine safety requirements.

3. **Image-only schematic artifacts**  
   The PDF is not machine-readable here; maintain a text/BOM/netlist export in `docs/` to prevent pin-map drift and improve reviewability.

