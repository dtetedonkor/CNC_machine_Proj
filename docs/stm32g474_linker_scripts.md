# STM32G474 linker configuration

Linker scripts are now managed by the STM32 HAL project/toolchain you use to build firmware.

For STM32G491RE/STM32G474-family targets, ensure your HAL project linker settings match your board memory map.

## Recommended checks

1. Verify flash origin and size for your exact MCU variant.
2. Verify SRAM origin and size for your exact MCU variant.
3. Keep startup file, linker script, and clock configuration aligned to the same MCU part number.
4. Re-verify after changing package variant or stepping to a different STM32G4 part.
