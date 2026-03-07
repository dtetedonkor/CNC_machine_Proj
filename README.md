# CNC Board Revision
This is where Im keeping the work I did for my Senior Design Project. Just a port of the repository that exist on my school github account.

#  Qt Front End (PySide6)

A desktop GUI built with **Qt** using **PySide6**.  
This front end loads and previews design files (e.g., **.svg**) and renders them on a canvas for user interaction and workflow steps like parsing into polylines.

Firmware custom built cnc library using **libopencm3** for driver development.
Firmware takes gcode and converts it into pwm motor controls

## Firmware Drivers (current modules)
- `serial_uart` for UART RX/TX transport buffering and line framing
- `protocol` for line validation and realtime command handling
- `state_machine`/`system_state` for run/hold/alarm transitions
- `io_limits_estop_hand` for debounced limits + latched E-stop safety
- `hal` as the platform-specific GPIO/timer/serial boundary

Driver setup and flashing details: `docs/driversetup.md`
STM32G474 board-support/peripheral/HAL glue stack: `docs/stm32g474_driver_stack.md`

## POSIX Serial Console Simulation
To preview what UART communication looks like on a host machine (instead of STM32 hardware), run:

```bash
cd <repo-root>
gcc -Wall -Werror -pedantic -std=c99 -g \
  examples/posix_serial_console_sim.c \
  src/serial_uart.c src/serial_gcode_bridge.c src/gcode.c src/arc.c src/kinematics.c \
  -lm -o /tmp/posix_serial_console_sim
/tmp/posix_serial_console_sim
```

This prints:
- startup text (`CNC ready`)
- `HOST -> MCU` G-code lines
- `MCU -> HOST` UART responses (`OK` / `error`)
- stepper activity logs for each command


## Features
- Qt-based desktop UI (PySide6)
- Load and preview **SVG** files on a canvas
- Modular UI blocks/widgets (easy to extend)
- [Add: zoom/pan, grid overlay, layers, export, etc.]

## Tech Stack
- **Python 3.10+** (or your version)
- **PySide6** (Qt for Python)
- **Grbl** based firmware control
- **libopencm3** library for low-level microcontroller control
  

## Project Structure
Example layout (adjust to match your repo):
