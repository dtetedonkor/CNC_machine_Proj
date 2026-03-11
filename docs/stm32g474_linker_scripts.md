# STM32G474 linker scripts

This project links STM32G474 firmware with:

1. A **project memory layout script** (`driver/stm32g474re-memory.ld`)
2. The **generic libopencm3 Cortex-M script** (`lib/libopencm3/lib/cortex-m-generic.ld`)

The driver `Makefile` passes both scripts to the linker:

```make
LDFLAGS += -T$(MEMORY_LD) -T$(GENERIC_LD)
```

## Script in this repository

### `driver/stm32g474re-memory.ld`

```ld
/* STM32G474RE: 512K Flash, 128K SRAM */
MEMORY
{
  rom (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
```

This file defines the physical memory regions used by `.text`, `.data`, `.bss`, stack, and heap placement from the generic Cortex-M linker script.

## Build usage

From the repository root:

```bash
cd driver
make clean
make
```

The firmware build now defaults to:

```make
MEMORY_LD := stm32g474re-memory.ld
```

## If your exact STM32G474 variant differs

If you use a part with a different Flash/SRAM size than `STM32G474RE`, duplicate `stm32g474re-memory.ld`, update `LENGTH` fields, and point `MEMORY_LD` in `driver/Makefile` to the new file.
