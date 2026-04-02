# DrvAnlogIic

## Goal

`DrvAnlogIic` provides a reusable software IIC core for upper modules that need a stable bit-banged bus.
The core owns timing, start and stop generation, ACK and NACK handling, repeated start sequences,
and bus recovery. Board-specific GPIO operations stay inside BSP files and are exposed to
`drvanlogiic_port.c` through the BSP hook table.

## Files

- `drvanlogiic.h`: Public API, status codes, transfer structure, and BSP hook table.
- `drvanlogiic.c`: Software IIC core implementation with locking and recovery.
- `drvanlogiic_port.h`: Logical bus identifiers and default timing macros.
- `drvanlogiic_port.c`: Project binding table for logical bus and BSP hook mappings.
- `bspanlogiic.h`: GD32F4 BSP hook declarations consumed by `drvanlogiic_port.c`.
- `bspanlogiic.c`: GD32F4 BSP implementation for GPIO line control and microsecond delay.

## Current Project Binding

- `DRVANLOGIIC_BUS0` is bound in `drvanlogiic_port.c` to the `bspAnlogIic*` hook set.
- `bspanlogiic.c` maps `DRVANLOGIIC_BUS0` to `GPIOD` `PD12` as SCL and `PD13` as SDA.
- The current default half period is `10` us and the recovery clock count is `9`.

## Public API

- `drvAnlogIicInit()`: Configure one logical bus and run a recovery sequence.
- `drvAnlogIicRecoverBus()`: Manually release a stuck bus by toggling SCL and sending STOP.
- `drvAnlogIicTransfer()`: Generic combined write and read transaction with repeated start support.
- `drvAnlogIicTransferTimeout()`: Timeout-compatible wrapper for interface alignment with `DrvIic`.
- `drvAnlogIicWrite()`: Write raw payload to a 7-bit slave address.
- `drvAnlogIicWriteTimeout()`: Timeout-compatible wrapper for interface alignment with `DrvIic`.
- `drvAnlogIicRead()`: Read raw payload from a 7-bit slave address.
- `drvAnlogIicReadTimeout()`: Timeout-compatible wrapper for interface alignment with `DrvIic`.
- `drvAnlogIicWriteRegister()`: Write register selector bytes first, then data.
- `drvAnlogIicWriteRegisterTimeout()`: Timeout-compatible wrapper for interface alignment with `DrvIic`.
- `drvAnlogIicReadRegister()`: Write register selector bytes, then read data with repeated start.
- `drvAnlogIicReadRegisterTimeout()`: Timeout-compatible wrapper for interface alignment with `DrvIic`.

All addresses use 7-bit slave addressing. The driver adds the R and W bit internally.

Transfer contract:

- `writeBuffer` and `secondWriteBuffer` are sent in order before any read phase begins.
- `readBuffer` is filled only after the write phase is finished.
- Timeout wrapper APIs intentionally ignore the timeout value because the software driver remains synchronous and delay-driven.

## Port Binding Requirements

`drvanlogiic_port.c` must assign a complete `stDrvAnlogIicBspInterface` entry for every enabled logical bus.

Required hooks:

- `init(iic)`: Configure GPIO mode and any pull-up or clock resources.
- `setScl(iic, releaseHigh)`: Drive SCL low when `releaseHigh` is false, release SCL high when true.
- `setSda(iic, releaseHigh)`: Drive SDA low when `releaseHigh` is false, release SDA high when true.
- `readScl(iic)`: Sample the current SCL level for recovery checks.
- `readSda(iic)`: Sample the current SDA level.
- `delayUs(delayUs)`: Busy-wait or timer-based delay in microseconds.

Implementation notes:

- Use open-drain semantics on both lines.
- External or internal pull-up must keep the line high when released.
- `setScl(..., true)` and `setSda(..., true)` must not actively drive a push-pull high level.
- `readScl()` is used during recovery checks, so it must reflect the real line state.

## Stability Features

- Bus recovery with configurable recovery pulse count.
- Internal bus lock so multiple tasks do not interleave transactions on the same bus.
- Parameter validation at the public API boundary.
- Explicit `NACK`, `BUSY`, and `NOT_READY` return values.

## Recommended Bring-Up Steps

1. Fill the bus entry in `drvanlogiic_port.c` with real BSP hooks.
2. Set `halfPeriodUs` to match the required bus speed.
3. Call `drvAnlogIicInit()` once during system initialization.
4. Validate `drvAnlogIicReadRegister()` against one known slave device.
5. Verify recovery by forcing SDA low and confirming `drvAnlogIicRecoverBus()` restores the bus.

## Remaining Hardware Risks

- Final timing accuracy depends on the `delayUs()` implementation in the port layer.
- Hardware validation is still required for rise time and pull-up value.