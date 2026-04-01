/***********************************************************************************
* @file     : pareser.c
* @brief    : Stream packet parser implementation.
* @details  : Locates packet headers in a byte stream and validates complete frames.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
**********************************************************************************/
#include "pareser.h"

#include <string.h>

typedef struct stCommPacketParserHeaderSearchResult {
    uint32_t discardLength;
    uint32_t partialHeaderLength;
    bool isFound;
} stCommPacketParserHeaderSearchResult;

static uint32_t commPacketParserMinU32(uint32_t left, uint32_t right);
static uint32_t commPacketParserToPhysicalIndex(const stRingBuffer *ringBuffer, uint32_t logicalIndex);
static uint32_t commPacketParserPeekOffset(const stRingBuffer *ringBuffer, uint32_t offset, uint8_t *buffer, uint32_t length);
static bool commPacketParserPeekByteAt(const stRingBuffer *ringBuffer, uint32_t offset, uint8_t *value);
static bool commPacketParserIsHeaderMatchAt(const stCommPacketParser *parser, uint32_t offset);
static uint32_t commPacketParserFindPartialHeaderLength(const stCommPacketParser *parser, uint32_t usedLength);
static stCommPacketParserHeaderSearchResult commPacketParserSearchHeader(const stCommPacketParser *parser, uint32_t usedLength);
static void commPacketParserClearPendingState(stCommPacketParser *parser);
static uint32_t commPacketParserGetTickMs(const stCommPacketParser *parser);
static bool commPacketParserIsPacketTimedOut(const stCommPacketParser *parser);
static bool commPacketParserResolveOffset(uint32_t packetLength, int32_t offset, uint32_t *resolvedOffset);
static bool commPacketParserGetPacketCrcValue(const stCommPacketParserConfig *config, const uint8_t *packetBuffer, uint32_t packetLength, uint32_t *packetCrcValue);
static bool commPacketParserCheckPacketCrc(const stCommPacketParserConfig *config, const uint8_t *packetBuffer, uint32_t packetLength);

static uint32_t commPacketParserMinU32(uint32_t left, uint32_t right)
{
    return (left < right) ? left : right;
}

static uint32_t commPacketParserToPhysicalIndex(const stRingBuffer *ringBuffer, uint32_t logicalIndex)
{
    if (ringBuffer->isPowerOfTwo != 0U) {
        return logicalIndex & ringBuffer->mask;
    }

    return logicalIndex % ringBuffer->capacity;
}

static uint32_t commPacketParserPeekOffset(const stRingBuffer *ringBuffer, uint32_t offset, uint8_t *buffer, uint32_t length)
{
    uint32_t lUsedLength;
    uint32_t lReadLength;
    uint32_t lLogicalIndex;
    uint32_t lPhysicalIndex;
    uint32_t lFirstLength;

    if ((ringBuffer == NULL) || ((buffer == NULL) && (length != 0U))) {
        return 0U;
    }

    lUsedLength = ringBufferGetUsed(ringBuffer);
    if (offset >= lUsedLength) {
        return 0U;
    }

    lReadLength = commPacketParserMinU32(length, lUsedLength - offset);
    if (lReadLength == 0U) {
        return 0U;
    }

    lLogicalIndex = ringBuffer->tail + offset;
    lPhysicalIndex = commPacketParserToPhysicalIndex(ringBuffer, lLogicalIndex);
    lFirstLength = commPacketParserMinU32(lReadLength, ringBuffer->capacity - lPhysicalIndex);
    (void)memcpy(buffer, &ringBuffer->buffer[lPhysicalIndex], lFirstLength);

    if (lReadLength > lFirstLength) {
        (void)memcpy(&buffer[lFirstLength], ringBuffer->buffer, lReadLength - lFirstLength);
    }

    return lReadLength;
}

static bool commPacketParserPeekByteAt(const stRingBuffer *ringBuffer, uint32_t offset, uint8_t *value)
{
    if ((ringBuffer == NULL) || (value == NULL)) {
        return false;
    }

    return commPacketParserPeekOffset(ringBuffer, offset, value, 1U) == 1U;
}

