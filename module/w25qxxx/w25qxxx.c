/************************************************************************************
* @file     : w25qxxx.c
* @brief    : Reusable W25Qxxx SPI NOR flash module implementation.
* @details  : This file contains parameter validation, JEDEC ID probing, busy
*             polling, paged writes, and erase helpers using the port hooks.
***********************************************************************************/
#include "w25qxxx.h"

#include <stddef.h>

#define W25QXXX_CMD_WRITE_ENABLE             0x06U
#define W25QXXX_CMD_READ_STATUS1             0x05U
#define W25QXXX_CMD_JEDEC_ID                 0x9FU
#define W25QXXX_CMD_READ_DATA                0x03U
#define W25QXXX_CMD_READ_DATA_4B             0x13U
#define W25QXXX_CMD_PAGE_PROGRAM             0x02U
#define W25QXXX_CMD_PAGE_PROGRAM_4B          0x12U
#define W25QXXX_CMD_SECTOR_ERASE             0x20U
#define W25QXXX_CMD_SECTOR_ERASE_4B          0x21U
#define W25QXXX_CMD_BLOCK_ERASE_64K          0xD8U
#define W25QXXX_CMD_BLOCK_ERASE_64K_4B       0xDCU
#define W25QXXX_CMD_CHIP_ERASE               0xC7U

#define W25QXXX_STATUS1_BUSY_MASK            0x01U
#define W25QXXX_STATUS1_WEL_MASK             0x02U

static bool w25qxxxIsValidDevice(const stW25qxxxDevice *device)
{
    return (device != NULL) && w25qxxxPortIsValidBinding(&device->binding);
}

static bool w25qxxxIsReadyForAccess(const stW25qxxxDevice *device)
{
    return w25qxxxIsValidDevice(device) && device->isReady;
}

static eW25qxxxStatus w25qxxxMapPortStatus(eDrvStatus status)
{
    switch (status) {
        case DRV_STATUS_OK:
        case DRV_STATUS_INVALID_PARAM:
        case DRV_STATUS_NOT_READY:
        case DRV_STATUS_BUSY:
        case DRV_STATUS_TIMEOUT:
        case DRV_STATUS_NACK:
        case DRV_STATUS_UNSUPPORTED:
        case DRV_STATUS_ID_NOTMATCH:
        case DRV_STATUS_ERROR:
            return (eW25qxxxStatus)status;
        default:
            return W25QXXX_STATUS_ERROR;
    }
}

static const stW25qxxxPortInterface *w25qxxxGetPortInterface(const stW25qxxxDevice *device)
{
    if (!w25qxxxIsValidDevice(device)) {
        return NULL;
    }

    return w25qxxxPortGetInterface(&device->binding);
}

