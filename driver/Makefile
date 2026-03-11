# Toolchain "triplet" (no trailing dash)
TOOLCHAIN ?= arm-none-eabi

# Accept PREFIX from environment, but normalize it:
# - if user passes PREFIX=arm-none-eabi- -> TOOLCHAIN becomes arm-none-eabi
# - if user passes PREFIX=arm-none-eabi  -> TOOLCHAIN stays arm-none-eabi
ifneq ($(origin PREFIX), undefined)
TOOLCHAIN := $(patsubst %- ,%,$(PREFIX))
TOOLCHAIN := $(patsubst %-,%,$(TOOLCHAIN))
endif

CC       := $(TOOLCHAIN)-gcc
OBJCOPY  := $(TOOLCHAIN)-objcopy
SIZE     := $(TOOLCHAIN)-size
TOP            := ..
LIBOPENCM3_DIR := $(TOP)/lib/libopencm3

TARGET := cnc_uart_gcode
BUILD  := build
GRBL_BUILD := $(BUILD)-grbl
GRBL_TOOLCHAIN := $(abspath $(TOP)/cmake/toolchains/arm-none-eabi-gcc.cmake)

SRCS := \
	main.c \
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

# Cortex-M4F (STM32G491RE / STM32G4 family)
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
MEMORY_LD  ?= stm32g491re-memory.ld

LDFLAGS += -T$(MEMORY_LD) -T$(GENERIC_LD)
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(CPUFLAGS)
LDFLAGS += -Wl,-Map=$(BUILD)/$(TARGET).map
LDFLAGS += --specs=nosys.specs
LDFLAGS += --specs=nano.specs

# libopencm3 library
LIBNAME := opencm3_stm32g4
LDLIBS  += -L$(LIBOPENCM3_DIR)/lib -l$(LIBNAME)
LDLIBS  += -L$(GRBL_BUILD) -lgrbl
LDLIBS  += -lm

.PHONY: all clean libopencm3 grbl flash

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin

libopencm3:
	$(MAKE) -C $(LIBOPENCM3_DIR) TARGETS='stm32/g4' PREFIX=$(TOOLCHAIN)-

$(GRBL_BUILD)/CMakeCache.txt: $(TOP)/CMakeLists.txt $(GRBL_TOOLCHAIN)
	cmake -S $(TOP) -B $(GRBL_BUILD) \
		-DCMAKE_TOOLCHAIN_FILE=$(GRBL_TOOLCHAIN)

grbl: $(GRBL_BUILD)/CMakeCache.txt
	cmake --build $(GRBL_BUILD) --target grbl

$(BUILD):
	mkdir -p $(BUILD)

define compile_source
$(call obj_for,$(1)): $(1) | $(BUILD)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
endef
$(foreach src,$(SRCS),$(eval $(call compile_source,$(src))))

$(BUILD)/$(TARGET).elf: libopencm3 grbl $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@


clean:
	rm -rf $(BUILD) $(GRBL_BUILD)

flash: $(BUILD)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program $(BUILD)/$(TARGET).elf verify reset exit"
