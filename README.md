# TM4C123 Zephyr Platform

Out-of-tree Zephyr RTOS firmware platform targeting the **TI Tiva C Series TM4C123G LaunchPad** (`tm4c123gxl`).

---

## Hardware Specifications
- **MCU**: TI TM4C123GH6PM (ARM Cortex-M4F @ 80 MHz)
- **Flash**: 256 KB
- **SRAM**: 32 KB
- **Default Console**: UART0 via on-board ICDI Virtual COM port (`115200` 8N1)

---

## Quick Start

### 1. Build the Application
You can build directly with the dedicated build script (it automatically sources the Python virtual environment and sets required Zephyr SDK paths):
```bash
./build.sh
```

Or using `make`:
```bash
make build
```

### 2. Clean / Pristine Rebuild
To force CMake re-configuration and re-build from scratch:
```bash
./build.sh -p
# or: make pristine
```

### 3. Flash to Board
Connect the TM4C123G LaunchPad via the Debug USB port (ICDI) and run:
```bash
./build.sh -f
# or: make flash
# or flash directly without rebuild: ./build.sh --flash-only
```

### 4. Interactive Configuration
```bash
./build.sh -m          # Terminal menuconfig
./build.sh -g          # GUI guiconfig
```

### 5. Open Serial Console
The ICDI provides a Virtual COM port (typically `/dev/ttyACM0`):
```bash
picocom -b 115200 /dev/ttyACM0
# or: minicom -D /dev/ttyACM0 -b 115200
```

---

## Project Structure
```
zephyr/
├── CMakeLists.txt        # Zephyr CMake entry point
├── prj.conf              # Kernel & driver Kconfig options
├── src/
│   └── main.c           # Application source code
├── env.sh                # Environment helper script
├── Makefile              # Convenience make targets
├── .gitignore            # Git ignore rules
└── compile_commands.json -> build/compile_commands.json
```

---

## Configuration & Tuning
To modify kernel and driver configuration via interactive Kconfig:
```bash
make menuconfig
# or: west build -t menuconfig
```