static bool commPacketParserIsHeaderMatchAt(const stCommPacketParser *parser, uint32_t offset)
{
    uint32_t lIndex;
    uint8_t lValue;

    for (lIndex = 0U; lIndex < parser->config.headerPatternLength; lIndex++) {
        if ((!commPacketParserPeekByteAt(parser->ringBuffer, offset + lIndex, &lValue)) ||
            (lValue != parser->config.headerPattern[lIndex])) {
            return false;
        }
    }

    return true;
}

static uint32_t commPacketParserFindPartialHeaderLength(const stCommPacketParser *parser, uint32_t usedLength)
{
    uint32_t lCandidateLength;
    uint32_t lCompareIndex;
    uint8_t lValue;

    if ((parser->config.headerPatternLength <= 1U) || (usedLength == 0U)) {
        return 0U;
    }

    for (lCandidateLength = commPacketParserMinU32(usedLength, parser->config.headerPatternLength - 1U);
         lCandidateLength > 0U;
         lCandidateLength--) {
        for (lCompareIndex = 0U; lCompareIndex < lCandidateLength; lCompareIndex++) {
            if ((!commPacketParserPeekByteAt(parser->ringBuffer,
                                             usedLength - lCandidateLength + lCompareIndex,
                                             &lValue)) ||
                (lValue != parser->config.headerPattern[lCompareIndex])) {
                break;
            }
        }

        if (lCompareIndex == lCandidateLength) {
            return lCandidateLength;
        }
    }

    return 0U;
}

static stCommPacketParserHeaderSearchResult commPacketParserSearchHeader(const stCommPacketParser *parser, uint32_t usedLength)
{
    stCommPacketParserHeaderSearchResult lResult = {0U, 0U, false};
    uint32_t lOffset;

    if (usedLength < parser->config.headerPatternLength) {
        lResult.partialHeaderLength = commPacketParserFindPartialHeaderLength(parser, usedLength);
        lResult.discardLength = usedLength - lResult.partialHeaderLength;
        return lResult;
    }

    for (lOffset = 0U; lOffset <= (usedLength - parser->config.headerPatternLength); lOffset++) {
        if (commPacketParserIsHeaderMatchAt(parser, lOffset)) {
            lResult.isFound = true;
            lResult.discardLength = lOffset;
            return lResult;
        }
    }

    lResult.partialHeaderLength = commPacketParserFindPartialHeaderLength(parser, usedLength);
    lResult.discardLength = usedLength - lResult.partialHeaderLength;
    return lResult;
}

static void commPacketParserClearPendingState(stCommPacketParser *parser)
{
    parser->pendingPacketTick = 0U;
    parser->pendingPacketLength = 0U;
    parser->hasPendingPacket = false;
}

static uint32_t commPacketParserGetTickMs(const stCommPacketParser *parser)
{
    if ((parser == NULL) || (parser->config.getTick == NULL)) {
        return 0U;
    }

    return parser->config.getTick();
}

static bool commPacketParserIsPacketTimedOut(const stCommPacketParser *parser)
{
    uint32_t lNowTick;

    if ((parser == NULL) || (parser->hasPendingPacket == false) || (parser->config.waitPacketTimeoutMs == 0U)) {
        return false;
    }

    if (parser->config.getTick == NULL) {
        return false;
    }

    lNowTick = commPacketParserGetTickMs(parser);
    return (uint32_t)(lNowTick - parser->pendingPacketTick) >= parser->config.waitPacketTimeoutMs;
}

static bool commPacketParserResolveOffset(uint32_t packetLength, int32_t offset, uint32_t *resolvedOffset)
{
    uint32_t lDistanceFromEnd;

    if (resolvedOffset == NULL) {
        return false;
    }

    if (offset >= 0) {
        *resolvedOffset = (uint32_t)offset;
        return *resolvedOffset < packetLength;
    }

    lDistanceFromEnd = (uint32_t)(-offset);
    if ((lDistanceFromEnd == 0U) || (lDistanceFromEnd > packetLength)) {
        return false;
    }

    *resolvedOffset = packetLength - lDistanceFromEnd;
    return true;
}

