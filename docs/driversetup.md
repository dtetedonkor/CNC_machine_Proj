Here are **reproducible, step-by-step instructions** for **both Windows and Linux** to build + flash a **libopencm3 TMC2209 STEP pulse demo** on **STM32G491RE** (4x STEP/DIR + shared EN), using the same flow (Makefile builds libopencm3 STM32G4 + app, flashes with OpenOCD).

---

# Project layout (same on both OSes)

```
CNC_machine_Proj/
  lib/
    libopencm3/          (git submodule)
  driver/
    main.c
    Makefile
    stm32g491re-memory.ld
```

---

# Driver module layout (current firmware)

The original `driver/main.c` below is still useful as a low-level STEP/DIR smoke test, but the firmware now has dedicated driver-facing modules in `src/`:

- `serial_uart` (`src/serial_uart.h/.c`)
  - UART transport ring buffers (RX/TX), CR/LF line handling, and optional backspace handling.
- `protocol` (`src/protocol.h/.c`)
  - Line intake/validation, `ok/error`-style acceptance boundaries, and realtime command detection (`?`, `!`, `~`, Ctrl-X).
- `state_machine` (`src/state_machine.h/.c`) + `system_state` (`src/system_state.h/.c`)
  - Runtime machine states (idle/run/hold/alarm) and feed-hold/resume/reset control points.
- `io_limits_estop_hand` (`src/io_limits_estop_hand.h/.c`)
  - Debounced limit/E-stop sampling and latched E-stop behavior for safety.
- `hal` (`src/hal.h`)
  - Hardware boundary for GPIO/timers/serial/spindle/limits.

For board bring-up:
1. Use `driver/main.c` to verify physical STEP/DIR/EN wiring and pulse polarity.
2. Then integrate the HAL implementation used by the modules above for full serial-to-motion behavior.

---

# 1) Source files (same on both)

## `driver/main.c`

```c
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

/* STEP: PA0..PA3, DIR: PA4..PA7, EN (active-low): PB0 */
#define STEP_PINS_MASK (GPIO0 | GPIO1 | GPIO2 | GPIO3)
#define DIR_PINS_MASK  (GPIO4 | GPIO5 | GPIO6 | GPIO7)
#define EN_PIN         GPIO0
#define PULSE_COUNT_PER_BURST 200U
#define STEP_PULSE_DELAY_CYCLES 4000U
#define DIR_CHANGE_DELAY_CYCLES 1200000U

static void delay_cycles(volatile uint32_t cycles) {
    while (cycles--) __asm__("nop");
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STEP_PINS_MASK | DIR_PINS_MASK);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, EN_PIN);
    gpio_clear(GPIOB, EN_PIN);
}

int main(void) {
    gpio_setup();
    gpio_set(GPIOA, DIR_PINS_MASK);

    while (1) {
        for (uint32_t i = 0; i < PULSE_COUNT_PER_BURST; i++) {
            gpio_set(GPIOA, STEP_PINS_MASK);
            delay_cycles(STEP_PULSE_DELAY_CYCLES);
            gpio_clear(GPIOA, STEP_PINS_MASK);
            delay_cycles(STEP_PULSE_DELAY_CYCLES);
        }
        delay_cycles(DIR_CHANGE_DELAY_CYCLES);
        gpio_toggle(GPIOA, DIR_PINS_MASK);
        delay_cycles(DIR_CHANGE_DELAY_CYCLES);
    }
}
```

## `driver/stm32g491re-memory.ld`

```ld
/* STM32G491RE: 512K Flash, 112K SRAM */
MEMORY
{
  rom (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  ram (rwx) : ORIGIN = 0x20000000, LENGTH = 112K
}
```

## `driver/Makefile`

