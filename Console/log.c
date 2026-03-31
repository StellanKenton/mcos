/************************************************************************************
* @file     : log.c
* @brief    : Lightweight logging implementation.
* @details  : Streams formatted output directly to ROM printf to avoid large task stacks.
* @author   : \.rumi
* @date     : 2026-03-31
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "log.h"

#include <inttypes.h>
#include <stddef.h>

#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *logGetLevelLabel(eLogLevel level)
{
    switch (level) {
        case LOG_LEVEL_ERROR:
            return "E";
        case LOG_LEVEL_WARN:
            return "W";
        case LOG_LEVEL_INFO:
            return "I";
        case LOG_LEVEL_DEBUG:
            return "D";
        default:
            return "?";
    }
}

void logVWrite(eLogLevel level, const char *tag, const char *format, va_list args)
{
    const char *lTag = (tag != NULL) ? tag : "app";
    uint32_t lTimestamp = 0U;

    if (format == NULL || level <= LOG_LEVEL_NONE || level > LOG_LEVEL_DEBUG) {
        return;
    }

    lTimestamp = (uint32_t)esp_log_timestamp();

    esp_rom_printf("%s (%" PRIu32 ") %s: ", logGetLevelLabel(level), lTimestamp, lTag);
    esp_rom_vprintf(format, args);
    esp_rom_printf("\n");
}

void logWrite(eLogLevel level, const char *tag, const char *format, ...)
{
    va_list lArgs;

    va_start(lArgs, format);
    logVWrite(level, tag, format, lArgs);
    va_end(lArgs);
}
/**************************End of file********************************/
