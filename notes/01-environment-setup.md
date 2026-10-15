# 01 — Environment Setup

**Date:** 2026-10-14
**App:** none yet (verify with `samples/hello_world` on `native_sim`)

## Goal

Get a working Zephyr build environment on my Windows machine that can build, flash and talk to the Nucleo-F401RE.

## Decision: host OS

Zephyr supports native Windows, Linux and macOS. It's the same Zephyr everywhere; only the host tooling differs.

| Option | Pros | Cons |
|---|---|---|
| Native Windows | Board/USB just works | Slow builds, no `native_sim` |
| **WSL2 (chosen)** | Linux env, fast builds, `native_sim`, Twister | ST-LINK needs `usbipd-win` passthrough |
| Dual-boot Linux | Smoothest | Overkill for now |

Why Linux matters:
- **`native_sim`** — builds the app as a Linux executable so I can test logic without hardware. Linux host only.
- **Twister** (Zephyr's test runner) works best on Linux.
- **Build speed** — Zephyr builds touch lots of small files; Windows is often 2–3x slower.
- Docs, forums, CI and most companies assume Linux.

## What I did

### 1. Install WSL2 (PowerShell, admin)

```powershell
wsl --install -d Ubuntu-24.04
```

> Keep the workspace in the Linux filesystem (`~/zephyrproject`), **never** under `/mnt/c/...` — cross-filesystem builds are extremely slow.

### 2. Toolchain (inside Ubuntu)

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv \
  python3-tk xz-utils file make gcc gcc-multilib g++-multilib \
  libsdl2-dev libmagic1 usbutils openocd

python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
pip install west
west init ~/zephyrproject
cd ~/zephyrproject
west update
west zephyr-export
west packages pip --install
west sdk install
```

Add to `~/.bashrc`:

```bash
source ~/zephyrproject/.venv/bin/activate
```

Sanity check without hardware:

```bash
cd ~/zephyrproject/zephyr
west build -p always -b native_sim samples/hello_world
./build/zephyr/zephyr.exe
```

### 3. ST-LINK passthrough (Windows side)

```powershell
winget install usbipd
# admin PowerShell, board plugged in:
usbipd list                                  # find ST-LINK BUSID, e.g. 2-3
usbipd bind --busid 2-3                      # one-time
usbipd attach --wsl --busid 2-3 --auto-attach
```

`--auto-attach` handles the ST-LINK re-enumerating during flashing. Leave that window open.

Check from Ubuntu: `lsusb` should list `STMicroelectronics ST-LINK/V2.1`.

### 4. USB permissions (Ubuntu)

```bash
sudo service udev restart
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo usermod -aG dialout,plugdev $USER
```

Then `wsl --shutdown` from PowerShell, reopen Ubuntu, reattach the board.

### 5. Build, flash, console

```bash
west build -p always -b nucleo_f401re samples/basic/blinky
west flash --runner openocd

sudo apt install minicom
minicom -D /dev/ttyACM0 -b 115200            # exit: Ctrl-A, X
```

## Key concepts

- **west** — Zephyr's meta-tool: manages the workspace (`init`, `update`), builds, flashes, debugs.
- **West workspace** — `~/zephyrproject` holds `zephyr/`, `modules/`, and my own repos side by side.
- **Zephyr SDK** — the cross-compilers (arm-zephyr-eabi etc.) and host tools.
- **Runner** — the backend `west flash` uses (`openocd`, `stm32cubeprogrammer`, `jlink`, `pyocd`).

## Gotchas

- `west flash` can't find the device → board not attached (`usbipd attach`), udev not running, or user not yet in `plugdev`.
- Fallback flashing path: open `\\wsl$\Ubuntu-24.04\home\<me>\zephyrproject\...\build\zephyr\zephyr.hex` in STM32CubeProgrammer on Windows.
- If any setup command differs from the current docs, trust the Getting Started Guide — the Python-deps and SDK steps have changed between releases.

## Editor

VS Code + **WSL extension**; run `code .` from inside Ubuntu.

## Open questions

### Why Zephyr? Bare metal vs RTOS vs Zephyr

Three levels, not two:

| | Bare metal | Kernel-only RTOS (e.g. FreeRTOS) | Zephyr |
|---|---|---|---|
| Structure | `while(1)` superloop + ISRs | Tasks + scheduler | Threads + scheduler |
| Drivers | Write your own / vendor HAL | Bring your own (vendor HAL) | Built-in driver model, portable APIs |
| Hardware config | Hardcoded in C | Hardcoded in C | Devicetree |
| Feature selection | `#define`s by hand | `FreeRTOSConfig.h` | Kconfig |
| Extras | None | Few | Networking, BLE, USB, CAN, filesystems, logging, shell, power mgmt, bootloader/OTA, test framework |

### Can we see the difference just looking at the code?

Yes. The same blinky, three ways:

```c
// Bare metal (registers): I own everything, including clocks
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;      // enable GPIOA clock
GPIOA->MODER |= (1 << (5 * 2));           // PA5 = output
while (1) {
    GPIOA->ODR ^= (1 << 5);               // toggle PA5
    for (volatile int i = 0; i < 500000; i++);   // busy-wait
}

// STM32 HAL: friendlier, still STM32-only and pin hardcoded
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
HAL_Delay(500);                           // still spins the CPU

// Zephyr: pin comes from the devicetree, sleep yields the CPU
gpio_pin_toggle_dt(&led);                 // led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios)
k_msleep(500);                            // thread sleeps; scheduler runs others or idles
```

What's *missing* from the Zephyr version is the point:
- **No pin numbers in C**: `led0` is resolved by the devicetree. The same code runs on any board with an `led0`.
- **No clock / UART / system init**: Zephyr configures clocks and the console from the devicetree and Kconfig before `main()`.
- **`k_msleep` is not a busy-wait**: the thread blocks and the scheduler can run other threads or put the CPU to sleep. `HAL_Delay` and the for-loop burn the CPU.
- **`main()` is a thread**: I could add more threads alongside it without restructuring anything.
- **`printk` just works**: it goes to whatever `chosen { zephyr,console }` points to (USART2 here). No UART init code.

### What does Zephyr give us?

- **Preemptive kernel**: threads, priorities, semaphores, mutexes, message queues, work queues, timers.
- **Portable driver APIs**: GPIO, I2C, SPI, UART, PWM, ADC, CAN, sensors. Same calls across vendors.
- **Devicetree + Kconfig**: hardware described in data, features switched on and off without touching code.
- **Subsystems**: logging, shell, settings/NVS, filesystems, power management, USB, networking (TCP/IP), Bluetooth LE, Thread/Matter.
- **MCUboot**: secure bootloader and firmware updates.
- **Testing**: `native_sim` (run firmware on the PC), ztest unit tests, Twister test runner.
- **Ecosystem**: hundreds of supported boards, vendor-backed (Nordic, NXP, ST, Intel…), Linux-Foundation project, permissive Apache 2.0 license.

### When to choose which?

**Use Zephyr when:**
- The system does several things at once (e.g. sample an IMU at a fixed rate, run a control loop, stream telemetry over CAN or UART). Threads keep these independent instead of one tangled superloop.
- You need a protocol stack (BLE, TCP/IP, USB, CAN) and don't want to write or integrate one.
- The code should survive a hardware change (new MCU or board → new devicetree, same app).
- It's a long-lived or team product needing OTA updates, logging, a shell and automated tests.

**Use bare metal when:**
- The MCU is tiny (a few KB flash/RAM). My blinky already uses ~18 KB flash and ~4.6 KB RAM in Zephyr; bare metal would be ~1–2 KB.
- The job is a single simple loop (read sensor → set output) with no comms stack.
- Timing must be cycle-exact and fully owned (e.g. hand-tuned bit-banging, very tight ISR latency budgets) and every layer of abstraction is a cost.
- The goal is learning the hardware itself (registers, clocks, interrupts), or certification where every line must be audited.

**The middle ground:** FreeRTOS + vendor HAL. It's lighter than Zephyr and gives you threads, but drivers, config and portability are on you.

**Costs of Zephyr to remember:** steeper learning curve (devicetree, Kconfig, CMake, west), bigger footprint, more layers to debug through, and a fast-moving codebase (I'm on `main`, 4.5.0-rc1).

### Still open

- How do Zephyr's ISR latency and scheduling jitter compare to bare metal on this board? (measure later with a scope/logic analyzer)
- How does a Zephyr thread differ from a FreeRTOS task in practice?

## References

- [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- [usbipd-win](https://github.com/dorssel/usbipd-win)
- [Nucleo-F401RE board docs](https://docs.zephyrproject.org/latest/boards/st/nucleo_f401re/doc/index.html)

## Known issue: `west flash` ends with FATAL ERROR (WSL + usbipd)

Every `west flash --runner openocd` ends with:

```
FATAL ERROR: command exited with status 1: .../openocd ... -c 'reset init'
  -c 'flash write_image erase .../zephyr.hex' -c 'reset run' -c shutdown
```

**The flash still succeeds.** Look for this line earlier in the output:

```
wrote 32768 bytes from file .../zephyr.hex in ~1.0s
```

If it's there, the new firmware is on the board. The error comes from the final
`reset run` / `shutdown` step: the chip resets, the ST-LINK briefly drops off the
USB-over-network link that `usbipd` provides, and OpenOCD reports that as a failure.
Messages like `communication failure` / `examination failed` at the *start* of the
next flash are the link still recovering from the previous reset.

**How to confirm a flash worked:** make a visible change (e.g. blink rate) and check
the board, or check the console output.

**If it ever blocks a flash** (no `wrote ... bytes` line):
- Unplug/replug the board and rerun `usbipd attach --wsl --busid 1-4 --auto-attach`.
- Try another runner (e.g. STM32CubeProgrammer, Linux version, installed in WSL).

**Side effect:** flashing resets the ST-LINK, so an open minicom session stops
showing output. Close minicom before flashing and reopen it after.
