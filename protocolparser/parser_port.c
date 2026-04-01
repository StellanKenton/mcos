/***********************************************************************************
* @file     : parser_port.c
* @brief    : Project port helpers for the stream packet parser.
* @details  : Provides the default millisecond tick source used by timeout logic.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
**********************************************************************************/
#include "parser_port.h"

#include "rep_config.h"

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

uint32_t commPacketParserPortGetTickMs(void)
{
#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
    return 0U;
#endif
}

void commPacketParserPortApplyDefaultConfig(stCommPacketParserConfig *config)
{
    if (config == NULL) {
        return;
    }

    if (config->minHeaderLength == 0U) {
        config->minHeaderLength = config->headerPatternLength;
    }

    if (config->waitPacketTimeoutMs == 0U) {
        config->waitPacketTimeoutMs = COMM_PACKET_PARSER_PORT_WAIT_PACKET_TIMEOUT_MS;
    }

    if (config->getTick == NULL) {
        config->getTick = commPacketParserPortGetTickMs;
    }
}

eCommPacketParserStatus commPacketParserPortInit(stCommPacketParser *parser,
                                                 stRingBuffer *ringBuffer,
                                                 stCommPacketParserConfig *config)
{
    if (config == NULL) {
        return COMM_PACKET_PARSER_INVALID_ARGUMENT;
    }

    commPacketParserPortApplyDefaultConfig(config);
    return commPacketParserInit(parser, ringBuffer, config);
}
/**************************End of file********************************/
