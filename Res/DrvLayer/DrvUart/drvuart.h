/************************************************************************************
* @file     : drvuart.h
* @brief    : Generic MCU UART driver abstraction.
* @details  : This module defines a stable UART interface for project-level drivers.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRVUART_H
#define DRVUART_H

#include <stdint.h>
#include "drvuart_portmap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRVUART_RECVLEN_DEBUGUART    1024U



typedef enum eDrvUartStatus {
    DRVUART_STATUS_OK = 0,
    DRVUART_STATUS_INVALID_PARAM,
    DRVUART_STATUS_NOT_READY,
    DRVUART_STATUS_BUSY,
    DRVUART_STATUS_UNSUPPORTED,
    DRVUART_STATUS_ERROR,
} eDrvUartStatus;

eDrvUartStatus drvUartInit(eDrvUartPortMap uart);
eDrvUartStatus drvUartTransmit(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length, uint32_t timeoutMs);
eDrvUartStatus drvUartTransmitIt(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length);
eDrvUartStatus drvUartTransmitDma(eDrvUartPortMap uart, const uint8_t *buffer, uint16_t length);
eDrvUartStatus drvUartReceive(eDrvUartPortMap uart, uint8_t *buffer, uint16_t length);
uint16_t drvUartGetDataLen(eDrvUartPortMap uart);

#ifdef __cplusplus
}
#endif

#endif  // DRVUART_H
/**************************End of file********************************/