/***********************************************************************************
* @file     : bsp_gpio.c
* @brief    : ESP32 GPIO BSP implementation.
* @details  : This module maps logical GPIO identifiers to ESP-IDF GPIO resources.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
**********************************************************************************/
#include "bsp_gpio.h"

#include <stdbool.h>

#include "driver/gpio.h"

typedef enum eBspGpioDirection {
    BSPGPIO_DIRECTION_INPUT = 0,
    BSPGPIO_DIRECTION_OUTPUT,
} eBspGpioDirection;

typedef struct stBspGpioPinConfig {
    gpio_num_t gpioNum;
    eBspGpioDirection direction;
    gpio_pull_mode_t pullMode;
    bool activeHigh;
    eDrvGpioPinState defaultState;
} stBspGpioPinConfig;

static const stBspGpioPinConfig gBspGpioPinConfig[DRVGPIO_MAX] = {
    [DRVGPIO_LEDR] = {
        .gpioNum = GPIO_NUM_9,
        .direction = BSPGPIO_DIRECTION_OUTPUT,
        .pullMode = GPIO_FLOATING,
        .activeHigh = true,
        .defaultState = DRVGPIO_PIN_RESET,
    },
    [DRVGPIO_LEDG] = {
        .gpioNum = GPIO_NUM_10,
        .direction = BSPGPIO_DIRECTION_OUTPUT,
        .pullMode = GPIO_FLOATING,
        .activeHigh = true,
        .defaultState = DRVGPIO_PIN_RESET,
    },
    [DRVGPIO_LEDB] = {
        .gpioNum = GPIO_NUM_11,
        .direction = BSPGPIO_DIRECTION_OUTPUT,
        .pullMode = GPIO_FLOATING,
        .activeHigh = true,
        .defaultState = DRVGPIO_PIN_RESET,
    },
    [DRVGPIO_KEY1] = {
        .gpioNum = GPIO_NUM_12,
        .direction = BSPGPIO_DIRECTION_INPUT,
        .pullMode = GPIO_PULLUP_ONLY,
        .activeHigh = false,
        .defaultState = DRVGPIO_PIN_RESET,
    },
};

static bool bspGpioIsValidPin(eDrvGpioPinMap pin)
{
    return (pin >= 0) && (pin < DRVGPIO_MAX);
}

static int bspGpioLogicalToPhysicalLevel(eDrvGpioPinMap pin, eDrvGpioPinState state)
{
    const stBspGpioPinConfig *lPinConfig = &gBspGpioPinConfig[pin];
    bool lLogicalHigh = (state == DRVGPIO_PIN_SET);

    return (lLogicalHigh == lPinConfig->activeHigh) ? 1 : 0;
}

static eDrvGpioPinState bspGpioPhysicalToLogicalState(eDrvGpioPinMap pin, int level)
{
    const stBspGpioPinConfig *lPinConfig = &gBspGpioPinConfig[pin];
    bool lLogicalHigh = (level != 0) == lPinConfig->activeHigh;

    return lLogicalHigh ? DRVGPIO_PIN_SET : DRVGPIO_PIN_RESET;
}

static void bspGpioApplyConfig(const stBspGpioPinConfig *pinConfig)
{
    gpio_config_t lConfig = {
        .pin_bit_mask = (1ULL << pinConfig->gpioNum),
        .mode = (pinConfig->direction == BSPGPIO_DIRECTION_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
        .pull_up_en = (pinConfig->pullMode == GPIO_PULLUP_ONLY || pinConfig->pullMode == GPIO_PULLUP_PULLDOWN) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (pinConfig->pullMode == GPIO_PULLDOWN_ONLY || pinConfig->pullMode == GPIO_PULLUP_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    (void)gpio_config(&lConfig);

    if (pinConfig->direction == BSPGPIO_DIRECTION_OUTPUT) {
        (void)gpio_set_level(pinConfig->gpioNum,
                             bspGpioLogicalToPhysicalLevel((eDrvGpioPinMap)(pinConfig - gBspGpioPinConfig), pinConfig->defaultState));
    }
}

void bspGpioInit(void)
{
    int lPinIndex = 0;

    for (lPinIndex = 0; lPinIndex < (int)DRVGPIO_MAX; ++lPinIndex) {
        bspGpioApplyConfig(&gBspGpioPinConfig[lPinIndex]);
    }
}

void bspGpioWrite(eDrvGpioPinMap pin, eDrvGpioPinState state)
{
    if (!bspGpioIsValidPin(pin)) {
        return;
    }

    if (gBspGpioPinConfig[pin].direction != BSPGPIO_DIRECTION_OUTPUT) {
        return;
    }

    (void)gpio_set_level(gBspGpioPinConfig[pin].gpioNum, bspGpioLogicalToPhysicalLevel(pin, state));
}

eDrvGpioPinState bspGpioRead(eDrvGpioPinMap pin)
{
    if (!bspGpioIsValidPin(pin)) {
        return DRVGPIO_PIN_RESET;
    }

    return bspGpioPhysicalToLogicalState(pin, gpio_get_level(gBspGpioPinConfig[pin].gpioNum));
}

void bspGpioToggle(eDrvGpioPinMap pin)
{
    eDrvGpioPinState lCurrentState;

    if (!bspGpioIsValidPin(pin)) {
        return;
    }

    if (gBspGpioPinConfig[pin].direction != BSPGPIO_DIRECTION_OUTPUT) {
        return;
    }

    lCurrentState = bspGpioRead(pin);
    bspGpioWrite(pin, (lCurrentState == DRVGPIO_PIN_SET) ? DRVGPIO_PIN_RESET : DRVGPIO_PIN_SET);
}

/**************************End of file********************************/