/***********************************************************************************
* @file     : drvgpio_port.c
* @brief    : GPIO port-layer BSP binding implementation.
* @details  : This file binds the generic GPIO driver interface to the board BSP.
* @author   : GitHub Copilot
* @date     : 2026-03-31
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drvgpio.h"
#include "string.h"


stDrvGpioBspInterface gDrvGpioBspInterface = {
    .init = NULL,
    .write = NULL,
    .read = NULL,
    .toggle = NULL,
};

/**************************End of file********************************/

