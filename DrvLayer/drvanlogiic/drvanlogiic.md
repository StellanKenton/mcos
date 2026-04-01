# DrvAnlogIic

## Goal

`DrvAnlogIic` provides a reusable software IIC core for upper modules that need a stable bit-banged bus.
The core owns timing, start and stop generation, ACK and NACK handling, repeated start sequences,
clock stretching wait, and bus recovery. Board-specific GPIO operations stay inside `drvanlogiic_port.c`
 through the BSP hook table.

## Files

- `drvanlogiic.h`: Public API, status codes, transfer structure, and BSP hook table.
- `drvanlogiic.c`: Software IIC core implementation with locking and recovery.
- `drvanlogiic_port.h`: Logical bus identifiers and default timing macros.
- `drvanlogiic_port.c`: Project binding table for SDA and SCL line operations.

## Public API

- `drvAnlogIicInit()`: Configure one logical bus and run a recovery sequence.
- `drvAnlogIicRecoverBus()`: Manually release a stuck bus by toggling SCL and sending STOP.
- `drvAnlogIicTransfer()`: Generic combined write and read transaction with repeated start support.
- `drvAnlogIicWrite()`: Write raw payload to a 7-bit slave address.
- `drvAnlogIicRead()`: Read raw payload from a 7-bit slave address.
- `drvAnlogIicWriteRegister()`: Write register selector bytes first, then data.
- `drvAnlogIicReadRegister()`: Write register selector bytes, then read data with repeated start.

All addresses use 7-bit slave addressing. The driver adds the R and W bit internally.

## Port Binding Requirements

`drvanlogiic_port.c` must assign a complete `stDrvAnlogIicBspInterface` entry for every enabled logical bus.

Required hooks:

- `init(iic)`: Configure GPIO mode and any pull-up or clock resources.
- `setScl(iic, releaseHigh)`: Drive SCL low when `releaseHigh` is false, release SCL high when true.
- `setSda(iic, releaseHigh)`: Drive SDA low when `releaseHigh` is false, release SDA high when true.
- `readScl(iic)`: Sample the current SCL level.
- `readSda(iic)`: Sample the current SDA level.
- `delayUs(delayUs)`: Busy-wait or timer-based delay in microseconds.

Implementation notes:

- Use open-drain semantics on both lines.
- External or internal pull-up must keep the line high when released.
- `setScl(..., true)` and `setSda(..., true)` must not actively drive a push-pull high level.
- `readScl()` is used for clock stretching detection, so it must reflect the real line state.

## Stability Features

- Clock stretching wait with configurable timeout.
- Bus recovery with configurable recovery pulse count.
- Internal bus lock so multiple tasks do not interleave transactions on the same bus.
- Parameter validation at the public API boundary.
- Explicit `NACK`, `TIMEOUT`, `BUSY`, and `NOT_READY` return values.

## Recommended Bring-Up Steps

1. Fill the bus entry in `drvanlogiic_port.c` with real GPIO hooks.
2. Set `halfPeriodUs` to match the required bus speed.
3. Call `drvAnlogIicInit()` once during system initialization.
4. Validate `drvAnlogIicReadRegister()` against one known slave device.
5. Verify recovery by forcing SDA low and confirming `drvAnlogIicRecoverBus()` restores the bus.

## Remaining Hardware Risks

- The current port file ships with all hooks set to `NULL`, so the bus returns `DRVANLOGIIC_STATUS_NOT_READY` until it is bound to real pins.
- Final timing accuracy depends on the `delayUs()` implementation in the port layer.
- Hardware validation is still required for rise time, pull-up value, and clock stretching behavior.