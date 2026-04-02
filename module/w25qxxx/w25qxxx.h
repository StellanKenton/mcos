/************************************************************************************
* @file     : w25qxxx.h
* @brief    : Reusable W25Qxxx SPI NOR flash module interface.
* @details  : This module provides JEDEC ID probing, read, page program, and
*             erase helpers on top of the generic drvspi abstraction.
***********************************************************************************/
#ifndef W25QXXX_H
#define W25QXXX_H

#include <stdbool.h>
#include <stdint.h>

#include "w25qxxx_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define W25QXXX_MANUFACTURER_ID              0xEFU
#define W25QXXX_PAGE_SIZE                    256U
#define W25QXXX_SECTOR_SIZE                  4096UL
#define W25QXXX_BLOCK64K_SIZE                65536UL
#define W25QXXX_MAX_TRANSFER_LENGTH          65535UL
#define W25QXXX_PAGE_PROGRAM_TIMEOUT_MS      10U
#define W25QXXX_SECTOR_ERASE_TIMEOUT_MS      500U
#define W25QXXX_BLOCK_ERASE_TIMEOUT_MS       3000U
#define W25QXXX_CHIP_ERASE_TIMEOUT_MS        120000U
#define W25QXXX_BUSY_POLL_DELAY_MS           1U

typedef enum eW25qxxxStatus {
    W25QXXX_STATUS_OK = 0,
    W25QXXX_STATUS_INVALID_PARAM,
    W25QXXX_STATUS_NOT_READY,
    W25QXXX_STATUS_BUSY,
    W25QXXX_STATUS_TIMEOUT,
    W25QXXX_STATUS_UNSUPPORTED,
    W25QXXX_STATUS_DEVICE_ID_MISMATCH,
    W25QXXX_STATUS_OUT_OF_RANGE,
    W25QXXX_STATUS_ERROR,
} eW25qxxxStatus;

typedef struct stW25qxxxInfo {
    uint8_t manufacturerId;
    uint8_t memoryType;
    uint8_t capacityId;
    uint8_t addressWidth;
    uint16_t pageSizeBytes;
    uint32_t totalSizeBytes;
    uint32_t sectorSizeBytes;
    uint32_t blockSizeBytes;
} stW25qxxxInfo;

typedef struct stW25qxxxDevice {
    stW25qxxxPortBinding binding;
    stW25qxxxInfo info;
    bool isReady;
} stW25qxxxDevice;

void w25qxxxGetDefaultConfig(stW25qxxxDevice *device);
eW25qxxxStatus w25qxxxInit(stW25qxxxDevice *device);
bool w25qxxxIsReady(const stW25qxxxDevice *device);
const stW25qxxxInfo *w25qxxxGetInfo(const stW25qxxxDevice *device);
eW25qxxxStatus w25qxxxReadJedecId(stW25qxxxDevice *device, uint8_t *manufacturerId, uint8_t *memoryType, uint8_t *capacityId);
eW25qxxxStatus w25qxxxReadStatus1(stW25qxxxDevice *device, uint8_t *statusValue);
eW25qxxxStatus w25qxxxWaitReady(stW25qxxxDevice *device, uint32_t timeoutMs);
eW25qxxxStatus w25qxxxRead(stW25qxxxDevice *device, uint32_t address, uint8_t *buffer, uint32_t length);
eW25qxxxStatus w25qxxxWrite(stW25qxxxDevice *device, uint32_t address, const uint8_t *buffer, uint32_t length);
eW25qxxxStatus w25qxxxEraseSector(stW25qxxxDevice *device, uint32_t address);
eW25qxxxStatus w25qxxxEraseBlock64k(stW25qxxxDevice *device, uint32_t address);
eW25qxxxStatus w25qxxxEraseChip(stW25qxxxDevice *device);

#ifdef __cplusplus
}
#endif

#endif  // W25QXXX_H
/**************************End of file********************************/
