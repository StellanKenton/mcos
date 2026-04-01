/***********************************************************************************
* @file     : drvgpio_port.c
* @brief    : GPIO port-layer BSP and console binding implementation.
* @details  : This file binds the generic GPIO driver interface to the board BSP and
*             keeps optional console commands close to the pin mapping layer.
* @author   : GitHub Copilot
* @date     : 2026-03-31
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drvgpio.h"
#include "bspgpio.h"

#if (DRVGPIO_CONSOLE_SUPPORT == 1)
#include <stddef.h>
#include <string.h>

#include "console.h"

typedef struct stDrvGpioConsoleCommandBinding {
    stConsoleCommand command;
    eDrvGpioPinMap pin;
} stDrvGpioConsoleCommandBinding;

static const stDrvGpioConsoleCommandBinding *drvGpioPortFindConsoleBinding(const char *commandName);
static bool drvGpioPortParseConsoleState(const char *argument, eDrvGpioPinState *state);
static eConsoleCommandResult drvGpioPortConsolePinHandler(uint32_t transport, int argc, char *argv[]);

static const stDrvGpioConsoleCommandBinding gDrvGpioConsoleBindings[] = {
    {
        .command = {
            .commandName = "ledr",
            .helpText = "ledr <0|1>",
            .ownerTag = "drvGpio",
            .handler = drvGpioPortConsolePinHandler,
        },
        .pin = DRVGPIO_LEDR,
    },
};
#endif


stDrvGpioBspInterface gDrvGpioBspInterface = {
    .init = bspGpioInit,
    .write = bspGpioWrite,
    .read = bspGpioRead,
    .toggle = bspGpioToggle,
};

#if (DRVGPIO_CONSOLE_SUPPORT == 1)
/**
* @brief : Find the GPIO console binding by command name.
* @param : commandName Console command name.
* @return: Matching binding entry, or NULL when not found.
**/
static const stDrvGpioConsoleCommandBinding *drvGpioPortFindConsoleBinding(const char *commandName)
{
    uint32_t lIndex;

    if (commandName == NULL) {
        return NULL;
    }

    for (lIndex = 0U; lIndex < (uint32_t)(sizeof(gDrvGpioConsoleBindings) / sizeof(gDrvGpioConsoleBindings[0])); ++lIndex) {
        if (strcmp(gDrvGpioConsoleBindings[lIndex].command.commandName, commandName) == 0) {
            return &gDrvGpioConsoleBindings[lIndex];
        }
    }

    return NULL;
}

/**
* @brief : Parse the console on/off argument into logical GPIO state.
* @param : argument Console command state argument.
* @param : state Parsed logical GPIO state output.
* @return: true when the argument is valid.
**/
static bool drvGpioPortParseConsoleState(const char *argument, eDrvGpioPinState *state)
{
    if ((argument == NULL) || (state == NULL) || (argument[1] != '\0')) {
        return false;
    }

    if (argument[0] == '0') {
        *state = DRVGPIO_PIN_SET;
        return true;
    }

    if (argument[0] == '1') {
        *state = DRVGPIO_PIN_RESET;
        return true;
    }

    return false;
}

/**
* @brief : Handle GPIO console commands that map to a single logical pin write.
* @param : transport Console reply transport.
* @param : argc Argument count.
* @param : argv Argument vector.
* @return: Console command execution result.
**/
static eConsoleCommandResult drvGpioPortConsolePinHandler(uint32_t transport, int argc, char *argv[])
{
    const stDrvGpioConsoleCommandBinding *lBinding;
    eDrvGpioPinState lTargetState;

    if ((argc != 2) || (argv == NULL) || (argv[0] == NULL)) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    lBinding = drvGpioPortFindConsoleBinding(argv[0]);
    if (lBinding == NULL) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    if (!drvGpioPortParseConsoleState(argv[1], &lTargetState)) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    drvGpioWrite(lBinding->pin, lTargetState);
    if (consoleReply(transport, "OK") <= 0) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    return CONSOLE_COMMAND_RESULT_OK;
}
#endif

bool drvGpioPortConsoleRegister(void)
{
#if (DRVGPIO_CONSOLE_SUPPORT == 1)
    uint32_t lIndex;

    for (lIndex = 0U; lIndex < (uint32_t)(sizeof(gDrvGpioConsoleBindings) / sizeof(gDrvGpioConsoleBindings[0])); ++lIndex) {
        if (!consoleRegisterCommand(&gDrvGpioConsoleBindings[lIndex].command)) {
            return false;
        }
    }

    return true;
#else
    return false;
#endif
}

/**************************End of file********************************/

