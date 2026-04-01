/***********************************************************************************
* @file     : drvgpio_debug.c
* @brief    : DrvGpio debug and console command implementation.
* @details  : This file hosts optional console bindings for GPIO debug operations.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drvgpio_debug.h"

#include "drvgpio.h"

#if (DRVGPIO_CONSOLE_SUPPORT == 1)
#include <stddef.h>
#include <string.h>

#include "console.h"

static const stDrvGpioDebugPinDescriptor *drvGpioDebugFindPinByName(const char *pinName);
static const stDrvGpioConsoleAliasCommand *drvGpioDebugFindAliasCommand(const char *commandName);
static bool drvGpioDebugParseConsoleState(const char *argument, eDrvGpioPinState *state);
static const char *drvGpioDebugGetStateText(eDrvGpioPinState state);
static eConsoleCommandResult drvGpioDebugReplyPinState(uint32_t transport, const stDrvGpioDebugPinDescriptor *pinDescriptor);
static eConsoleCommandResult drvGpioDebugReplyPinList(uint32_t transport);
static eConsoleCommandResult drvGpioDebugConsoleAliasHandler(uint32_t transport, int argc, char *argv[]);
static eConsoleCommandResult drvGpioDebugConsoleHandler(uint32_t transport, int argc, char *argv[]);

static const stDrvGpioDebugPinDescriptor gDrvGpioDebugPins[] = {
    {
        .pin = DRVGPIO_LEDR,
        .pinName = "ledr",
        .aliasCommandName = "ledr",
        .aliasHelpText = "ledr <0|1|on|off>",
        .isReadable = true,
        .isWritable = true,
        .isToggleSupported = true,
    },
    {
        .pin = DRVGPIO_LEDG,
        .pinName = "ledg",
        .aliasCommandName = "ledg",
        .aliasHelpText = "ledg <0|1|on|off>",
        .isReadable = true,
        .isWritable = true,
        .isToggleSupported = true,
    },
    {
        .pin = DRVGPIO_LEDB,
        .pinName = "ledb",
        .aliasCommandName = "ledb",
        .aliasHelpText = "ledb <0|1|on|off>",
        .isReadable = true,
        .isWritable = true,
        .isToggleSupported = true,
    },
    {
        .pin = DRVGPIO_KEY1,
        .pinName = "key1",
        .aliasCommandName = NULL,
        .aliasHelpText = NULL,
        .isReadable = true,
        .isWritable = false,
        .isToggleSupported = false,
    },
};

static stDrvGpioConsoleAliasCommand gDrvGpioAliasCommands[DRVGPIO_DEBUG_PIN_COUNT];

static const stConsoleCommand gDrvGpioConsoleCommand = {
    .commandName = "gpio",
    .helpText = "gpio <list|get|set|toggle> [pin] [0|1|on|off]",
    .ownerTag = "drvGpio",
    .handler = drvGpioDebugConsoleHandler,
};

/**
* @brief : Find a GPIO pin descriptor by logical pin name.
* @param : pinName Logical pin name.
* @return: Matching descriptor entry, or NULL when not found.
**/
static const stDrvGpioDebugPinDescriptor *drvGpioDebugFindPinByName(const char *pinName)
{
    uint32_t lIndex;

    if (pinName == NULL) {
        return NULL;
    }

    for (lIndex = 0U; lIndex < DRVGPIO_DEBUG_PIN_COUNT; ++lIndex) {
        if (strcmp(gDrvGpioDebugPins[lIndex].pinName, pinName) == 0) {
            return &gDrvGpioDebugPins[lIndex];
        }
    }

    return NULL;
}