static eW25qxxxStatus w25qxxxTransferInternal(const stW25qxxxDevice *device, const uint8_t *writeBuffer, uint16_t writeLength, const uint8_t *secondWriteBuffer, uint16_t secondWriteLength, uint8_t *readBuffer, uint16_t readLength)
{
    const stW25qxxxPortInterface *lPortInterface;

    lPortInterface = w25qxxxGetPortInterface(device);
    if ((lPortInterface == NULL) || (lPortInterface->transfer == NULL)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    return w25qxxxMapPortStatus(lPortInterface->transfer(&device->binding,
                                                         writeBuffer,
                                                         writeLength,
                                                         secondWriteBuffer,
                                                         secondWriteLength,
                                                         readBuffer,
                                                         readLength,
                                                         W25QXXX_PORT_READ_FILL_DATA));
}

static void w25qxxxBuildAddressCommand(uint8_t *header, uint8_t command, uint32_t address, uint8_t addressWidth)
{
    header[0] = command;
    if (addressWidth == 4U) {
        header[1] = (uint8_t)(address >> 24);
        header[2] = (uint8_t)(address >> 16);
        header[3] = (uint8_t)(address >> 8);
        header[4] = (uint8_t)address;
    } else {
        header[1] = (uint8_t)(address >> 16);
        header[2] = (uint8_t)(address >> 8);
        header[3] = (uint8_t)address;
    }
}

static bool w25qxxxIsValidCapacityId(uint8_t capacityId)
{
    return (capacityId >= 0x10U) && (capacityId < 32U);
}

static bool w25qxxxIsRangeValid(const stW25qxxxDevice *device, uint32_t address, uint32_t length)
{
    if (!w25qxxxIsReadyForAccess(device)) {
        return false;
    }

    if (length == 0U) {
        return address <= device->info.totalSizeBytes;
    }

    if (address >= device->info.totalSizeBytes) {
        return false;
    }

    return length <= (device->info.totalSizeBytes - address);
}

static eW25qxxxStatus w25qxxxReadStatus1Internal(const stW25qxxxDevice *device, uint8_t *statusValue)
{
    uint8_t lCommand;

    if ((device == NULL) || (statusValue == NULL)) {
        return W25QXXX_STATUS_INVALID_PARAM;
    }

    lCommand = W25QXXX_CMD_READ_STATUS1;
    return w25qxxxTransferInternal(device, &lCommand, 1U, NULL, 0U, statusValue, 1U);
}

static eW25qxxxStatus w25qxxxReadJedecIdInternal(const stW25qxxxDevice *device, uint8_t *manufacturerId, uint8_t *memoryType, uint8_t *capacityId)
{
    uint8_t lCommand;
    uint8_t lJedecId[3];
    eW25qxxxStatus lStatus;

    if ((device == NULL) || (manufacturerId == NULL) || (memoryType == NULL) || (capacityId == NULL)) {
        return W25QXXX_STATUS_INVALID_PARAM;
    }

    lCommand = W25QXXX_CMD_JEDEC_ID;
    lStatus = w25qxxxTransferInternal(device, &lCommand, 1U, NULL, 0U, lJedecId, 3U);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    *manufacturerId = lJedecId[0];
    *memoryType = lJedecId[1];
    *capacityId = lJedecId[2];
    return W25QXXX_STATUS_OK;
}

static eW25qxxxStatus w25qxxxWriteEnableInternal(const stW25qxxxDevice *device)
{
    uint8_t lCommand;
    uint8_t lStatusValue;
    eW25qxxxStatus lStatus;

    if (device == NULL) {
        return W25QXXX_STATUS_INVALID_PARAM;
    }

    lCommand = W25QXXX_CMD_WRITE_ENABLE;
    lStatus = w25qxxxTransferInternal(device, &lCommand, 1U, NULL, 0U, NULL, 0U);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    lStatus = w25qxxxReadStatus1Internal(device, &lStatusValue);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    if ((lStatusValue & W25QXXX_STATUS1_WEL_MASK) == 0U) {
        return W25QXXX_STATUS_ERROR;
    }

    return W25QXXX_STATUS_OK;
}

static eW25qxxxStatus w25qxxxWaitReadyInternal(const stW25qxxxDevice *device, uint32_t timeoutMs)
{
    const stW25qxxxPortInterface *lPortInterface;
    uint8_t lStatusValue;
    uint32_t lElapsedMs;
    eW25qxxxStatus lStatus;

    if (device == NULL) {
        return W25QXXX_STATUS_INVALID_PARAM;
    }

    lPortInterface = w25qxxxGetPortInterface(device);
    if ((lPortInterface == NULL) || (lPortInterface->delayMs == NULL)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    lElapsedMs = 0U;
    while (true) {
        lStatus = w25qxxxReadStatus1Internal(device, &lStatusValue);
        if (lStatus != W25QXXX_STATUS_OK) {
            return lStatus;
        }

        if ((lStatusValue & W25QXXX_STATUS1_BUSY_MASK) == 0U) {
            return W25QXXX_STATUS_OK;
        }

        if (lElapsedMs >= timeoutMs) {
            return W25QXXX_STATUS_TIMEOUT;
        }

        lPortInterface->delayMs(W25QXXX_BUSY_POLL_DELAY_MS);
        lElapsedMs += W25QXXX_BUSY_POLL_DELAY_MS;
    }
}

static uint8_t w25qxxxGetReadCommand(const stW25qxxxDevice *device)
{
    return (device->info.addressWidth == 4U) ? W25QXXX_CMD_READ_DATA_4B : W25QXXX_CMD_READ_DATA;
}

static uint8_t w25qxxxGetProgramCommand(const stW25qxxxDevice *device)
{
    return (device->info.addressWidth == 4U) ? W25QXXX_CMD_PAGE_PROGRAM_4B : W25QXXX_CMD_PAGE_PROGRAM;
}

static uint8_t w25qxxxGetSectorEraseCommand(const stW25qxxxDevice *device)
{
    return (device->info.addressWidth == 4U) ? W25QXXX_CMD_SECTOR_ERASE_4B : W25QXXX_CMD_SECTOR_ERASE;
}

static uint8_t w25qxxxGetBlockEraseCommand(const stW25qxxxDevice *device)
{
    return (device->info.addressWidth == 4U) ? W25QXXX_CMD_BLOCK_ERASE_64K_4B : W25QXXX_CMD_BLOCK_ERASE_64K;
}

void w25qxxxGetDefaultConfig(stW25qxxxDevice *device)
{
    if (device == NULL) {
        return;
    }

    device->binding = w25qxxxPortGetDefaultBinding();
    device->info.manufacturerId = 0U;
    device->info.memoryType = 0U;
    device->info.capacityId = 0U;
    device->info.addressWidth = 3U;
    device->info.pageSizeBytes = W25QXXX_PAGE_SIZE;
    device->info.totalSizeBytes = 0U;
    device->info.sectorSizeBytes = W25QXXX_SECTOR_SIZE;
    device->info.blockSizeBytes = W25QXXX_BLOCK64K_SIZE;
    device->isReady = false;
}

eW25qxxxStatus w25qxxxInit(stW25qxxxDevice *device)
{
    const stW25qxxxPortInterface *lPortInterface;
    eW25qxxxStatus lStatus;

    if (!w25qxxxIsValidDevice(device)) {
        return W25QXXX_STATUS_INVALID_PARAM;
    }

    lPortInterface = w25qxxxGetPortInterface(device);
    if ((lPortInterface == NULL) || (lPortInterface->init == NULL)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    device->isReady = false;

    lStatus = w25qxxxMapPortStatus(lPortInterface->init(&device->binding));
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    lStatus = w25qxxxReadJedecIdInternal(device, &device->info.manufacturerId, &device->info.memoryType, &device->info.capacityId);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    if (device->info.manufacturerId != W25QXXX_MANUFACTURER_ID) {
        return W25QXXX_STATUS_DEVICE_ID_MISMATCH;
    }

    if (!w25qxxxIsValidCapacityId(device->info.capacityId)) {
        return W25QXXX_STATUS_UNSUPPORTED;
    }

    device->info.totalSizeBytes = (uint32_t)(1UL << device->info.capacityId);
    device->info.pageSizeBytes = W25QXXX_PAGE_SIZE;
    device->info.sectorSizeBytes = W25QXXX_SECTOR_SIZE;
    device->info.blockSizeBytes = W25QXXX_BLOCK64K_SIZE;
    device->info.addressWidth = (device->info.totalSizeBytes > 0x01000000UL) ? 4U : 3U;
    device->isReady = true;
    return W25QXXX_STATUS_OK;
}

bool w25qxxxIsReady(const stW25qxxxDevice *device)
{
    return w25qxxxIsReadyForAccess(device);
}

const stW25qxxxInfo *w25qxxxGetInfo(const stW25qxxxDevice *device)
{
    if (!w25qxxxIsReadyForAccess(device)) {
        return NULL;
    }

    return &device->info;
}

eW25qxxxStatus w25qxxxReadJedecId(stW25qxxxDevice *device, uint8_t *manufacturerId, uint8_t *memoryType, uint8_t *capacityId)
{
    if (!w25qxxxIsReadyForAccess(device)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    return w25qxxxReadJedecIdInternal(device, manufacturerId, memoryType, capacityId);
}

eW25qxxxStatus w25qxxxReadStatus1(stW25qxxxDevice *device, uint8_t *statusValue)
{
    if (!w25qxxxIsReadyForAccess(device)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    return w25qxxxReadStatus1Internal(device, statusValue);
}

eW25qxxxStatus w25qxxxWaitReady(stW25qxxxDevice *device, uint32_t timeoutMs)
{
    if (!w25qxxxIsReadyForAccess(device)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    return w25qxxxWaitReadyInternal(device, timeoutMs);
}

eW25qxxxStatus w25qxxxRead(stW25qxxxDevice *device, uint32_t address, uint8_t *buffer, uint32_t length)
{
    uint8_t lHeader[5];
    uint32_t lOffset;
    uint32_t lChunkLength;
    uint8_t lHeaderLength;
    eW25qxxxStatus lStatus;

    if (!w25qxxxIsReadyForAccess(device)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    if ((buffer == NULL) && (length > 0U)) {
        return W25QXXX_STATUS_INVALID_PARAM;
    }

    if (length == 0U) {
        return W25QXXX_STATUS_OK;
    }

    if (!w25qxxxIsRangeValid(device, address, length)) {
        return W25QXXX_STATUS_OUT_OF_RANGE;
    }

    lHeaderLength = (device->info.addressWidth == 4U) ? 5U : 4U;
    lOffset = 0U;
    while (lOffset < length) {
        lChunkLength = length - lOffset;
        if (lChunkLength > W25QXXX_MAX_TRANSFER_LENGTH) {
            lChunkLength = W25QXXX_MAX_TRANSFER_LENGTH;
        }

        w25qxxxBuildAddressCommand(lHeader, w25qxxxGetReadCommand(device), address + lOffset, device->info.addressWidth);
        lStatus = w25qxxxTransferInternal(device, lHeader, lHeaderLength, NULL, 0U, &buffer[lOffset], (uint16_t)lChunkLength);
        if (lStatus != W25QXXX_STATUS_OK) {
            return lStatus;
        }

        lOffset += lChunkLength;
    }

    return W25QXXX_STATUS_OK;
}

eW25qxxxStatus w25qxxxWrite(stW25qxxxDevice *device, uint32_t address, const uint8_t *buffer, uint32_t length)
{
    uint8_t lHeader[5];
    uint32_t lOffset;
    uint32_t lChunkLength;
    uint32_t lPageOffset;
    uint32_t lPageRemain;
    uint8_t lHeaderLength;
    eW25qxxxStatus lStatus;

    if (!w25qxxxIsReadyForAccess(device)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    if ((buffer == NULL) && (length > 0U)) {
        return W25QXXX_STATUS_INVALID_PARAM;
    }

    if (length == 0U) {
        return W25QXXX_STATUS_OK;
    }

    if (!w25qxxxIsRangeValid(device, address, length)) {
        return W25QXXX_STATUS_OUT_OF_RANGE;
    }

    lHeaderLength = (device->info.addressWidth == 4U) ? 5U : 4U;
    lOffset = 0U;
    while (lOffset < length) {
        lPageOffset = (address + lOffset) % device->info.pageSizeBytes;
        lPageRemain = device->info.pageSizeBytes - lPageOffset;
        lChunkLength = length - lOffset;
        if (lChunkLength > lPageRemain) {
            lChunkLength = lPageRemain;
        }

        lStatus = w25qxxxWriteEnableInternal(device);
        if (lStatus != W25QXXX_STATUS_OK) {
            return lStatus;
        }

        w25qxxxBuildAddressCommand(lHeader, w25qxxxGetProgramCommand(device), address + lOffset, device->info.addressWidth);
        lStatus = w25qxxxTransferInternal(device, lHeader, lHeaderLength, &buffer[lOffset], (uint16_t)lChunkLength, NULL, 0U);
        if (lStatus != W25QXXX_STATUS_OK) {
            return lStatus;
        }

        lStatus = w25qxxxWaitReadyInternal(device, W25QXXX_PAGE_PROGRAM_TIMEOUT_MS);
        if (lStatus != W25QXXX_STATUS_OK) {
            return lStatus;
        }

        lOffset += lChunkLength;
    }

    return W25QXXX_STATUS_OK;
}

eW25qxxxStatus w25qxxxEraseSector(stW25qxxxDevice *device, uint32_t address)
{
    uint8_t lHeader[5];
    uint8_t lHeaderLength;
    eW25qxxxStatus lStatus;

    if (!w25qxxxIsReadyForAccess(device)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    if ((address % device->info.sectorSizeBytes) != 0U) {
        return W25QXXX_STATUS_OUT_OF_RANGE;
    }

    if (!w25qxxxIsRangeValid(device, address, device->info.sectorSizeBytes)) {
        return W25QXXX_STATUS_OUT_OF_RANGE;
    }

    lStatus = w25qxxxWriteEnableInternal(device);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    lHeaderLength = (device->info.addressWidth == 4U) ? 5U : 4U;
    w25qxxxBuildAddressCommand(lHeader, w25qxxxGetSectorEraseCommand(device), address, device->info.addressWidth);
    lStatus = w25qxxxTransferInternal(device, lHeader, lHeaderLength, NULL, 0U, NULL, 0U);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    return w25qxxxWaitReadyInternal(device, W25QXXX_SECTOR_ERASE_TIMEOUT_MS);
}

eW25qxxxStatus w25qxxxEraseBlock64k(stW25qxxxDevice *device, uint32_t address)
{
    uint8_t lHeader[5];
    uint8_t lHeaderLength;
    eW25qxxxStatus lStatus;

    if (!w25qxxxIsReadyForAccess(device)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    if ((address % device->info.blockSizeBytes) != 0U) {
        return W25QXXX_STATUS_OUT_OF_RANGE;
    }

    if (!w25qxxxIsRangeValid(device, address, device->info.blockSizeBytes)) {
        return W25QXXX_STATUS_OUT_OF_RANGE;
    }

    lStatus = w25qxxxWriteEnableInternal(device);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    lHeaderLength = (device->info.addressWidth == 4U) ? 5U : 4U;
    w25qxxxBuildAddressCommand(lHeader, w25qxxxGetBlockEraseCommand(device), address, device->info.addressWidth);
    lStatus = w25qxxxTransferInternal(device, lHeader, lHeaderLength, NULL, 0U, NULL, 0U);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    return w25qxxxWaitReadyInternal(device, W25QXXX_BLOCK_ERASE_TIMEOUT_MS);
}

eW25qxxxStatus w25qxxxEraseChip(stW25qxxxDevice *device)
{
    uint8_t lCommand;
    eW25qxxxStatus lStatus;

    if (!w25qxxxIsReadyForAccess(device)) {
        return W25QXXX_STATUS_NOT_READY;
    }

    lStatus = w25qxxxWriteEnableInternal(device);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    lCommand = W25QXXX_CMD_CHIP_ERASE;
    lStatus = w25qxxxTransferInternal(device, &lCommand, 1U, NULL, 0U, NULL, 0U);
    if (lStatus != W25QXXX_STATUS_OK) {
        return lStatus;
    }

    return w25qxxxWaitReadyInternal(device, W25QXXX_CHIP_ERASE_TIMEOUT_MS);
}
/**************************End of file********************************/
