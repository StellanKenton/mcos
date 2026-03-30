/************************************************************************************
* @file     : bsp_gpio.h
* @brief    : ESP32 GPIO BSP interface.
* @details  : This module keeps ESP-IDF GPIO details inside the BSP layer.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
***********************************************************************************/
#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include "drvgpio.h"

#ifdef __cplusplus
extern "C" {
#endif

void bspGpioInit(void);
void bspGpioWrite(eDrvGpioPinMap pin, eDrvGpioPinState state);
eDrvGpioPinState bspGpioRead(eDrvGpioPinMap pin);
void bspGpioToggle(eDrvGpioPinMap pin);

#ifdef __cplusplus
}
#endif

#endif  // BSP_GPIO_H
/**************************End of file********************************/