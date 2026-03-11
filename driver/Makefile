PREFIX   ?= arm-none-eabi
CC       := $(PREFIX)-gcc
OBJCOPY  := $(PREFIX)-objcopy
SIZE     := $(PREFIX)-size

TOP            := ..
LIBOPENCM3_DIR := $(TOP)/lib/libopencm3

TARGET := cnc_uart_gcode
BUILD  := build

SRCS := \
	main.c \
	../src/serial_uart.c \
	../src/serial_gcode_bridge.c \
	../src/gcode.c \
	../src/arc.c \
	../src/kinematics.c \
	../drivers/stm32g474/board/board_init.c \
	../drivers/stm32g474/periph/drv_gpio.c \
	../drivers/stm32g474/periph/drv_inputs.c \
	../drivers/stm32g474/periph/drv_stepper_timer.c \
	../drivers/stm32g474/periph/drv_timebase.c \
	../drivers/stm32g474/periph/drv_uart.c \
	../drivers/stm32g474/hal/hal_impl.c \
	../drivers/stm32g474/hal/axis_motion.c

obj_for = $(BUILD)/$(subst /,_,$(basename $(1))).o
OBJS := $(foreach src,$(SRCS),$(call obj_for,$(src)))

# Cortex-M4F (STM32G474RE)
CPUFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS  += -O0 -g3 -Wall -Wextra
CFLAGS  += $(CPUFLAGS)
CFLAGS  += -ffunction-sections -fdata-sections
CFLAGS  += -I$(LIBOPENCM3_DIR)/include
CFLAGS  += -I$(TOP)/src
CFLAGS  += -I$(TOP)/drivers/stm32g474
CFLAGS  += -DSTM32G4

# Generic libopencm3 linker script + our memory layout
GENERIC_LD := $(LIBOPENCM3_DIR)/lib/cortex-m-generic.ld
MEMORY_LD  := stm32g474re-memory.ld

LDFLAGS += -T$(MEMORY_LD) -T$(GENERIC_LD)
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(CPUFLAGS)
LDFLAGS += -Wl,-Map=$(BUILD)/$(TARGET).map

# libopencm3 library
LIBNAME := opencm3_stm32g4
LDLIBS  += -L$(LIBOPENCM3_DIR)/lib -l$(LIBNAME)
LDLIBS  += -lm

.PHONY: all clean libopencm3 flash

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin

libopencm3:
	$(MAKE) -C $(LIBOPENCM3_DIR) TARGETS='stm32/g4' PREFIX=$(PREFIX)

$(BUILD):
	mkdir -p $(BUILD)

define compile_source
$(call obj_for,$(1)): $(1) | $(BUILD)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
endef
$(foreach src,$(SRCS),$(eval $(call compile_source,$(src))))

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
