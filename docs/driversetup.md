# STM32 HAL driver setup (STM32G491RE / STM32G4 family)

This repository no longer uses the old third-party submodule-based bring-up path.

The current embedded bring-up source in-repo is:

- `drivers/main.c`

That file is an STM32 HAL UART smoke test which transmits:

- `Hello World\r\n`

at `115200 8N1` in a loop.

## Quick verification flow

1. Open the HAL project for your board (for example, STM32CubeIDE/CubeMX-generated project).
2. Use `drivers/main.c` as the application source.
3. Build and flash to STM32G491RE/STM32G474-family hardware.
4. Open a serial terminal at `115200 8N1`.
5. Confirm `Hello World` repeats about once per second.

## Notes

- Core CNC logic remains under `src/` and should not include MCU-vendor headers directly.
- Keep hardware-specific setup isolated in the HAL driver layer.
