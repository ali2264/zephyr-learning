# 02 — Devicetree

**Date:** 2026-10-14
**App:** `apps/02-devicetree`

## Goal

Understand where hardware config (pins, UARTs) lives in Zephyr, and add an external LED without hardcoding pins in C.

## Key concepts

- **Devicetree** describes hardware in text. At build time it becomes C macros (no runtime cost, unlike Linux).
- **Layers:** SoC `.dtsi` → board `.dts` → my `.overlay` → merged result in `build/zephyr/zephyr.dts`.
- **Overlay location:** `apps/<app>/boards/<board>.overlay` is picked up automatically for that board.
- **`<&gpioa 8 GPIO_ACTIVE_HIGH>`:** `&gpioa` = reference (phandle) to the GPIO A controller node, `8` = pin number (PA8), `GPIO_ACTIVE_HIGH` = logical 1 means high voltage.
- **Active low:** the user button (PC13) is `GPIO_ACTIVE_LOW` — pressing it pulls the pin to GND, but `gpio_pin_get_dt()` still returns 1 when pressed. Wiring polarity stays in the devicetree, not in C.
- **Node label vs alias:**
  - Label (`ext_led:`) = handle used *inside* the devicetree (`&ext_led`).
  - Alias (`led1 = &ext_led;`) = standard name used by *C code* (`DT_ALIAS(led1)`). Aliases make code portable across boards.
- **`chosen` vs `aliases`:** `chosen` assigns system roles for Zephyr (`zephyr,console = &usart2`); `aliases` are for app code.
- **Console** is on USART2 (`chosen` → `zephyr,console`), routed to the ST-LINK virtual COM port.

## What I did

- Traced `led0` → LD2 on PA5 in `build/zephyr/zephyr.dts` and `zephyr/boards/st/nucleo_f401re/nucleo_f401re.dts`.
- Wired an external LED: D7 (PA8) → 330 Ω → LED → GND.
- Added `boards/nucleo_f401re.overlay` defining `ext_led` and alias `led1`.
- Extended blinky to alternate LD2 and the external LED.
- Moving the LED to another pin = change one number in the overlay, no C changes.

## Gotchas

- Overlay must start with `/ {` or `&label {` — a stray pasted line gave `parse error at :1 (column 1)`.
- Devicetree errors give `file:line (column)` — read that first.
- Missing alias in C shows up as a cryptic macro error containing `DT_N_ALIAS_<name>`.
- `#if !DT_NODE_EXISTS(DT_ALIAS(x))` + `#error` gives a readable message instead.
- New/changed overlay → use `west build -p always`.

## References

- `build/zephyr/zephyr.dts` (final merged devicetree)
- `~/zephyrproject/zephyr/boards/st/nucleo_f401re/nucleo_f401re.dts`
