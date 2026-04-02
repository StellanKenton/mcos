/************************************************************************************
* @file     : w25qxxx_port.h
* @brief    : W25Qxxx module port-layer SPI binding definitions.
* @details  : This file adapts the generic W25Qxxx module to the reusable drvspi
*             public interface and provides platform timing hooks.
***********************************************************************************/
#ifndef W25QXXX_PORT_H
#define W25QXXX_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "Rep/drvlayer/drvspi/drvspi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef W25QXXX_PORT_READ_FILL_DATA
#define W25QXXX_PORT_READ_FILL_DATA           0xFFU
#endif

typedef enum eW25qxxxPortType {
    W25QXXX_PORT_NONE = 0,
    W25QXXX_PORT_DRVSPI,
} eW25qxxxPortType;

typedef eDrvStatus eW25qxxxPortStatus;

typedef struct stW25qxxxPortBinding {
    eW25qxxxPortType type;
    eDrvSpiPortMap spi;
} stW25qxxxPortBinding;

typedef eW25qxxxPortStatus (*w25qxxxPortInitFunc)(const stW25qxxxPortBinding *binding);
typedef eW25qxxxPortStatus (*w25qxxxPortTransferFunc)(const stW25qxxxPortBinding *binding, const uint8_t *writeBuffer, uint16_t writeLength, const uint8_t *secondWriteBuffer, uint16_t secondWriteLength, uint8_t *readBuffer, uint16_t readLength, uint8_t readFillData);
typedef void (*w25qxxxPortDelayMsFunc)(uint32_t delayMs);

typedef struct stW25qxxxPortInterface {
    w25qxxxPortInitFunc init;
    w25qxxxPortTransferFunc transfer;
    w25qxxxPortDelayMsFunc delayMs;
} stW25qxxxPortInterface;

stW25qxxxPortBinding w25qxxxPortGetDefaultBinding(void);
void w25qxxxPortSetHardwareSpi(stW25qxxxPortBinding *binding, eDrvSpiPortMap spi);
bool w25qxxxPortIsValidBinding(const stW25qxxxPortBinding *binding);
const stW25qxxxPortInterface *w25qxxxPortGetInterface(const stW25qxxxPortBinding *binding);
void w25qxxxPortDelayMs(uint32_t delayMs);

#ifdef __cplusplus
}
#endif

#endif  // W25QXXX_PORT_H
/**************************End of file********************************/
