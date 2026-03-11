.PHONY: all clean

all:
	@echo "Legacy third-party firmware submodule support was removed; use STM32 HAL firmware under /drivers."
	@echo "For host core builds use: cmake -S . -B /tmp/cnc-core-build && cmake --build /tmp/cnc-core-build"

clean:
	@echo "Nothing to clean at repository root."
