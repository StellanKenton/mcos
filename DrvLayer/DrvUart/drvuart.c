/***********************************************************************************
* @file     : drvuart.c
* @brief    : Generic MCU UART driver abstraction implementation.
* @details  : This module provides a small UART interface for project-level drivers.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drvuart.h"

#include <stdbool.h>
#include <stddef.h>

static bool gDrvUartInitialized[DRVUART_MAX];

extern stDrvUartBspInterface gDrvUartBspInterface[DRVUART_MAX];
extern stRingBuffer *drvUartPortGetRingBuffer(eDrvUartPortMap uart);
extern eDrvUartStatus drvUartPortGetStorageConfig(eDrvUartPortMap uart, uint8_t **storage, uint32_t *capacity);

/**
* @brief : Check if the provided logical UART mapping is valid.
* @param : uart UART mapping identifier.
* @return: true if the UART mapping is valid, false otherwise.
**/
static bool drvUartIsValid(eDrvUartPortMap uart)
{
    return (uart < DRVUART_MAX);
}

/**
* @brief : Get the BSP interface entry for a logical UART.
* @param : uart UART mapping identifier.
* @return: Pointer to the BSP interface entry, or NULL when invalid.
**/
static stDrvUartBspInterface *drvUartGetBspInterface(eDrvUartPortMap uart)
{
    if (!drvUartIsValid(uart)) {
        return NULL;
    }

    return &gDrvUartBspInterface[uart];
}

/**
* @brief : Check whether the BSP hook table is complete for basic UART usage.
* @param : None
* @return: true when required hooks are available.
**/
static bool drvUartHasValidBspInterface(eDrvUartPortMap uart)
{
    stDrvUartBspInterface *lBspInterface = drvUartGetBspInterface(uart);

    return (lBspInterface != NULL) &&
           (lBspInterface->init != NULL) &&
           (lBspInterface->transmit != NULL) &&
           (lBspInterface->getDataLen != NULL) &&
           (lBspInterface->receive != NULL);
}

/**
* @brief : Check if the provided UART buffer is valid.
* @param : buffer UART data buffer pointer.
* @param : length UART data length.
* @return: true if the buffer is valid, false otherwise.
**/
static bool drvUartIsValidBuffer(const uint8_t *buffer, uint16_t length)
{
    return (buffer != NULL) && (length > 0U);
}

/**
* @brief : Check whether the logical UART has already been initialized.
* @param : uart UART mapping identifier.
* @return: true if the logical UART is initialized.
**/
static bool drvUartIsInitialized(eDrvUartPortMap uart)
{
    return drvUartIsValid(uart) && gDrvUartInitialized[uart];
}

/**
* @brief : Pull pending bytes from BSP RX storage into the drv ring buffer.
* @param : uart UART mapping identifier.
* @return: UART operation status.
**/
static eDrvUartStatus drvUartSyncRxData(eDrvUartPortMap uart)
{
    uint8_t lScratch[DRVUART_BSP_SYNC_CHUNK_SIZE];
    stRingBuffer *lRingBuffer = NULL;
    stDrvUartBspInterface *lBspInterface = NULL;
    uint32_t lRingFree = 0U;
    uint16_t lPending = 0U;

    if (!drvUartIsInitialized(uart)) {
        return DRVUART_STATUS_NOT_READY;
    }

    lBspInterface = drvUartGetBspInterface(uart);
    if ((lBspInterface == NULL) || (lBspInterface->getDataLen == NULL) || (lBspInterface->receive == NULL)) {
        return DRVUART_STATUS_NOT_READY;
    }

    lRingBuffer = drvUartPortGetRingBuffer(uart);
    if (lRingBuffer == NULL) {
        return DRVUART_STATUS_ERROR;
    }

    lRingFree = ringBufferGetFree(lRingBuffer);
    lPending = lBspInterface->getDataLen(uart);

    while ((lPending > 0U) && (lRingFree > 0U)) {
        uint16_t lChunkLength = lPending;

        if (lChunkLength > DRVUART_BSP_SYNC_CHUNK_SIZE) {
            lChunkLength = DRVUART_BSP_SYNC_CHUNK_SIZE;
        }

        if ((uint32_t)lChunkLength > lRingFree) {
            lChunkLength = (uint16_t)lRingFree;
        }

        if (lBspInterface->receive(uart, lScratch, lChunkLength) != DRVUART_STATUS_OK) {
            return DRVUART_STATUS_ERROR;
        }

        if (ringBufferWrite(lRingBuffer, lScratch, lChunkLength) != (uint32_t)lChunkLength) {
            return DRVUART_STATUS_ERROR;
        }

        lRingFree -= lChunkLength;
        lPending = lBspInterface->getDataLen(uart);
    }

    return DRVUART_STATUS_OK;
}

