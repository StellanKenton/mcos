# DrvIic

## Goal

`DrvIic` provides a reusable hardware IIC master abstraction for upper modules that need a stable bus API.
The common layer owns parameter validation, task-safe bus serialization, helper APIs for register access,
and timeout normalization. Board-specific controller setup and transfer details stay inside `drviic_port.c`
through the BSP hook table.

## Files

- `drviic.h`: Public API, status codes, transfer structure, and BSP hook table.
- `drviic.c`: Common driver implementation with locking and helper transactions.
- `drviic_port.h`: Logical bus identifiers and default option macros.
- `drviic_port.c`: Project binding table for hardware-specific IIC operations.

## Public API

- `drvIicInit()`: Initialize one logical bus through the bound BSP hook.
- `drvIicRecoverBus()`: Request an optional bus recovery sequence from the BSP layer.
- `drvIicTransfer()`: Execute one combined transfer with up to two write segments and one read phase.
- `drvIicWrite()`: Write one contiguous payload to a 7-bit slave address.
- `drvIicRead()`: Read one contiguous payload from a 7-bit slave address.
- `drvIicWriteRegister()`: Send register selector bytes first, then send payload bytes in the same write transaction.
- `drvIicReadRegister()`: Send register selector bytes, then read payload bytes with a repeated-start capable transfer.

All addresses use 7-bit slave addressing. The BSP layer is responsible for applying the controller-specific
read or write direction bit.

## Port Binding Requirements

`drviic_port.c` must assign a complete `stDrvIicBspInterface` entry for every enabled logical bus.

Required hooks:

- `init(iic)`: Configure clocks, pins, filters, bus speed, and controller state.
- `transfer(iic, transfer, timeoutMs)`: Execute the requested master transaction and return normalized status.

Optional hooks:

- `recoverBus(iic)`: Recover a stuck bus when the controller or GPIO fallback supports it.

Transfer contract:

- `firstWriteBuffer` and `secondWriteBuffer` are sent in order before any read phase begins.
- `readBuffer` is filled only after the write phase is finished.
- A register read should usually map to `firstWriteBuffer = register selector` and `readBuffer = destination`.
- The BSP layer should use a repeated start between write and read phases when the target device requires it.

## Stability Features

- Explicit `INVALID_PARAM`, `NOT_READY`, `BUSY`, `TIMEOUT`, `NACK`, and `UNSUPPORTED` return values.
- Internal bus lock so multiple tasks do not interleave transfers on the same logical bus.
- Timeout normalization through `defaultTimeoutMs` when the caller passes `0`.
- Common register read and register write helpers so upper modules do not duplicate transaction assembly.

## Recommended Bring-Up Steps

1. Fill the bus entry in `drviic_port.c` with real controller hooks.
2. Configure the matching GD32 I2C peripheral clock source, pins, and speed in the BSP `init()` hook.
3. Call `drvIicInit()` once during system initialization.
4. Validate `drvIicReadRegister()` against one known slave device.
5. If bus recovery is supported, verify `drvIicRecoverBus()` against a forced stuck-bus condition.

## Remaining Hardware Risks

- The current port file ships with all hooks set to `NULL`, so the bus returns `DRVIIC_STATUS_NOT_READY` until it is bound to real hardware.
- Final timeout behavior depends on the controller polling or interrupt strategy used in the BSP layer.
- Hardware validation is still required for pin mux, pull-up value, clock speed, and repeated-start behavior.
