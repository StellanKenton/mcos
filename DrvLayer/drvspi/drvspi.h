/************************************************************************************
* @file     : drvspi.h
* @brief    : Reusable hardware SPI driver abstraction.
* @details  : This module exposes a stable master-mode SPI interface for upper
*             modules while hiding controller and chip-select details behind hooks.
***********************************************************************************/
#ifndef DRVSPI_H
#define DRVSPI_H

#include <stdbool.h>
#include <stdint.h>

#include "drvspi_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eDrvSpiStatus {
    DRVSPI_STATUS_OK = 0,
    DRVSPI_STATUS_INVALID_PARAM,
    DRVSPI_STATUS_NOT_READY,
    DRVSPI_STATUS_BUSY,
    DRVSPI_STATUS_TIMEOUT,
    DRVSPI_STATUS_UNSUPPORTED,
    DRVSPI_STATUS_ERROR,
} eDrvSpiStatus;

typedef struct stDrvSpiTransfer {
    const uint8_t *writeBuffer;
    uint16_t writeLength;
    const uint8_t *secondWriteBuffer;
    uint16_t secondWriteLength;
    uint8_t *readBuffer;
    uint16_t readLength;
    uint8_t readFillData;
} stDrvSpiTransfer;

typedef eDrvSpiStatus (*drvSpiBspInitFunc)(eDrvSpiPortMap spi);
typedef eDrvSpiStatus (*drvSpiBspTransferFunc)(eDrvSpiPortMap spi, const uint8_t *txBuffer, uint8_t *rxBuffer, uint16_t length, uint8_t fillData, uint32_t timeoutMs);
typedef void (*drvSpiCsInitFunc)(void *context);
typedef void (*drvSpiCsWriteFunc)(void *context, bool isActive);

typedef struct stDrvSpiCsControl {
    drvSpiCsInitFunc init;
    drvSpiCsWriteFunc write;
    void *context;
} stDrvSpiCsControl;

typedef struct stDrvSpiBspInterface {
    drvSpiBspInitFunc init;
    drvSpiBspTransferFunc transfer;
    uint32_t defaultTimeoutMs;
    stDrvSpiCsControl csControl;
} stDrvSpiBspInterface;

eDrvSpiStatus drvSpiInit(eDrvSpiPortMap spi);
eDrvSpiStatus drvSpiSetCsControl(eDrvSpiPortMap spi, const stDrvSpiCsControl *control);
eDrvSpiStatus drvSpiTransfer(eDrvSpiPortMap spi, const stDrvSpiTransfer *transfer);
eDrvSpiStatus drvSpiTransferTimeout(eDrvSpiPortMap spi, const stDrvSpiTransfer *transfer, uint32_t timeoutMs);
eDrvSpiStatus drvSpiWrite(eDrvSpiPortMap spi, const uint8_t *buffer, uint16_t length);
eDrvSpiStatus drvSpiWriteTimeout(eDrvSpiPortMap spi, const uint8_t *buffer, uint16_t length, uint32_t timeoutMs);
eDrvSpiStatus drvSpiRead(eDrvSpiPortMap spi, uint8_t *buffer, uint16_t length);
eDrvSpiStatus drvSpiReadTimeout(eDrvSpiPortMap spi, uint8_t *buffer, uint16_t length, uint32_t timeoutMs);
eDrvSpiStatus drvSpiWriteRead(eDrvSpiPortMap spi, const uint8_t *writeBuffer, uint16_t writeLength, uint8_t *readBuffer, uint16_t readLength);
eDrvSpiStatus drvSpiWriteReadTimeout(eDrvSpiPortMap spi, const uint8_t *writeBuffer, uint16_t writeLength, uint8_t *readBuffer, uint16_t readLength, uint32_t timeoutMs);
eDrvSpiStatus drvSpiExchange(eDrvSpiPortMap spi, const uint8_t *txBuffer, uint8_t *rxBuffer, uint16_t length);
eDrvSpiStatus drvSpiExchangeTimeout(eDrvSpiPortMap spi, const uint8_t *txBuffer, uint8_t *rxBuffer, uint16_t length, uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif  // DRVSPI_H
/**************************End of file********************************/