/**
* @brief : Initialize the UART driver and configure related resources.
* @param : uart UART mapping identifier.
* @return: UART operation status.
**/
eDrvUartStatus drvUartInit(eDrvUartPortMap uart)
{
    uint8_t *lStorage = NULL;
    uint32_t lCapacity = 0U;
    stRingBuffer *lRingBuffer = NULL;
    stDrvUartBspInterface *lBspInterface = NULL;
    eDrvUartStatus lStatus;

    if (!drvUartIsValid(uart)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }

    if (!drvUartHasValidBspInterface(uart)) {
        return DRVUART_STATUS_NOT_READY;
    }

    lBspInterface = drvUartGetBspInterface(uart);
    if (lBspInterface == NULL) {
        return DRVUART_STATUS_NOT_READY;
    }

    lStatus = drvUartPortGetStorageConfig(uart, &lStorage, &lCapacity);
    if (lStatus != DRVUART_STATUS_OK) {
        return lStatus;
    }

    if (lBspInterface->Buffer == NULL) {
        return DRVUART_STATUS_NOT_READY;
    }

    lStorage = lBspInterface->Buffer;

    lRingBuffer = drvUartPortGetRingBuffer(uart);
    if (lRingBuffer == NULL) {
        return DRVUART_STATUS_ERROR;
    }

    if (ringBufferInit(lRingBuffer, lStorage, lCapacity) != RINGBUFFER_OK) {
        return DRVUART_STATUS_ERROR;
    }

    lStatus = lBspInterface->init(uart);
    if (lStatus != DRVUART_STATUS_OK) {
        return lStatus;
    }

    gDrvUartInitialized[uart] = true;
    return DRVUART_STATUS_OK;
}

/**
* @brief : Transmit data through UART in polling mode.
* @param : uart      UART mapping identifier.
* @param : buffer    Pointer to the transmit buffer.
* @param : length    Number of bytes to transmit.
* @param : timeoutMs Timeout in milliseconds.
* @return: UART operation status.
**/
eDrvUartStatus drvUartTransmit(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length, uint32_t timeoutMs)
{
    stDrvUartBspInterface *lBspInterface = NULL;

    if (!drvUartIsValid(uart) || !drvUartIsValidBuffer(buffer, length)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }

    if (!drvUartIsInitialized(uart)) {
        return DRVUART_STATUS_NOT_READY;
    }

    lBspInterface = drvUartGetBspInterface(uart);
    if ((lBspInterface == NULL) || (lBspInterface->transmit == NULL)) {
        return DRVUART_STATUS_NOT_READY;
    }

    return lBspInterface->transmit(uart, buffer, length, timeoutMs);
}

