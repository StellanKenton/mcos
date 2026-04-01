/************************************************************************************
* @file     : pareser.h
* @brief    : Stream packet parser public API.
* @details  : Reassembles complete packets from a byte stream ring buffer.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
***********************************************************************************/
#ifndef COMM_PACKET_PARSER_H
#define COMM_PACKET_PARSER_H

#include <stdbool.h>
#include <stdint.h>

#include "ringbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eCommPacketParserStatus {
    COMM_PACKET_PARSER_OK = 0,
    COMM_PACKET_PARSER_EMPTY,
    COMM_PACKET_PARSER_NEED_MORE_DATA,
    COMM_PACKET_PARSER_INVALID_ARGUMENT,
    COMM_PACKET_PARSER_HEADER_NOT_FOUND,
    COMM_PACKET_PARSER_HEADER_INVALID,
    COMM_PACKET_PARSER_LENGTH_INVALID,
    COMM_PACKET_PARSER_CRC_FAILED,
    COMM_PACKET_PARSER_OUTPUT_BUFFER_TOO_SMALL
} eCommPacketParserStatus;

typedef enum eCommPacketParserCrcEndian {
    COMM_PACKET_PARSER_CRC_ENDIAN_LITTLE = 0,
    COMM_PACKET_PARSER_CRC_ENDIAN_BIG
} eCommPacketParserCrcEndian;

typedef uint32_t (*commPacketParserHeaderLengthFunc)(const uint8_t *buffer, uint32_t availableLength, void *userContext);
typedef uint32_t (*commPacketParserPacketLengthFunc)(const uint8_t *buffer, uint32_t headerLength, uint32_t availableLength, void *userContext);
typedef uint32_t (*commPacketParserCrcCalculateFunc)(const uint8_t *buffer, uint32_t length, void *userContext);
typedef uint32_t (*commPacketParserGetTickFunc)(void);

typedef struct stCommPacketParserPacket {
    const uint8_t *buffer;
    uint16_t length;
} stCommPacketParserPacket;

typedef struct stCommPacketParserConfig {
    const uint8_t *headerPattern;
    uint16_t headerPatternLength;
    uint16_t minHeaderLength;
    uint16_t minPacketLength;
    uint16_t maxPacketLength;
    uint16_t waitPacketTimeoutMs;
    int32_t crcRangeStartOffset;
    int32_t crcRangeEndOffset;
    int32_t crcFieldOffset;
    uint8_t crcFieldLength;
    eCommPacketParserCrcEndian crcFieldEndian;
    uint8_t *outputBuffer;
    uint16_t outputBufferSize;
    commPacketParserHeaderLengthFunc headerLengthFunc;
    commPacketParserPacketLengthFunc packetLengthFunc;
    commPacketParserCrcCalculateFunc crcCalculateFunc;
    commPacketParserGetTickFunc getTick;
    void *userContext;
} stCommPacketParserConfig;

typedef struct stCommPacketParser {
    stRingBuffer *ringBuffer;
    stCommPacketParserConfig config;
    stCommPacketParserPacket packet;
    uint32_t pendingPacketTick;
    uint16_t pendingPacketLength;
    bool hasPendingPacket;
    bool hasReadyPacket;
    bool isInitialized;
} stCommPacketParser;

bool commPacketParserIsConfigValid(const stCommPacketParserConfig *config);
eCommPacketParserStatus commPacketParserInit(stCommPacketParser *parser, stRingBuffer *ringBuffer, const stCommPacketParserConfig *config);
void commPacketParserReset(stCommPacketParser *parser);
eCommPacketParserStatus commPacketParserProcess(stCommPacketParser *parser, stCommPacketParserPacket *packet);
bool commPacketParserHasReadyPacket(const stCommPacketParser *parser);
const stCommPacketParserPacket *commPacketParserGetReadyPacket(const stCommPacketParser *parser);
void commPacketParserReleasePacket(stCommPacketParser *parser);

#ifdef __cplusplus
}
#endif

#endif  // COMM_PACKET_PARSER_H
/**************************End of file********************************/

