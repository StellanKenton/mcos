/************************************************************************************
* @file     : drvgpio_debug.h
* @brief    : DrvGpio debug helpers.
* @details  : This header exposes optional debug and console registration hooks.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRVGPIO_DEBUG_H
#define DRVGPIO_DEBUG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct stDrvGpioDebugPinDescriptor {
    eDrvGpioPinMap pin;
    const char *pinName;
    const char *aliasCommandName;
    const char *aliasHelpText;
    bool isReadable;
    bool isWritable;
    bool isToggleSupported;
} stDrvGpioDebugPinDescriptor;

typedef struct stDrvGpioConsoleAliasCommand {
    stConsoleCommand command;
    const stDrvGpioDebugPinDescriptor *pinDescriptor;
} stDrvGpioConsoleAliasCommand;

#define DRVGPIO_DEBUG_PIN_COUNT  ((uint32_t)(sizeof(gDrvGpioDebugPins) / sizeof(gDrvGpioDebugPins[0])))

bool drvGpioDebugConsoleRegister(void);

#ifdef __cplusplus
}
#endif

#endif  // DRVGPIO_DEBUG_H
/**************************End of file********************************/
