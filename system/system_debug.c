/***********************************************************************************
* @file     : system_debug.c
* @brief    : System debug and console command implementation.
* @details  : This file hosts optional console bindings for system debug operations.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "system_debug.h"

#include "console.h"
#include "system.h"

static eConsoleCommandResult systemDebugConsoleVersionHandler(uint32_t transport, int argc, char *argv[]);
static eConsoleCommandResult systemDebugConsoleStatusHandler(uint32_t transport, int argc, char *argv[]);

static const stConsoleCommand gSystemVersionConsoleCommand = {
    .commandName = "verinfo",
    .helpText = "verinfo",
    .ownerTag = "system",
    .handler = systemDebugConsoleVersionHandler,
};

static const stConsoleCommand gSystemStatusConsoleCommand = {
    .commandName = "sysstatus",
    .helpText = "sysstatus",
    .ownerTag = "system",
    .handler = systemDebugConsoleStatusHandler,
};

/**
* @brief : Reply with firmware and hardware version strings.
* @param : transport - console reply transport.
* @param : argc - console argument count.
* @param : argv - console argument vector.
* @return: Console command execution result.
**/
static eConsoleCommandResult systemDebugConsoleVersionHandler(uint32_t transport, int argc, char *argv[])
{
    (void)argv;

    if (argc != 1) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    if (consoleReply(transport,
        "Firmware: %s\nHardware: %s\nOK",
        systemGetFirmwareVersion(),
        systemGetHardwareVersion()) <= 0) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    return CONSOLE_COMMAND_RESULT_OK;
}

/**
* @brief : Reply with current system runtime status.
* @param : transport - console reply transport.
* @param : argc - console argument count.
* @param : argv - console argument vector.
* @return: Console command execution result.
**/
static eConsoleCommandResult systemDebugConsoleStatusHandler(uint32_t transport, int argc, char *argv[])
{
    (void)argv;

    if (argc != 1) {
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    if (consoleReply(transport,
        "Mode: %s\nOK",
        systemGetModeString(systemGetMode())) <= 0) {
        return CONSOLE_COMMAND_RESULT_ERROR;
    }

    return CONSOLE_COMMAND_RESULT_OK;
}

bool systemDebugConsoleRegister(void)
{
    if (!consoleRegisterCommand(&gSystemVersionConsoleCommand)) {
        return false;
    }

    if (!consoleRegisterCommand(&gSystemStatusConsoleCommand)) {
        return false;
    }

    return true;
}
/**************************End of file********************************/