```make
PREFIX   ?= arm-none-eabi
CC       := $(PREFIX)-gcc
OBJCOPY  := $(PREFIX)-objcopy
SIZE     := $(PREFIX)-size

TOP            := ..
LIBOPENCM3_DIR := $(TOP)/lib/libopencm3

TARGET := tmc2209_pulse
BUILD  := build

SRCS := main.c
OBJS := $(SRCS:%.c=$(BUILD)/%.o)

# Cortex-M4F (STM32G491RE)
CPUFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS  += -O0 -g3 -Wall -Wextra
CFLAGS  += $(CPUFLAGS)
CFLAGS  += -ffunction-sections -fdata-sections
CFLAGS  += -I$(LIBOPENCM3_DIR)/include
CFLAGS  += -DSTM32G4

# Generic libopencm3 linker script + our memory layout
GENERIC_LD := $(LIBOPENCM3_DIR)/lib/cortex-m-generic.ld
MEMORY_LD  := stm32g491re-memory.ld

LDFLAGS += -T$(MEMORY_LD) -T$(GENERIC_LD)
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(CPUFLAGS)
LDFLAGS += -Wl,-Map=$(BUILD)/$(TARGET).map

# libopencm3 library
LIBNAME := opencm3_stm32g4
LDLIBS  += -L$(LIBOPENCM3_DIR)/lib -l$(LIBNAME)

.PHONY: all clean libopencm3 flash

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin

# Build libopencm3 only for STM32G4 family
libopencm3:
	$(MAKE) -C $(LIBOPENCM3_DIR) TARGETS='stm32/g4' PREFIX=$(PREFIX)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: libopencm3 $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD)

flash: $(BUILD)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program $(BUILD)/$(TARGET).elf verify reset exit"
```

---

# 2) Add libopencm3 (same on both)

From `CNC_machine_Proj/`:

```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
mkdir -p lib
git submodule add https://github.com/libopencm3/libopencm3.git libopencm3
```

If already added:

```bash
git submodule update --init 
```

---

# Windows instructions (MSYS2 MinGW64)

## A) Install MSYS2 + packages

1. Install MSYS2 to `C:\msys64`
2. Open **Start → MSYS2 MinGW x64** (prompt should say `MINGW64`)

Update:

```bash
pacman -Syu
# reopen terminal if prompted
pacman -Syu
```

Install tools:

```bash
pacman -S --needed git make python mingw-w64-x86_64-openocd mingw-w64-x86_64-arm-none-eabi-toolchain
```

Verify:

```bash
arm-none-eabi-gcc --version
openocd --version
```

## B) Build + flash

```bash
cd /c/Users/nana1/cnc-core/CNC_machine_Proj/driver
make clean
make
make flash
```

Expected OpenOCD success lines:

* `** Programming Finished **`
* `** Verified OK **`
* `** Resetting Target **`

You should see STEP activity on **PA0..PA3** (logic analyzer/scope) for TMC2209 testing.

---

# Linux instructions (Ubuntu/Debian-style)

## A) Install packages

```bash
sudo apt update
sudo apt install -y git make openocd gcc-arm-none-eabi binutils-arm-none-eabi
```

Verify:

```bash
arm-none-eabi-gcc --version
openocd --version
```

> Note: Some distros also need `gdb-multiarch` if you later want debugging, but not required for flashing.

## B) Build + flash

From your project root:

```bash
cd ~/path/to/CNC_machine_Proj/driver
make clean
make
make flash
```

## C) If OpenOCD can’t access ST-LINK (Linux permissions)

If you see “permission denied” or it can’t open the USB device:

1. Create a udev rule:

```bash
sudo tee /etc/udev/rules.d/49-stlinkv2.rules >/dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE:="0666", GROUP:="plugdev"
EOF
```

2. Reload rules and replug the board:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Then unplug/replug and retry:

```bash
make flash
```

---

# “Known good” commands summary

## Windows (MSYS2)

```bash
pacman -S --needed git make python mingw-w64-x86_64-openocd mingw-w64-x86_64-arm-none-eabi-toolchain
cd /c/Users/nana1/cnc-core/CNC_machine_Proj/driver
make clean && make && make flash
```

## Linux (Ubuntu/Debian)

```bash
sudo apt install -y git make openocd gcc-arm-none-eabi binutils-arm-none-eabi
cd ~/.../CNC_machine_Proj/driver
make clean && make && make flash
```

---

If you want, I can format this into a **single README.md** with two sections (“Windows (MSYS2)” and “Linux (Ubuntu)”) that your teammates can follow exactly.
