/************************************************************************************
* @file     : drvuart_portmap.h
* @brief    : Shared UART logical port mapping definitions.
* @details  : This file keeps the project-level logical UART identifiers independent
*             from the driver interface declaration.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRVUART_PORTMAP_H
#define DRVUART_PORTMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define DRVGPIO_LOG_SUPPORT             1
#define DRVGPIO_CONSOLE_SUPPORT         0

#define DRVUART_RECVLEN_DEBUGUART    1024U

typedef enum eDrvUartPortMapTable {
    DRVUART_DEBUG = 0,      // PA9 PA10
    DRVUART_MAX,
} eDrvUartPortMap;

#ifdef __cplusplus
}
#endif

#endif  // DRVUART_PORTMAP_H
/**************************End of file********************************/