static bool commPacketParserGetPacketCrcValue(const stCommPacketParserConfig *config, const uint8_t *packetBuffer, uint32_t packetLength, uint32_t *packetCrcValue)
{
    uint32_t lFieldOffset;
    uint32_t lIndex;
    uint32_t lValue = 0U;

    if ((config == NULL) || (packetBuffer == NULL) || (packetCrcValue == NULL)) {
        return false;
    }

    if ((!commPacketParserResolveOffset(packetLength, config->crcFieldOffset, &lFieldOffset)) ||
        (config->crcFieldLength == 0U) ||
        (config->crcFieldLength > sizeof(uint32_t)) ||
        ((lFieldOffset + config->crcFieldLength) > packetLength)) {
        return false;
    }

    if (config->crcFieldEndian == COMM_PACKET_PARSER_CRC_ENDIAN_BIG) {
        for (lIndex = 0U; lIndex < config->crcFieldLength; lIndex++) {
            lValue = (lValue << 8U) | packetBuffer[lFieldOffset + lIndex];
        }
    } else {
        for (lIndex = 0U; lIndex < config->crcFieldLength; lIndex++) {
            lValue |= ((uint32_t)packetBuffer[lFieldOffset + lIndex]) << (8U * lIndex);
        }
    }

    *packetCrcValue = lValue;
    return true;
}

static bool commPacketParserCheckPacketCrc(const stCommPacketParserConfig *config, const uint8_t *packetBuffer, uint32_t packetLength)
{
    uint32_t lRangeStart;
    uint32_t lRangeEnd;
    uint32_t lPacketCrcValue;
    uint32_t lCalculatedCrcValue;

    if ((config == NULL) || (packetBuffer == NULL) || (config->crcCalculateFunc == NULL)) {
        return false;
    }

    if ((!commPacketParserResolveOffset(packetLength, config->crcRangeStartOffset, &lRangeStart)) ||
        (!commPacketParserResolveOffset(packetLength, config->crcRangeEndOffset, &lRangeEnd)) ||
        (lRangeEnd < lRangeStart)) {
        return false;
    }

    if (!commPacketParserGetPacketCrcValue(config, packetBuffer, packetLength, &lPacketCrcValue)) {
        return false;
    }

    lCalculatedCrcValue = config->crcCalculateFunc(&packetBuffer[lRangeStart],
                                                   (lRangeEnd - lRangeStart) + 1U,
                                                   config->userContext);
    return lCalculatedCrcValue == lPacketCrcValue;
}

bool commPacketParserIsConfigValid(const stCommPacketParserConfig *config)
{
    if ((config == NULL) ||
        (config->headerPattern == NULL) ||
        (config->headerPatternLength == 0U) ||
        (config->minHeaderLength < config->headerPatternLength) ||
        (config->minPacketLength < config->minHeaderLength) ||
        (config->maxPacketLength < config->minPacketLength) ||
        (config->outputBuffer == NULL) ||
        (config->outputBufferSize < config->minHeaderLength) ||
        (config->packetLengthFunc == NULL) ||
        (config->crcCalculateFunc == NULL) ||
        (config->crcFieldLength == 0U) ||
        (config->crcFieldLength > sizeof(uint32_t))) {
        return false;
    }

    return true;
}

eCommPacketParserStatus commPacketParserInit(stCommPacketParser *parser, stRingBuffer *ringBuffer, const stCommPacketParserConfig *config)
{
    if ((parser == NULL) || (ringBuffer == NULL) || (!commPacketParserIsConfigValid(config))) {
        return COMM_PACKET_PARSER_INVALID_ARGUMENT;
    }

    (void)memset(parser, 0, sizeof(*parser));
    parser->ringBuffer = ringBuffer;
    parser->config = *config;
    parser->isInitialized = true;
    return COMM_PACKET_PARSER_OK;
}

