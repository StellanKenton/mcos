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

typedef enum eDrvGpioPinState {
    DRVGPIO_PIN_RESET = 0,
    DRVGPIO_PIN_SET
} eDrvGpioPinState;

void drvGpioInit(void);
void drvGpioWrite(eDrvGpioPinMap pin, eDrvGpioPinState state);
eDrvGpioPinState drvGpioRead(eDrvGpioPinMap pin);
void drvGpioToggle(eDrvGpioPinMap pin);

#ifdef __cplusplus
}
#endif

#endif  // DRVGPIO_H
/**************************End of file********************************/