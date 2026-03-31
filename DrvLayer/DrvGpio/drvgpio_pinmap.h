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
    DRVGPIO_LED0,
    DRVGPIO_MAX,
} eDrvGpioPinMap;

const char* g_gpioPinNames[DRVGPIO_MAX] = {
    "led0",
    "max",
};
#ifdef __cplusplus
}
#endif

#endif  // DRVGPIO_PINMAP_H
/**************************End of file********************************/