# Zephyr Learning Log

My notes, experiments and example apps from learning [Zephyr RTOS](https://zephyrproject.org/) on an **STM32 Nucleo-F401RE**.

## Hardware

| | |
|---|---|
| Board | ST Nucleo-F401RE (Zephyr board name: `nucleo_f401re`) |
| MCU | STM32F401RE, Arm Cortex-M4F @ 84 MHz |
| Memory | 512 KB flash, 96 KB RAM |
| Debugger | On-board ST-LINK/V2-1 (flashing, debugging, USB virtual COM port) |
| Console | USART2 via ST-LINK VCP, 115200 baud |
| User I/O | LD2 green LED (`led0`), B1 blue button (`sw0`) |
| Notably missing | No CAN peripheral |

## Host setup

Windows host, building inside **WSL2 (Ubuntu 24.04)**, with the ST-LINK passed into WSL via `usbipd-win`.
Full steps and reasoning: [notes/01-environment-setup.md](notes/01-environment-setup.md).

## Repo layout

```
zephyr-learning/
├── README.md          # this file: overview + progress log
├── notes/             # one markdown note per topic, numbered in learning order
│   └── _template.md   # copy this to start a new note
└── apps/              # out-of-tree Zephyr applications, one folder per experiment
```

This repo is meant to live **inside** the west workspace, e.g. `~/zephyrproject/zephyr-learning`, so `west build` finds Zephyr automatically.

### Building an app

```bash
cd ~/zephyrproject/zephyr-learning
west build -p always -b nucleo_f401re apps/01-blinky
west flash --runner openocd
```

Use `-b native_sim` to run an app on the PC instead (only for apps that don't depend on board hardware).

## Roadmap

- [ ] 01 — Environment setup (WSL2, west, SDK, usbipd) → `hello_world` on `native_sim`
- [ ] 02 — Blinky on hardware; build / flash / console loop
- [ ] 03 — Devicetree: read the board `.dts`, write an overlay for an external LED
- [ ] 04 — Kconfig: `prj.conf`, `menuconfig`
- [ ] 05 — GPIO input: button interrupts and callbacks
- [ ] 06 — Kernel primitives: threads, semaphores, message queues, work queues, timers
- [ ] 07 — Logging and shell
- [ ] 08 — Peripherals: I2C IMU via sensor API, PWM servo
- [ ] 09 — Debugging with `west debug` / GDB
- [ ] 10 — Testing with `native_sim` and Twister

## Progress log

Newest first. One line per session: what I did, what I learned, where the note is.

| Date | What | Notes |
|---|---|---|
| 2026-10-14 | Picked board and host setup (Windows → WSL2), created this repo | [01](notes/01-environment-setup.md) |

## Resources

- [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- [Nucleo-F401RE board page (Zephyr docs)](https://docs.zephyrproject.org/latest/boards/st/nucleo_f401re/doc/index.html)
- [example-application template](https://github.com/zephyrproject-rtos/example-application)
- Nordic DevAcademy — *nRF Connect SDK Fundamentals* (Nordic-focused, but the Zephyr concepts transfer)
- [usbipd-win](https://github.com/dorssel/usbipd-win)
