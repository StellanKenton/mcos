/************************************************************************************
* @file     : log.c
* @brief    : Lightweight logging implementation.
* @details  : Formats log lines once and dispatches them through fixed hook interfaces.
* @author   : \.rumi
* @date     : 2026-03-31
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "log.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../../../SEGGER/bsp_rtt.h"
#include "../drvlayer/drvuart/drvuart.h"

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_ESP32)
#include "esp_log.h"
#endif

static logTimestampProvider gLogTimestampProvider = NULL;
static bool gLogIsInitialized = false;

static const char *logGetLevelLabel(eLogLevel level);
static bool logIsValidOutputInterface(const stLogInterface *interface);
static bool logIsValidInputInterface(const stLogInterface *interface);
static uint32_t logGetAvailableInterfaceCount(void);
static uint32_t logGetInterfaceCount(void);

static uint32_t logGetDefaultTimestamp(void)
{
#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_ESP32)
    return (uint32_t)esp_log_timestamp();
#elif (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
    return 0U;
#endif
}

static stLogInterface gLogInterfaces[] = {
    {
        .transport = LOG_TRANSPORT_RTT,
        .init = bspRttLogInit,
        .write = bspRttLogWrite,
        .getBuffer = bspRttLogGetInputBuffer,
        .isOutputEnabled = true,
        .isInputEnabled = true,
    },
    {
        .transport = LOG_TRANSPORT_UART,
        .init = drvUartLogInit,
        .write = drvUartLogWrite,
        .getBuffer = drvUartLogGetInputBuffer,
        .isOutputEnabled = true,
        .isInputEnabled = true,
    },
};

static bool logIsValidOutputInterface(const stLogInterface *interface)
{
    return (interface != NULL) &&
           (interface->transport != LOG_TRANSPORT_NONE) &&
           (interface->write != NULL) &&
           (interface->isOutputEnabled == true);
}

static bool logIsValidInputInterface(const stLogInterface *interface)
{
    return (interface != NULL) &&
           (interface->transport != LOG_TRANSPORT_NONE) &&
           (interface->getBuffer != NULL) &&
           (interface->isInputEnabled == true);
}

static uint32_t logGetAvailableInterfaceCount(void)
{
    return (uint32_t)(sizeof(gLogInterfaces) / sizeof(gLogInterfaces[0]));
}

static uint32_t logGetInterfaceCount(void)
{
    uint32_t lAvailableCount = logGetAvailableInterfaceCount();

    if (REP_LOG_OUTPUT_PORT < lAvailableCount) {
    return REP_LOG_OUTPUT_PORT;
    }

    return lAvailableCount;
}

static bool logIsValidLevel(eLogLevel level)
{
    return (level > LOG_LEVEL_NONE) && (level <= LOG_LEVEL_DEBUG);
}

static uint16_t logFormatLine(char *buffer, uint16_t capacity, eLogLevel level, const char *tag, const char *format, va_list args)
{
    const char *lTag = (tag != NULL) ? tag : "app";
    uint32_t lTimestamp = 0U;
    int lPrefixLength = 0;
    int lMessageLength = 0;
    uint16_t lUsed = 0U;
    va_list lArgsCopy;

    if (buffer == NULL || capacity == 0U || format == NULL || !logIsValidLevel(level)) {
        return 0U;
    }

    if (gLogTimestampProvider == NULL) {
        gLogTimestampProvider = logGetDefaultTimestamp;
    }

    lTimestamp = gLogTimestampProvider();
    lPrefixLength = snprintf(buffer, capacity, "%s (%" PRIu32 ") %s: ", logGetLevelLabel(level), lTimestamp, lTag);
    if (lPrefixLength < 0) {
        return 0U;
    }

    if ((uint16_t)lPrefixLength >= capacity) {
        buffer[capacity - 1U] = '\n';
        return capacity;
    }

    va_copy(lArgsCopy, args);
    lMessageLength = vsnprintf(&buffer[lPrefixLength], capacity - (uint16_t)lPrefixLength, format, lArgsCopy);
    va_end(lArgsCopy);

    if (lMessageLength < 0) {
        return 0U;
    }

    lUsed = (uint16_t)lPrefixLength;
    if ((uint16_t)lMessageLength >= (capacity - lUsed)) {
        lUsed = capacity - 1U;
    } else {
        lUsed = lUsed + (uint16_t)lMessageLength;
    }

    if (lUsed >= capacity) {
        lUsed = capacity - 1U;
    }

    if (lUsed < (capacity - 1U)) {
        buffer[lUsed] = '\n';
        lUsed++;
        buffer[lUsed] = '\0';
    } else {
        buffer[capacity - 2U] = '\n';
        buffer[capacity - 1U] = '\0';
        lUsed = capacity - 1U;
    }

    return lUsed;
}

bool logInit(void)
{
    uint32_t lIndex = 0U;

    if (gLogIsInitialized) {
        return true;
    }

    gLogTimestampProvider = logGetDefaultTimestamp;
    gLogIsInitialized = true;

    for (lIndex = 0U; lIndex < logGetInterfaceCount(); lIndex++) {
        if ((logIsValidOutputInterface(&gLogInterfaces[lIndex]) || logIsValidInputInterface(&gLogInterfaces[lIndex])) &&
            (gLogInterfaces[lIndex].init != NULL)) {
            gLogInterfaces[lIndex].init();
        }
    }

    return true;
}

uint32_t logGetInputCount(void)
{
    uint32_t lIndex = 0U;
    uint32_t lCount = 0U;

    for (lIndex = 0U; lIndex < logGetInterfaceCount(); lIndex++) {
        if (logIsValidInputInterface(&gLogInterfaces[lIndex])) {
            lCount++;
        }
    }

    return lCount;
}

stRingBuffer *logGetInputBuffer(uint32_t transport)
{
    uint32_t lIndex = 0U;

    for (lIndex = 0U; lIndex < logGetInterfaceCount(); lIndex++) {
        if (!logIsValidInputInterface(&gLogInterfaces[lIndex])) {
            continue;
        }

        if (gLogInterfaces[lIndex].transport == transport) {
            return gLogInterfaces[lIndex].getBuffer();
        }
    }

    return NULL;
}

void logSetTimestampProvider(logTimestampProvider provider)
{
    gLogTimestampProvider = (provider != NULL) ? provider : logGetDefaultTimestamp;
}

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
    char lBuffer[LOG_LINE_BUFFER_SIZE];
    uint16_t lLength = 0U;
    uint32_t lIndex = 0U;

    if (!logInit()) {
        return;
    }

    lLength = logFormatLine(lBuffer, (uint16_t)sizeof(lBuffer), level, tag, format, args);
    if (lLength == 0U) {
        return;
    }

    for (lIndex = 0U; lIndex < logGetInterfaceCount(); lIndex++) {
        if (!logIsValidOutputInterface(&gLogInterfaces[lIndex])) {
            continue;
        }

        (void)gLogInterfaces[lIndex].write((const uint8_t *)lBuffer, lLength);
    }
}

void logWrite(eLogLevel level, const char *tag, const char *format, ...)
{
    va_list lArgs;

    va_start(lArgs, format);
    logVWrite(level, tag, format, lArgs);
    va_end(lArgs);
}
/**************************End of file********************************/
