# ---- Toolchain prefix (GCC ARM Embedded) ----
PREFIX ?= arm-none-eabi

# ---- Path to libopencm3 submodule ----
LIBOPENCM3_DIR := lib/libopencm3

# ---- Build only STM32G4 family (covers STM32G491RE) ----
LIBOPENCM3_TARGETS := stm32/g4

.PHONY: all libopencm3 clean clean-libopencm3

all: libopencm3

# Build only the requested family, not the whole world
libopencm3:
	$(MAKE) -C $(LIBOPENCM3_DIR) PREFIX=$(PREFIX) TARGETS='$(LIBOPENCM3_TARGETS)'

clean-libopencm3:
	$(MAKE) -C $(LIBOPENCM3_DIR) clean

clean: clean-libopencm3
