/************************************************************************************
* @file     : drvgpio_pinmap.h
* @brief    : Shared GPIO logical pin mapping definitions.
* @details  : This file keeps the project-level logical GPIO enumeration independent
*             from the driver interface declaration.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRVGPIO_PINMAP_H
#define DRVGPIO_PINMAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eDrvGpioPinMap {
    DRVGPIO_LEDR,       // PIN9
    DRVGPIO_LEDG,       // PIN10
    DRVGPIO_LEDB,       // PIN11

    DRVGPIO_KEY1,       // PIN12
    DRVGPIO_MAX,
} eDrvGpioPinMap;

#ifdef __cplusplus
}
#endif

#endif  // DRVGPIO_PINMAP_H
/**************************End of file********************************/