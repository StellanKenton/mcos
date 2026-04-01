/************************************************************************************
* @file     : parser_port.h
* @brief    : Project port helpers for the stream packet parser.
* @details  : Supplies default timing hooks and parser initialization helpers.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
***********************************************************************************/
#ifndef COMM_PACKET_PARSER_PORT_H
#define COMM_PACKET_PARSER_PORT_H

#include <stdint.h>

#include "pareser.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef COMM_PACKET_PARSER_PORT_WAIT_PACKET_TIMEOUT_MS
#define COMM_PACKET_PARSER_PORT_WAIT_PACKET_TIMEOUT_MS    60U
#endif

uint32_t commPacketParserPortGetTickMs(void);
void commPacketParserPortApplyDefaultConfig(stCommPacketParserConfig *config);
eCommPacketParserStatus commPacketParserPortInit(stCommPacketParser *parser, stRingBuffer *ringBuffer, stCommPacketParserConfig *config);

#ifdef __cplusplus
}
#endif

#endif  // COMM_PACKET_PARSER_PORT_H
/**************************End of file********************************/

