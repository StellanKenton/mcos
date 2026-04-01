/************************************************************************************
* @file     : drviic.h
* @brief    : Reusable hardware IIC driver abstraction.
* @details  : This module exposes a stable master-mode IIC interface for upper
*             modules while hiding board-specific controller details behind hooks.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRVIIC_H
#define DRVIIC_H

#include <stdint.h>

#include "drviic_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eDrvIicStatus {
    DRVIIC_STATUS_OK = 0,
    DRVIIC_STATUS_INVALID_PARAM,
    DRVIIC_STATUS_NOT_READY,
    DRVIIC_STATUS_BUSY,
    DRVIIC_STATUS_TIMEOUT,
    DRVIIC_STATUS_NACK,
    DRVIIC_STATUS_UNSUPPORTED,
    DRVIIC_STATUS_ERROR,
} eDrvIicStatus;

typedef struct stDrvIicTransfer {
    uint8_t address;
    const uint8_t *writeBuffer;
    uint16_t writeLength;
    uint8_t *readBuffer;
    uint16_t readLength;
    const uint8_t *secondWriteBuffer;
    uint16_t secondWriteLength;
} stDrvIicTransfer;

typedef eDrvIicStatus (*drvIicBspInitFunc)(eDrvIicPortMap iic);
typedef eDrvIicStatus (*drvIicBspTransferFunc)(eDrvIicPortMap iic,
                                              const stDrvIicTransfer *transfer,
                                              uint32_t timeoutMs);
typedef eDrvIicStatus (*drvIicBspRecoverBusFunc)(eDrvIicPortMap iic);

typedef struct stDrvIicBspInterface {
    drvIicBspInitFunc init;
    drvIicBspTransferFunc transfer;
    drvIicBspRecoverBusFunc recoverBus;
    uint32_t defaultTimeoutMs;
} stDrvIicBspInterface;

eDrvIicStatus drvIicInit(eDrvIicPortMap iic);
eDrvIicStatus drvIicRecoverBus(eDrvIicPortMap iic);
eDrvIicStatus drvIicTransfer(eDrvIicPortMap iic, const stDrvIicTransfer *transfer);
eDrvIicStatus drvIicTransferTimeout(eDrvIicPortMap iic, const stDrvIicTransfer *transfer, uint32_t timeoutMs);
eDrvIicStatus drvIicWrite(eDrvIicPortMap iic,
                          uint8_t address,
                          const uint8_t *buffer,
                 uint16_t length);
eDrvIicStatus drvIicWriteTimeout(eDrvIicPortMap iic,
                     uint8_t address,
                     const uint8_t *buffer,
                     uint16_t length,
                     uint32_t timeoutMs);
eDrvIicStatus drvIicRead(eDrvIicPortMap iic,
                         uint8_t address,
                         uint8_t *buffer,
                uint16_t length);
eDrvIicStatus drvIicReadTimeout(eDrvIicPortMap iic,
                    uint8_t address,
                    uint8_t *buffer,
                    uint16_t length,
                    uint32_t timeoutMs);
eDrvIicStatus drvIicWriteRegister(eDrvIicPortMap iic,
                                  uint8_t address,
                                  const uint8_t *registerBuffer,
                                  uint16_t registerLength,
                                  const uint8_t *buffer,
                      uint16_t length);
eDrvIicStatus drvIicWriteRegisterTimeout(eDrvIicPortMap iic,
                          uint8_t address,
                          const uint8_t *registerBuffer,
                          uint16_t registerLength,
                          const uint8_t *buffer,
                          uint16_t length,
                          uint32_t timeoutMs);
eDrvIicStatus drvIicReadRegister(eDrvIicPortMap iic,
                                 uint8_t address,
                                 const uint8_t *registerBuffer,
                                 uint16_t registerLength,
                                 uint8_t *buffer,
                     uint16_t length);
eDrvIicStatus drvIicReadRegisterTimeout(eDrvIicPortMap iic,
                         uint8_t address,
                         const uint8_t *registerBuffer,
                         uint16_t registerLength,
                         uint8_t *buffer,
                         uint16_t length,
                         uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif  // DRVIIC_H
/**************************End of file********************************/
