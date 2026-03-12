# STM32 HAL driver setup (STM32G491RE / STM32G4 family)

This repository no longer uses the old third-party submodule-based bring-up path.

The current embedded bring-up source in-repo is:

- `drivers/main.c`

That file is an STM32 HAL UART command shell/front-end that:

- prints a boot banner (`STM32 shell ready`)
- prints a prompt (`> `)
- accepts line input over UART
- routes non-local lines into `src/core.[ch]` (`core_submit_line`)
- returns protocol/core responses (`OK` or `error:<code/text>`)

## Quick verification flow

1. Open the HAL project for your board (for example, STM32CubeIDE/CubeMX-generated project).
2. Use `drivers/main.c` as the application source.
3. Build and flash to STM32G491RE/STM32G474-family hardware.
4. Open a serial terminal at `115200 8N1`.
5. Confirm the shell banner appears:
   - `STM32 shell ready`
6. Confirm prompt appears:
   - `> `
7. Type `help` + Enter and verify command response + `ok`.
8. Type `G0 X10` and verify an `OK` response from the core path.
9. Type `G99` and verify an `error` response.

## Notes

- Core CNC logic remains under `src/` and should not include MCU-vendor headers directly.
- Keep hardware-specific setup isolated in the HAL driver layer.
- STM32 target glue lives under `platform/stm32g474/` (`board_stm32g474.*`, `hal_stm32.*`).
