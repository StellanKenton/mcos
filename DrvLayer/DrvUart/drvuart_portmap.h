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

typedef enum eDrvUartPortMap {
    DRVUART_DEBUG = 0,
    DRVUART_MAX,
} eDrvUartPortMap;

#ifdef __cplusplus
}
#endif

#endif  // DRVUART_PORTMAP_H
/**************************End of file********************************/