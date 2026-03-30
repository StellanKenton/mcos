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
#include "ringbuffer.h"
#include <stddef.h>

static stRingBuffer uartRxBuffer[DRVUART_MAX];
static uint8_t uartRxStorage_debug[DRVUART_RECVLEN_DEBUGUART];

/**
* @brief : Check if the provided logical UART mapping is valid.
* @param : uart UART mapping identifier.
* @return: true if the UART mapping is valid, false otherwise.
**/
static bool drvUartIsValid(eDrvUartPortMap uart)
{
    return (uart >= 0) && (uart < DRVUART_MAX);
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
* @brief : Initialize the UART driver and configure related resources.
* @param : uart UART mapping identifier.
* @return: UART operation status.
**/
eDrvUartStatus drvUartInit(eDrvUartPortMap uart)
{
    if (!drvUartIsValid(uart)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }
    // Initial RingBuffer
    for(eDrvUartPortMap i = 0; i < DRVUART_MAX; i++) {
        stRingBuffer *rb = &uartRxBuffer[i];
        uint8_t *storage = NULL;
        uint32_t capacity = 0U;

        switch (i) {
            case DRVUART_DEBUG:
                storage = uartRxStorage_debug;
                capacity = DRVUART_RECVLEN_DEBUGUART;
                break;
            default:
                return DRVUART_STATUS_UNSUPPORTED;
        }

        if (ringBufferInit(rb, storage, capacity) != RINGBUFFER_OK) {
            return DRVUART_STATUS_ERROR;
        }
    }
/*************************Bsp Area**********************/

/*******************************************************/
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
    (void)timeoutMs;

    if (!drvUartIsValid(uart) || !drvUartIsValidBuffer(buffer, length)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }
/*************************Bsp Area**********************/

/*******************************************************/
    return DRVUART_STATUS_OK;
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
    if (!drvUartIsValid(uart) || !drvUartIsValidBuffer(buffer, length)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }
/*************************Bsp Area***********************/

/*******************************************************/
    return DRVUART_STATUS_OK;
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
    if (!drvUartIsValid(uart) || !drvUartIsValidBuffer(buffer, length)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }
/*************************Bsp Area**********************/

/*******************************************************/
    return DRVUART_STATUS_OK;
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
    if (!drvUartIsValid(uart) || !drvUartIsValidBuffer(buffer, length)) {
        return DRVUART_STATUS_INVALID_PARAM;
    }
/*************************Bsp Area**********************/

/*******************************************************/
    return DRVUART_STATUS_OK;
}

/**
* @brief : Get the amount of data currently available in the UART receive path.
* @param : uart UART mapping identifier.
* @return: Available data length in bytes.
**/
uint16_t drvUartGetDataLen(eDrvUartPortMap uart)
{
    if (!drvUartIsValid(uart)) {
        return 0U;
    }
/*************************Bsp Area***********************/

/*******************************************************/
    return 0U;
}

/**************************End of file********************************/