void commPacketParserReset(stCommPacketParser *parser)
{
    if (parser == NULL) {
        return;
    }

    parser->packet.buffer = NULL;
    parser->packet.length = 0U;
    parser->hasReadyPacket = false;
    commPacketParserClearPendingState(parser);
}

eCommPacketParserStatus commPacketParserProcess(stCommPacketParser *parser, stCommPacketParserPacket *packet)
{
    eCommPacketParserStatus lLastStatus = COMM_PACKET_PARSER_OK;
    uint32_t lUsedLength;
    stCommPacketParserHeaderSearchResult lHeaderSearch;
    uint32_t lHeaderLength;
    uint32_t lPacketLength;
    uint32_t lPeekLength;

    if ((parser == NULL) || (!parser->isInitialized) || (!commPacketParserIsConfigValid(&parser->config))) {
        return COMM_PACKET_PARSER_INVALID_ARGUMENT;
    }

    if (parser->hasReadyPacket) {
        if (packet != NULL) {
            *packet = parser->packet;
        }
        return COMM_PACKET_PARSER_OK;
    }

    while (true) {
        lUsedLength = ringBufferGetUsed(parser->ringBuffer);
        if (lUsedLength == 0U) {
            commPacketParserClearPendingState(parser);
            return (lLastStatus != COMM_PACKET_PARSER_OK) ? lLastStatus : COMM_PACKET_PARSER_EMPTY;
        }

        lHeaderSearch = commPacketParserSearchHeader(parser, lUsedLength);
        if (!lHeaderSearch.isFound) {
            if (lHeaderSearch.discardLength > 0U) {
                (void)ringBufferDiscard(parser->ringBuffer, lHeaderSearch.discardLength);
                commPacketParserClearPendingState(parser);
            }

            if (lHeaderSearch.partialHeaderLength > 0U) {
                return (lLastStatus != COMM_PACKET_PARSER_OK) ? lLastStatus : COMM_PACKET_PARSER_NEED_MORE_DATA;
            }

            return (lLastStatus != COMM_PACKET_PARSER_OK) ? lLastStatus : COMM_PACKET_PARSER_HEADER_NOT_FOUND;
        }

        if (lHeaderSearch.discardLength > 0U) {
            (void)ringBufferDiscard(parser->ringBuffer, lHeaderSearch.discardLength);
            commPacketParserClearPendingState(parser);
        }

        lUsedLength = ringBufferGetUsed(parser->ringBuffer);
        if (lUsedLength < parser->config.headerPatternLength) {
            return (lLastStatus != COMM_PACKET_PARSER_OK) ? lLastStatus : COMM_PACKET_PARSER_NEED_MORE_DATA;
        }

        if (lUsedLength < parser->config.minHeaderLength) {
            if (!parser->hasPendingPacket) {
                parser->hasPendingPacket = true;
                parser->pendingPacketTick = commPacketParserGetTickMs(parser);
                parser->pendingPacketLength = 0U;
            }

            if (commPacketParserIsPacketTimedOut(parser)) {
                (void)ringBufferDiscard(parser->ringBuffer, parser->config.headerPatternLength);
                commPacketParserClearPendingState(parser);
                lLastStatus = COMM_PACKET_PARSER_NEED_MORE_DATA;
                continue;
            }

            return (lLastStatus != COMM_PACKET_PARSER_OK) ? lLastStatus : COMM_PACKET_PARSER_NEED_MORE_DATA;
        }

        lPeekLength = commPacketParserMinU32(lUsedLength, parser->config.maxPacketLength);
        lPeekLength = commPacketParserMinU32(lPeekLength, parser->config.outputBufferSize);
        if (commPacketParserPeekOffset(parser->ringBuffer, 0U, parser->config.outputBuffer, lPeekLength) != lPeekLength) {
            return COMM_PACKET_PARSER_INVALID_ARGUMENT;
        }

        if (parser->config.headerLengthFunc != NULL) {
            lHeaderLength = parser->config.headerLengthFunc(parser->config.outputBuffer,
                                                            lPeekLength,
                                                            parser->config.userContext);
        } else {
            lHeaderLength = parser->config.headerPatternLength;
        }

        if ((lHeaderLength < parser->config.minHeaderLength) ||
            (lHeaderLength > parser->config.maxPacketLength)) {
            (void)ringBufferDiscard(parser->ringBuffer, 1U);
            commPacketParserClearPendingState(parser);
            lLastStatus = COMM_PACKET_PARSER_HEADER_INVALID;
            continue;
        }

        lPacketLength = parser->config.packetLengthFunc(parser->config.outputBuffer,
                                                        lHeaderLength,
                                                        lPeekLength,
                                                        parser->config.userContext);
        if ((lPacketLength < lHeaderLength) ||
            (lPacketLength < parser->config.minPacketLength) ||
            (lPacketLength > parser->config.maxPacketLength)) {
            (void)ringBufferDiscard(parser->ringBuffer, 1U);
            commPacketParserClearPendingState(parser);
            lLastStatus = COMM_PACKET_PARSER_LENGTH_INVALID;
            continue;
        }

        if (parser->config.outputBufferSize < lPacketLength) {
            return COMM_PACKET_PARSER_OUTPUT_BUFFER_TOO_SMALL;
        }

        if (lUsedLength < lPacketLength) {
            if ((!parser->hasPendingPacket) || (parser->pendingPacketLength != lPacketLength)) {
                parser->hasPendingPacket = true;
                parser->pendingPacketTick = commPacketParserGetTickMs(parser);
                parser->pendingPacketLength = (uint16_t)lPacketLength;
            }

            if (commPacketParserIsPacketTimedOut(parser)) {
                (void)ringBufferDiscard(parser->ringBuffer, lHeaderLength);
                commPacketParserClearPendingState(parser);
                lLastStatus = COMM_PACKET_PARSER_NEED_MORE_DATA;
                continue;
            }

            return (lLastStatus != COMM_PACKET_PARSER_OK) ? lLastStatus : COMM_PACKET_PARSER_NEED_MORE_DATA;
        }

        if (commPacketParserPeekOffset(parser->ringBuffer, 0U, parser->config.outputBuffer, lPacketLength) != lPacketLength) {
            return COMM_PACKET_PARSER_INVALID_ARGUMENT;
        }

        if (!commPacketParserCheckPacketCrc(&parser->config, parser->config.outputBuffer, lPacketLength)) {
            (void)ringBufferDiscard(parser->ringBuffer, 1U);
            commPacketParserClearPendingState(parser);
            lLastStatus = COMM_PACKET_PARSER_CRC_FAILED;
            continue;
        }

        (void)ringBufferDiscard(parser->ringBuffer, lPacketLength);
        commPacketParserClearPendingState(parser);
        parser->packet.buffer = parser->config.outputBuffer;
        parser->packet.length = (uint16_t)lPacketLength;
        parser->hasReadyPacket = true;

        if (packet != NULL) {
            *packet = parser->packet;
        }

        return COMM_PACKET_PARSER_OK;
    }
}

bool commPacketParserHasReadyPacket(const stCommPacketParser *parser)
{
    return (parser != NULL) && parser->hasReadyPacket;
}

const stCommPacketParserPacket *commPacketParserGetReadyPacket(const stCommPacketParser *parser)
{
    if ((parser == NULL) || (!parser->hasReadyPacket)) {
        return NULL;
    }

    return &parser->packet;
}

void commPacketParserReleasePacket(stCommPacketParser *parser)
{
    if (parser == NULL) {
        return;
    }

    parser->packet.buffer = NULL;
    parser->packet.length = 0U;
    parser->hasReadyPacket = false;
}
/**************************End of file********************************/

