# W25QXXX Module

## Overview

- `w25qxxx` provides a reusable SPI NOR flash module built on the `drvspi` public interface.
- The core layer handles JEDEC ID probing, capacity parsing, read, page program, busy polling, and erase flows.
- The port layer binds the module to `DRVSPI_BUS0` by default and provides the delay hook used while polling the flash busy bit.
- The current default hardware mapping uses the existing SPI BSP configuration: `SPI0` on `PA5/PA6/PA7` with active-low CS on `PA4`.
- The implementation auto-selects 3-byte or 4-byte address commands according to the detected flash capacity.

## Files

- `w25qxxx.h`: Public status codes, device information structure, and module API declarations.
- `w25qxxx.c`: JEDEC probing, parameter validation, read, page program, status polling, and erase logic.
- `w25qxxx_port.h`: Port binding structure, port status codes, and port interface declarations.
- `w25qxxx_port.c`: `drvspi` binding and RTOS-aware millisecond delay helper.

## Initialization Flow

1. Call `w25qxxxGetDefaultConfig()`.
2. Optionally switch to another SPI logical bus with `w25qxxxPortSetHardwareSpi()`.
3. Call `w25qxxxInit()`.
4. Check `w25qxxxIsReady()` before read, write, or erase operations.

## Public API

- `w25qxxxInit(...)`: Initializes the bound SPI bus and reads the JEDEC ID.
- `w25qxxxReadJedecId(...)`: Returns manufacturer, memory type, and capacity bytes after initialization.
- `w25qxxxReadStatus1(...)`: Reads status register 1.
- `w25qxxxWaitReady(...)`: Polls the busy bit until the flash becomes idle or times out.
- `w25qxxxRead(...)`: Reads arbitrary flash ranges and automatically splits large transfers.
- `w25qxxxWrite(...)`: Programs data with page alignment handling and write-enable sequencing.
- `w25qxxxEraseSector(...)`: Erases one aligned 4 KB sector.
- `w25qxxxEraseBlock64k(...)`: Erases one aligned 64 KB block.
- `w25qxxxEraseChip(...)`: Erases the full device.

## Verification Notes

- The module is intended for the current GD32F4 + FreeRTOS project and compiles through the Keil project after adding the two new source files.
- Hardware verification is still required for the selected SPI wiring, flash timing margins, and long erase timeouts on the actual board.
