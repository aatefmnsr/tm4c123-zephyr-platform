# TM4C123 Zephyr RTOS Platform

[![Zephyr Version](https://img.shields.io/badge/Zephyr%20RTOS-v4.x-brightgreen.svg)](https://zephyrproject.org/)
[![Target MCU](https://img.shields.io/badge/MCU-TI%20TM4C123GH6PM%20(Cortex--M4F)-blue.svg)](https://www.ti.com/product/TM4C123GH6PM)
[![License](https://img.shields.io/badge/License-Apache%202.0-yellow.svg)](LICENSE)

Production-grade, out-of-tree **Zephyr RTOS** platform and driver implementation targeting the **TI Tiva C Series TM4C123G LaunchPad Evaluation Kit** (`ek_tm4c123gxl`).

This repository implements:
1. **Out-of-Tree Board Porting (Zephyr Hardware Model v2)**: Complete board port (`boards/ti/ek_tm4c123gxl`) compliant with Zephyr's latest hardware architecture.
2. **Custom Tiva C GPIO Driver (`ti,tiva-c-gpio`)**: Production-grade Zephyr GPIO controller driver with peripheral clock gating, PF0 NMI commit unlocking, internal pull-up/down support, and edge/level interrupt dispatching.
3. **Devicetree Hardware Modeling**: Standard `gpio-leds` (RGB LED) and `gpio-keys` (SW1/SW2) nodes with aliases (`led0..2`, `sw0..1`).
4. **Custom Kconfig Definitions**: Hierarchical Kconfig tree extending Zephyr's `menuconfig` with application feature flags.
5. **Interactive Shell Subsystem**: Multi-threaded serial CLI with real-time hardware telemetry, compile-time Devicetree introspection, LED controls, and pushbutton telemetry.

---

## Hardware Specifications

| Parameter | Specification |
| :--- | :--- |
| **Core Architecture** | ARM Cortex-M4F with single-precision FPU & MPU |
| **System Clock** | 80 MHz (driven by 16 MHz main crystal + PLL) |
| **Flash Memory** | 256 KB On-Chip Flash |
| **SRAM** | 32 KB On-Chip SRAM |
| **Primary Console** | UART0 via on-board TI ICDI Virtual COM port (`115200` 8N1) |
| **User LEDs** | On-board Tri-Color RGB LED: Red (PF1), Blue (PF2), Green (PF3) |
| **User Pushbuttons** | SW1 (PF4, active-low pull-up) and SW2 (PF0, active-low pull-up) |
| **Debug Probe** | TI ICDI (In-Circuit Debug Interface, OpenOCD compatible) |
| **External Wiring** | **None** — runs entirely over the on-board ICDI micro-USB port |

---

## Architectural Highlights

### 1. Zephyr Hardware Model v2 Board Port
Zephyr v3.7+ and v4.x transitioned to Hardware Model v2, which separates hardware descriptions into discrete YAML metadata, DTS files, and Kconfig layers:
* `boards/ti/ek_tm4c123gxl/board.yml`: Registers the board identifier, full name, vendor, and target SoC (`ti_tm4c123gh6pm`).
* `boards/ti/ek_tm4c123gxl/ek_tm4c123gxl.dts`: Defines pin controller muxing (`pinctrl`), memory mappings (`chosen` nodes for `sram0` and `flash0`), and peripheral status.
* `boards/ti/ek_tm4c123gxl/Kconfig.ek_tm4c123gxl`: Binds the board symbol to the TI SoC configuration.
* `boards/ti/ek_tm4c123gxl/ek_tm4c123gxl_defconfig`: Supplies baseline board defaults (serial console, UART, GPIO).

### 2. Custom Tiva C GPIO Driver (`drivers/gpio/gpio_tiva_c.c`)
Implements Zephyr's `gpio_driver_api` for TI TM4C123 silicon:
* **Clock Gating**: Automatically gates the run-mode clock (`SYSCTL_RCGCGPIO`) for the requested port prior to register access to eliminate BusFault exceptions.
* **NMI Commit Unlock**: Unlocks the hardware lock (`GPIO_LOCK_KEY` / `GPIO_O_CR`) on commit-protected pins (Port F Pin 0 / Port D Pin 7).
* **Weak Pull-Up/Down Support**: Programs pad configuration registers (`GPIOPadConfigSet`) with `GPIO_PIN_TYPE_STD_WPU` to support unbuffered switches.
* **Interrupt Subsystem**: Configures edge-triggered interrupts with NVIC vector routing and callback dispatching (`gpio_fire_callbacks`).

### 3. Layered Kconfig Architecture
* `Kconfig`: Root application Kconfig menu declaring custom build flags (`CONFIG_APP_WELCOME_BANNER`, `CONFIG_APP_ENABLE_DTS_INSPECTION_CMD`, `CONFIG_APP_ENABLE_GPIO_DEMO`).
* `prj.conf`: Application configuration enabling the Zephyr Shell, UART backend, GPIO subsystem, and RAM-conscious stack/buffer tuning for 32 KB SRAM targets.

### 4. Interactive Shell Terminal (`src/main.c`)
Zephyr runs the interactive shell inside its own preemptible kernel thread. Custom command handlers registered via `SHELL_CMD_REGISTER` provide:
* `tm4c info`: Displays active target board, SoC architecture, core clock frequency, memory map, peripheral list, and Zephyr kernel version.
* `tm4c uptime`: Real-time system timer telemetry since boot.
* `tm4c dts`: Compile-time Devicetree introspection demonstrating zero-overhead `DT_` macro queries (`DT_REG_ADDR`, `DT_REG_SIZE`, `DT_NODE_HAS_STATUS`, `DT_PROP`).
* `tm4c led <red|blue|green|all> <on|off|toggle>`: Direct hardware control over the on-board RGB LED.
* `tm4c button`: Real-time status and interrupt press count telemetry for SW1 and SW2.

---

## Repository Structure

```
.
├── CMakeLists.txt              # Root build script declaring BOARD_ROOT and sources
├── Kconfig                     # Custom application Kconfig definitions
├── prj.conf                    # Kernel & subsystem Kconfig settings
├── Makefile                    # Make convenience targets
├── build.sh                    # Automated build, config, and flashing wrapper
├── env.sh                      # Environment export helper script
├── setup-udev.sh               # Host Linux udev rules for ICDI debug probe
├── boards/
│   └── ti/
│       └── ek_tm4c123gxl/      # Out-of-tree board definition
│           ├── board.yml       # Hardware Model v2 board metadata
│           ├── board.cmake     # OpenOCD runner integration
│           ├── ek_tm4c123gxl.dts        # Board Devicetree hardware description
│           ├── ek_tm4c123gxl.yaml       # Twister and target runner description
│           ├── ek_tm4c123gxl_defconfig  # Board-level Kconfig defaults
│           ├── Kconfig.ek_tm4c123gxl    # Board Kconfig symbol definition
│           └── support/
│               └── openocd.cfg # TI ICDI OpenOCD script
├── dts/
│   └── bindings/
│       └── gpio/
│           └── ti,tiva-c-gpio.yaml      # Custom GPIO Devicetree binding
├── drivers/
│   └── gpio/
│       └── gpio_tiva_c.c       # Custom Tiva C GPIO controller driver
└── src/
    └── main.c                  # Shell commands, interrupt callbacks, and entry point
```

---

## Quick Start

### 1. Build the Firmware
Compile with the automated build script (defaults to `ek_tm4c123gxl`):
```bash
./build.sh
```

Or using `make`:
```bash
make build
```

### 2. Force a Pristine Rebuild
```bash
./build.sh -p
# or: make pristine
```

### 3. Flash to Silicon
Connect the TM4C123G LaunchPad via the Debug USB port (ICDI) and run:
```bash
./build.sh -f
# or flash directly without rebuild:
./build.sh --flash-only
```
> **USB Permissions Note**: If you encounter `LIBUSB_ERROR_ACCESS`, run the one-time host script:
> `sudo ./setup-udev.sh`

### 4. Interactive Configuration (`menuconfig`)
Inspect or customize kernel, driver, and application parameters:
```bash
./build.sh -m
# or: make menuconfig
```

### 5. Connect to the Serial Terminal
Open the serial console on the ICDI virtual COM port:
```bash
picocom -b 115200 /dev/ttyACM0
```

---

## Example Shell Session

```text
==============================================
   TI TM4C123GXL Zephyr Shell Terminal
   Zephyr OS 4.5.99 | Board: ek_tm4c123gxl/ti_tm4c123gh6pm
   RGB LEDs & SW1/SW2 Buttons: Initialized
   Type 'help' or 'tm4c --help' for commands
==============================================

tm4c:~$ tm4c info
========================================
  Target Board : ek_tm4c123gxl/ti_tm4c123gh6pm
  SOC          : TI TM4C123GH6PM (ARM Cortex-M4F)
  Clock        : 80 MHz
  Flash / RAM  : 256 KB Flash / 32 KB SRAM
  Peripherals  : UART0, RGB LED (PF1-3), SW1/SW2 (PF4,0)
  Zephyr RTOS  : 4.5.99
========================================

tm4c:~$ tm4c led red on
LED 'red' set to ON

tm4c:~$ tm4c led blue toggle
LED 'blue' toggled

tm4c:~$ tm4c led all off
LED 'all' set to OFF

tm4c:~$ tm4c button
=== Pushbutton Telemetry ===
  SW1 (PF4) : RELEASED (Presses: 3)
  SW2 (PF0) : RELEASED (Presses: 1)
============================

tm4c:~$ tm4c dts
=== Devicetree Compile-Time Introspection ===
Target Board  : ek_tm4c123gxl/ti_tm4c123gh6pm
UART0 Status  : okay (Baud: 115200)
SRAM Base/Size: 0x20000000 / 32 KB
Flash Base/Sz : 0x00000000 / 256 KB
LED0 (Red)    : Port F Pin 1 (Active High)
LED1 (Blue)   : Port F Pin 2 (Active High)
LED2 (Green)  : Port F Pin 3 (Active High)
SW0 (Btn 1)   : Port F Pin 4 (Pull-Up, Active Low)
SW1 (Btn 2)   : Port F Pin 0 (Pull-Up, Active Low)
=============================================

tm4c:~$ tm4c uptime
System Uptime: 0 h 0 m 42 s (42110 ms)
```