/**
* @brief : Find the GPIO alias command metadata by command name.
* @param : commandName Console command name.
* @return: Matching alias entry, or NULL when not found.
**/
static const stDrvGpioConsoleAliasCommand *drvGpioDebugFindAliasCommand(const char *commandName)
{
    uint32_t lIndex;

    if (commandName == NULL) {
        return NULL;
    }

    for (lIndex = 0U; lIndex < DRVGPIO_DEBUG_PIN_COUNT; ++lIndex) {
        if ((gDrvGpioAliasCommands[lIndex].command.commandName != NULL) &&
            (strcmp(gDrvGpioAliasCommands[lIndex].command.commandName, commandName) == 0)) {
            return &gDrvGpioAliasCommands[lIndex];
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
static bool drvGpioDebugParseConsoleState(const char *argument, eDrvGpioPinState *state)
{
    if ((argument == NULL) || (state == NULL)) {
        return false;
    }

    if ((strcmp(argument, "0") == 0) ||
        (strcmp(argument, "on") == 0) ||
        (strcmp(argument, "set") == 0) ||
        (strcmp(argument, "active") == 0)) {
        *state = DRVGPIO_PIN_SET;
        return true;
    }

    if ((strcmp(argument, "1") == 0) ||
        (strcmp(argument, "off") == 0) ||
        (strcmp(argument, "reset") == 0) ||
        (strcmp(argument, "inactive") == 0)) {
        *state = DRVGPIO_PIN_RESET;
        return true;
    }

    return false;
}

/**
* @brief : Convert a GPIO state into console text.
* @param : state Logical GPIO state.
* @return: Text description for console replies.
**/
static const char *drvGpioDebugGetStateText(eDrvGpioPinState state)
{
    if (state == DRVGPIO_PIN_SET) {
        return "set";
    }

    if (state == DRVGPIO_PIN_RESET) {
        return "reset";
    }

    return "invalid";
}

/**
* @brief : Reply with the current state of a logical pin.
* @param : transport Console reply transport.
* @param : pinDescriptor Target pin descriptor.
* @return: Console command execution result.
**/
static eConsoleCommandResult drvGpioDebugReplyPinState(uint32_t transport, const stDrvGpioDebugPinDescriptor *pinDescriptor)
{
    eDrvGpioPinState lState;

    if ((pinDescriptor == NULL) || !pinDescriptor->isReadable) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    lState = drvGpioRead(pinDescriptor->pin);
    if (lState == DRVGPIO_PIN_STATE_INVALID) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    if (consoleReply(transport, "%s=%s\nOK", pinDescriptor->pinName, drvGpioDebugGetStateText(lState)) <= 0) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    return CONSOLE_COMMAND_RESULT_OK;
}

/**
* @brief : Reply with all registered logical pins and capabilities.
* @param : transport Console reply transport.
* @return: Console command execution result.
**/
static eConsoleCommandResult drvGpioDebugReplyPinList(uint32_t transport)
{
    uint32_t lIndex;
    int32_t lReplyResult;

    for (lIndex = 0U; lIndex < DRVGPIO_DEBUG_PIN_COUNT; ++lIndex) {
        lReplyResult = consoleReply(transport,
            "%s read=%s write=%s toggle=%s\n",
            gDrvGpioDebugPins[lIndex].pinName,
            gDrvGpioDebugPins[lIndex].isReadable ? "yes" : "no",
            gDrvGpioDebugPins[lIndex].isWritable ? "yes" : "no",
            gDrvGpioDebugPins[lIndex].isToggleSupported ? "yes" : "no");
        if (lReplyResult <= 0) {
            return CONSOLE_COMMAND_RESULT_ERROR;
        }
    }

    if (consoleReply(transport, "OK") <= 0) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    return CONSOLE_COMMAND_RESULT_OK;
}

/**
* @brief : Handle GPIO alias console commands such as ledr and ledg.
* @param : transport Console reply transport.
* @param : argc Argument count.
* @param : argv Argument vector.
* @return: Console command execution result.
**/
static eConsoleCommandResult drvGpioDebugConsoleAliasHandler(uint32_t transport, int argc, char *argv[])
{
    const stDrvGpioConsoleAliasCommand *lAliasCommand;
    eDrvGpioPinState lTargetState;

    if ((argc != 2) || (argv == NULL) || (argv[0] == NULL)) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    lAliasCommand = drvGpioDebugFindAliasCommand(argv[0]);
    if ((lAliasCommand == NULL) || (lAliasCommand->pinDescriptor == NULL) || !lAliasCommand->pinDescriptor->isWritable) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    if (!drvGpioDebugParseConsoleState(argv[1], &lTargetState)) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    drvGpioWrite(lAliasCommand->pinDescriptor->pin, lTargetState);
    if (consoleReply(transport, "OK") <= 0) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    return CONSOLE_COMMAND_RESULT_OK;
}

/**
* @brief : Handle the generic GPIO console command.
* @param : transport Console reply transport.
* @param : argc Argument count.
* @param : argv Argument vector.
* @return: Console command execution result.
**/
static eConsoleCommandResult drvGpioDebugConsoleHandler(uint32_t transport, int argc, char *argv[])
{
    const stDrvGpioDebugPinDescriptor *lPinDescriptor;
    eDrvGpioPinState lTargetState;

    if ((argc < 2) || (argv == NULL) || (argv[1] == NULL)) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    if (strcmp(argv[1], "list") == 0) {
        if (argc != 2) {
            return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
        }

        return drvGpioDebugReplyPinList(transport);
    }

    if (argc < 3) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    lPinDescriptor = drvGpioDebugFindPinByName(argv[2]);
    if (lPinDescriptor == NULL) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    if ((strcmp(argv[1], "get") == 0) || (strcmp(argv[1], "read") == 0)) {
        if (argc != 3) {
            return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
        }

        return drvGpioDebugReplyPinState(transport, lPinDescriptor);
    }

    if ((strcmp(argv[1], "set") == 0) || (strcmp(argv[1], "write") == 0)) {
        if ((argc != 4) || !lPinDescriptor->isWritable) {
            return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
        }

        if (!drvGpioDebugParseConsoleState(argv[3], &lTargetState)) {
            return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
        }

        drvGpioWrite(lPinDescriptor->pin, lTargetState);
        if (consoleReply(transport, "OK") <= 0) {
            return CONSOLE_COMMAND_RESULT_ERROR;
        }

        return CONSOLE_COMMAND_RESULT_OK;
    }

    if (strcmp(argv[1], "toggle") == 0) {
        if ((argc != 3) || !lPinDescriptor->isToggleSupported) {
            return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
        }

        drvGpioToggle(lPinDescriptor->pin);
        if (consoleReply(transport, "OK") <= 0) {
            return CONSOLE_COMMAND_RESULT_ERROR;
        }

        return CONSOLE_COMMAND_RESULT_OK;
    }

    return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
}
#endif

bool drvGpioDebugConsoleRegister(void)
{
#if (DRVGPIO_CONSOLE_SUPPORT == 1)
    uint32_t lIndex;

    if (!consoleRegisterCommand(&gDrvGpioConsoleCommand)) {
        return false;
    }

    for (lIndex = 0U; lIndex < DRVGPIO_DEBUG_PIN_COUNT; ++lIndex) {
        if (gDrvGpioDebugPins[lIndex].aliasCommandName == NULL) {
            continue;
        }

        gDrvGpioAliasCommands[lIndex].command.commandName = gDrvGpioDebugPins[lIndex].aliasCommandName;
        gDrvGpioAliasCommands[lIndex].command.helpText = gDrvGpioDebugPins[lIndex].aliasHelpText;
        gDrvGpioAliasCommands[lIndex].command.ownerTag = "drvGpio";
        gDrvGpioAliasCommands[lIndex].command.handler = drvGpioDebugConsoleAliasHandler;
        gDrvGpioAliasCommands[lIndex].pinDescriptor = &gDrvGpioDebugPins[lIndex];

        if (!consoleRegisterCommand(&gDrvGpioAliasCommands[lIndex].command)) {
            return false;
        }
    }

    return true;
#else
    return false;
#endif
}

/**************************End of file********************************/