/**
* @brief : Start UART transmit in interrupt mode.
* @param : uart   UART mapping identifier.
* @param : buffer Pointer to the transmit buffer.
* @param : length Number of bytes to transmit.
* @return: UART operation status.
**/
eDrvUartStatus drvUartTransmitIt(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length)
{
    stDrvUartBspInterface *lBspInterface = NULL;

    if (!drvUartIsValid(uart) || !drvUartIsValidBuffer(buffer, length)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }

    if (!drvUartIsInitialized(uart)) {
        return DRVUART_STATUS_NOT_READY;
    }

    lBspInterface = drvUartGetBspInterface(uart);
    if ((lBspInterface == NULL) || (lBspInterface->transmitIt == NULL)) {
        return DRVUART_STATUS_UNSUPPORTED;
    }

    return lBspInterface->transmitIt(uart, buffer, length);
}

/**
* @brief : Start UART transmit in DMA mode.
* @param : uart   UART mapping identifier.
* @param : buffer Pointer to the transmit buffer.
* @param : length Number of bytes to transmit.
* @return: UART operation status.
**/
eDrvUartStatus drvUartTransmitDma(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length)
{
    stDrvUartBspInterface *lBspInterface = NULL;

    if (!drvUartIsValid(uart) || !drvUartIsValidBuffer(buffer, length)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }

    if (!drvUartIsInitialized(uart)) {
        return DRVUART_STATUS_NOT_READY;
    }

    lBspInterface = drvUartGetBspInterface(uart);
    if ((lBspInterface == NULL) || (lBspInterface->transmitDma == NULL)) {
        return DRVUART_STATUS_UNSUPPORTED;
    }

    return lBspInterface->transmitDma(uart, buffer, length);
}

/**
* @brief : Receive data from UART.
* @param : uart   UART mapping identifier.
* @param : buffer Pointer to the receive buffer.
* @param : length Number of bytes to receive.
* @return: UART operation status.
**/
eDrvUartStatus drvUartReceive(eDrvUartPortMap uart, uint8_t *buffer, uint16_t length)
{
    stRingBuffer *lRingBuffer = NULL;
    eDrvUartStatus lStatus;

    if (!drvUartIsValid(uart) || !drvUartIsValidBuffer(buffer, length)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }

    if (!drvUartIsInitialized(uart)) {
        return DRVUART_STATUS_NOT_READY;
    }

    lStatus = drvUartSyncRxData(uart);
    if (lStatus != DRVUART_STATUS_OK) {
        return lStatus;
    }

    lRingBuffer = drvUartPortGetRingBuffer(uart);
    if (lRingBuffer == NULL) {
        return DRVUART_STATUS_ERROR;
    }

    if (ringBufferGetUsed(lRingBuffer) < (uint32_t)length) {
        return DRVUART_STATUS_NOT_READY;
    }

    if (ringBufferRead(lRingBuffer, buffer, length) != (uint32_t)length) {
        return DRVUART_STATUS_ERROR;
    }

    return DRVUART_STATUS_OK;
}

/**
* @brief : Get the amount of data currently available in the UART receive path.
* @param : uart UART mapping identifier.
* @return: Available data length in bytes.
**/
uint16_t drvUartGetDataLen(eDrvUartPortMap uart)
{
    uint32_t lUsed;

    if (!drvUartIsValid(uart)) {
        return 0U;
    }

    if (!drvUartIsInitialized(uart)) {
        return 0U;
    }

    if (drvUartSyncRxData(uart) != DRVUART_STATUS_OK) {
        return 0U;
    }

    lUsed = ringBufferGetUsed(drvUartPortGetRingBuffer(uart));
    if (lUsed > UINT16_MAX) {
        return UINT16_MAX;
    }

    return (uint16_t)lUsed;
}

/**
* @brief : Get the pointer to the UART receive ring buffer.
* @param : uart UART mapping identifier.
* @return: Pointer to the UART receive ring buffer, or NULL if invalid.
**/
stRingBuffer* drvUartGetRingBuffer(eDrvUartPortMap uart)
{
    if (!drvUartIsValid(uart)) {
        return NULL;
    }

    return drvUartPortGetRingBuffer(uart);
}


/**************************End of file********************************/
