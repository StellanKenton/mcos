/***********************************************************************************
* @file     : drvgpio.c
* @brief    : Generic MCU GPIO driver abstraction implementation.
* @details  : The platform-specific GPIO operations are provided through port hooks.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drvgpio.h"
#include <stddef.h>

eDrvGpioPinState g_gpioPinStates[DRVGPIO_MAX] = {0}; 

/**
* @brief : Check if the provided logical pin mapping is valid.
* @param : pin GPIO pin mapping identifier.
* @return: true if the pin mapping is valid, false otherwise.
**/
static bool drvGpioIsValidPin(eDrvGpioPinMap pin)
{
	return (pin >= 0) && (pin < DRVGPIO_MAX);
}

/**
* @brief : Initialize the GPIO driver and configure pins.
* @param : None
* @return: None
**/
void drvGpioInit(void)
{
/*************************Bsp Area***********************/

/*******************************************************/
}

/**
* @brief : Drive the specified GPIO pin to the requested logic state.
* @param : pin   GPIO pin mapping identifier.
* @param : state Target GPIO output state.
* @return: None
**/
void drvGpioWrite(eDrvGpioPinMap pin, eDrvGpioPinState state)
{
    if (!drvGpioIsValidPin(pin)) {
        return;
    }
    g_gpioPinStates[pin] = state;

/*************************Bsp Area***********************/

/*******************************************************/
}

/**
* @brief : Read the current logic state of the specified GPIO pin.
* @param : pin GPIO pin mapping identifier.
* @return: Current GPIO pin state.
**/
eDrvGpioPinState drvGpioRead(eDrvGpioPinMap pin)
{
    eDrvGpioPinState state;
    if (!drvGpioIsValidPin(pin)) {
        return DRVGPIO_PIN_STATE_INVALID;
    }
/*************************Bsp Area***********************/

/*******************************************************/
    g_gpioPinStates[pin] = state;
    return state;
}

/**
* @brief : Toggle the output state of the specified GPIO pin.
* @param : pin GPIO pin mapping identifier.
* @return: None
**/
void drvGpioToggle(eDrvGpioPinMap pin)
{
    if (!drvGpioIsValidPin(pin)) {
        return;
    }
/*************************Bsp Area***********************/

/*******************************************************/
    g_gpioPinStates[pin] = (g_gpioPinStates[pin] == DRVGPIO_PIN_SET) ? DRVGPIO_PIN_RESET: DRVGPIO_PIN_SET;
}


/**************************End of file********************************/