/************************************************************************************
* @file     : drvgpio.h
* @brief    : Generic MCU GPIO driver abstraction.
* @details  : This module defines a small GPIO interface for project-level drivers.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRVGPIO_H
#define DRVGPIO_H

#include <stdint.h>
#include "drvgpio_pinmap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRVGPIO_LOG_SUPPORT             1
#define DRVGPIO_LOG_TAG                 "drvGpio"

#define DRVGPIO_CONSOLE_SUPPORT         1


typedef void (*drvGpioBspInitFunc)(void);
typedef void (*drvGpioBspWriteFunc)(eDrvGpioPinMap pin, eDrvGpioPinState state);
typedef eDrvGpioPinState (*drvGpioBspReadFunc)(eDrvGpioPinMap pin);
typedef void (*drvGpioBspToggleFunc)(eDrvGpioPinMap pin);

typedef struct stDrvGpioBspInterface {
    eDrvGpioPinState pinStates[DRVGPIO_MAX];
    drvGpioBspInitFunc init;
    drvGpioBspWriteFunc write;
    drvGpioBspReadFunc read;
    drvGpioBspToggleFunc toggle;
} stDrvGpioBspInterface;

void drvGpioInit(void);
void drvGpioWrite(eDrvGpioPinMap pin, eDrvGpioPinState state);
eDrvGpioPinState drvGpioRead(eDrvGpioPinMap pin);
void drvGpioToggle(eDrvGpioPinMap pin);

#ifdef __cplusplus
}
#endif

#endif  // DRVGPIO_H
/**************************End of file********************************/