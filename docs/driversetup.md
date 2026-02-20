Here are **reproducible, step-by-step instructions** for **both Windows and Linux** to build + flash the same **libopencm3 blink** to a **NUCLEO-F446RE** (LD2 = **PA5**), using the exact flow that worked for you (Makefile builds libopencm3 STM32F4 + app, flashes with OpenOCD).

---

# Project layout (same on both OSes)

```
CNC_machine_Proj/
  lib/
    libopencm3/          (git submodule)
  driver/
    main.c
    Makefile
    stm32f446re-memory.ld
```

---

# 1) Source files (same on both)

## `driver/main.c`

```c
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void delay(volatile uint32_t n) {
    while (n--) __asm__("nop");
}

int main(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5); // LD2 = PA5

    while (1) {
        gpio_toggle(GPIOA, GPIO5);
        delay(2000000);
    }
}
```

## `driver/stm32f446re-memory.ld`

```ld
/* STM32F446RE: 512K Flash, 128K SRAM */
MEMORY
{
  rom (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
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

TARGET := blink
BUILD  := build

SRCS := main.c
OBJS := $(SRCS:%.c=$(BUILD)/%.o)

# Cortex-M4F (STM32F446RE)
CPUFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS  += -O0 -g3 -Wall -Wextra
CFLAGS  += $(CPUFLAGS)
CFLAGS  += -ffunction-sections -fdata-sections
CFLAGS  += -I$(LIBOPENCM3_DIR)/include
CFLAGS  += -DSTM32F4

# Generic libopencm3 linker script + our memory layout
GENERIC_LD := $(LIBOPENCM3_DIR)/lib/cortex-m-generic.ld
MEMORY_LD  := stm32f446re-memory.ld

LDFLAGS += -T$(MEMORY_LD) -T$(GENERIC_LD)
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(CPUFLAGS)
LDFLAGS += -Wl,-Map=$(BUILD)/$(TARGET).map

# libopencm3 library
LIBNAME := opencm3_stm32f4
LDLIBS  += -L$(LIBOPENCM3_DIR)/lib -l$(LIBNAME)

.PHONY: all clean libopencm3 flash

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin

# Build libopencm3 only for STM32F4 family
libopencm3:
	$(MAKE) -C $(LIBOPENCM3_DIR) TARGETS='stm32/f4' PREFIX=$(PREFIX)

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
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
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

LED **LD2 (PA5)** should blink.

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
