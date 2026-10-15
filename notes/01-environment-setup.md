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

- 

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
