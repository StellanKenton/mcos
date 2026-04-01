# DrvSpi

## Overview

- `drvspi` provides a reusable master-mode SPI abstraction for upper modules.
- The core layer validates parameters, serializes bus access, and controls CS through a hook table.
- The BSP layer owns the GD32F4 SPI controller setup and raw byte transfers.
- CS is intentionally separated from the SPI controller so each logical bus can bind any GPIO pin.

## Files

- `drvspi.h`: Public status codes, transfer descriptors, CS hook types, and API declarations.
- `drvspi.c`: Public parameter validation, bus locking, CS assert/deassert, and transfer helpers.
- `drvspi_port.h`: Logical SPI bus identifiers and default timing macros.
- `drvspi_port.c`: Default BSP binding and the per-bus custom CS GPIO configuration.
- `bspspi.h`: GD32F4 BSP SPI entry points plus the reusable CS pin descriptor.
- `bspspi.c`: SPI0 controller configuration, polling transfer implementation, and CS GPIO helpers.

## Default Mapping

- `DRVSPI_BUS0` uses `SPI0` on `PA5/PA6/PA7`.
- Default custom CS uses `PA4` and is active low.
- To change the CS pin, update the `gDrvSpiBus0CsPin` definition in `drvspi_port.c` or replace it at runtime with `drvSpiSetCsControl()`.

## Public API

- `drvSpiInit()`: Initialize the SPI controller and the currently bound CS pin.
- `drvSpiTransfer()`: Execute a write, write+write, write+read, or read-only transaction under one CS window.
- `drvSpiWrite()` and `drvSpiRead()`: Convenience single-phase helpers.
- `drvSpiWriteRead()`: Send a command or register prefix, then read data while keeping CS asserted.
- `drvSpiExchange()`: Perform a same-length full-duplex transfer.
- `drvSpiSetCsControl()`: Override the default CS hooks and bind a different custom pin strategy.

## Verification Notes

- The implementation is intended to compile in the current GD32F4 Keil project.
- Hardware verification is still required for the selected SPI mode, pin mux, and target device timing.
/**************************End of file********************************/