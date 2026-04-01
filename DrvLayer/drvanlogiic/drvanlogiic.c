/***********************************************************************************
* @file     : drvanlogiic.c
* @brief    : Reusable software IIC driver abstraction implementation.
* @details  : This module implements stable bit-banged IIC transfers with bus
*             recovery, clock stretching support, and task-safe bus locking.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drvanlogiic.h"

#include <stdbool.h>
#include <stddef.h>

#include "rep_config.h"

#if (DRVANLOGIIC_LOG_SUPPORT == 1)
#include "../../console/log.h"
#endif

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#elif (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
#include "gd32f4xx.h"
#endif

#define DRVANLOGIIC_LOG_TAG                  "drvAnlogIic"

static bool gDrvAnlogIicInitialized[DRVANLOGIIC_MAX];

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
static SemaphoreHandle_t gDrvAnlogIicMutex[DRVANLOGIIC_MAX];
#else
static volatile bool gDrvAnlogIicBusBusy[DRVANLOGIIC_MAX];
#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
static uint32_t gDrvAnlogIicCriticalState = 0U;
static uint32_t gDrvAnlogIicCriticalDepth = 0U;
#endif
#endif

extern stDrvAnlogIicBspInterface gDrvAnlogIicBspInterface[DRVANLOGIIC_MAX];

#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
static void drvAnlogIicEnterCritical(void)
{
    uint32_t lPrimask = __get_PRIMASK();

    __set_PRIMASK(1U);
    if (gDrvAnlogIicCriticalDepth == 0U) {
        gDrvAnlogIicCriticalState = lPrimask;
    }
    gDrvAnlogIicCriticalDepth++;
}

static void drvAnlogIicExitCritical(void)
{
    if (gDrvAnlogIicCriticalDepth > 0U) {
        gDrvAnlogIicCriticalDepth--;
        if (gDrvAnlogIicCriticalDepth == 0U) {
            __set_PRIMASK(gDrvAnlogIicCriticalState);
        }
    }
}
#endif

static bool drvAnlogIicIsValid(eDrvAnlogIicPortMap iic)
{
    return iic < DRVANLOGIIC_MAX;
}

static bool drvAnlogIicIsValidAddress(uint8_t address)
{
    return address < 0x80U;
}

static bool drvAnlogIicIsValidWriteBuffer(const uint8_t *buffer, uint16_t length)
{
    return ((buffer != NULL) || (length == 0U));
}

static bool drvAnlogIicIsValidReadBuffer(uint8_t *buffer, uint16_t length)
{
    return ((buffer != NULL) || (length == 0U));
}

static stDrvAnlogIicBspInterface *drvAnlogIicGetBspInterface(eDrvAnlogIicPortMap iic)
{
    if (!drvAnlogIicIsValid(iic)) {
        return NULL;
    }

    return &gDrvAnlogIicBspInterface[iic];
}

static bool drvAnlogIicHasValidBspInterface(eDrvAnlogIicPortMap iic)
{
    stDrvAnlogIicBspInterface *lBspInterface = drvAnlogIicGetBspInterface(iic);

    return (lBspInterface != NULL) &&
           (lBspInterface->init != NULL) &&
           (lBspInterface->setScl != NULL) &&
           (lBspInterface->setSda != NULL) &&
           (lBspInterface->readScl != NULL) &&
           (lBspInterface->readSda != NULL) &&
           (lBspInterface->delayUs != NULL);
}

static uint16_t drvAnlogIicGetHalfPeriodUs(const stDrvAnlogIicBspInterface *bspInterface)
{
    if ((bspInterface == NULL) || (bspInterface->halfPeriodUs == 0U)) {
        return DRVANLOGIIC_DEFAULT_HALF_PERIOD_US;
    }

    return bspInterface->halfPeriodUs;
}

static uint8_t drvAnlogIicGetRecoveryClockCount(const stDrvAnlogIicBspInterface *bspInterface)
{
    if ((bspInterface == NULL) || (bspInterface->recoveryClockCount == 0U)) {
        return DRVANLOGIIC_DEFAULT_RECOVERY_CLOCKS;
    }

    return bspInterface->recoveryClockCount;
}

static void drvAnlogIicDelayHalfPeriod(const stDrvAnlogIicBspInterface *bspInterface)
{
    bspInterface->delayUs(drvAnlogIicGetHalfPeriodUs(bspInterface));
}

static eDrvAnlogIicStatus drvAnlogIicWaitSclHigh(eDrvAnlogIicPortMap iic,
                                                 stDrvAnlogIicBspInterface *bspInterface)
{
    if (bspInterface == NULL) {
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    bspInterface->setScl(iic, true);
    return DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicWriteBit(eDrvAnlogIicPortMap iic,
                                              stDrvAnlogIicBspInterface *bspInterface,
                                              bool isHigh)
{
    eDrvAnlogIicStatus lStatus;

    bspInterface->setSda(iic, isHigh);
    drvAnlogIicDelayHalfPeriod(bspInterface);

    lStatus = drvAnlogIicWaitSclHigh(iic, bspInterface);
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    drvAnlogIicDelayHalfPeriod(bspInterface);
    bspInterface->setScl(iic, false);
    drvAnlogIicDelayHalfPeriod(bspInterface);
    return DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicReadBit(eDrvAnlogIicPortMap iic,
                                             stDrvAnlogIicBspInterface *bspInterface,
                                             bool *bitValue)
{
    eDrvAnlogIicStatus lStatus;

    if (bitValue == NULL) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    bspInterface->setSda(iic, true);
    drvAnlogIicDelayHalfPeriod(bspInterface);

    lStatus = drvAnlogIicWaitSclHigh(iic, bspInterface);
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    drvAnlogIicDelayHalfPeriod(bspInterface);
    *bitValue = bspInterface->readSda(iic);
    bspInterface->setScl(iic, false);
    drvAnlogIicDelayHalfPeriod(bspInterface);
    return DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicSendStart(eDrvAnlogIicPortMap iic,
                                               stDrvAnlogIicBspInterface *bspInterface)
{
    eDrvAnlogIicStatus lStatus;

    bspInterface->setSda(iic, true);
    lStatus = drvAnlogIicWaitSclHigh(iic, bspInterface);
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    drvAnlogIicDelayHalfPeriod(bspInterface);
    bspInterface->setSda(iic, false);
    drvAnlogIicDelayHalfPeriod(bspInterface);
    bspInterface->setScl(iic, false);
    drvAnlogIicDelayHalfPeriod(bspInterface);
    return DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicSendStop(eDrvAnlogIicPortMap iic,
                                              stDrvAnlogIicBspInterface *bspInterface)
{
    eDrvAnlogIicStatus lStatus;

    bspInterface->setSda(iic, false);
    drvAnlogIicDelayHalfPeriod(bspInterface);

    lStatus = drvAnlogIicWaitSclHigh(iic, bspInterface);
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    drvAnlogIicDelayHalfPeriod(bspInterface);
    bspInterface->setSda(iic, true);
    drvAnlogIicDelayHalfPeriod(bspInterface);
    return DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicWriteByte(eDrvAnlogIicPortMap iic,
                                               stDrvAnlogIicBspInterface *bspInterface,
                                               uint8_t data)
{
    uint8_t lBitIndex;
    bool lAckLevel = true;
    eDrvAnlogIicStatus lStatus;

    for (lBitIndex = 0U; lBitIndex < 8U; ++lBitIndex) {
        lStatus = drvAnlogIicWriteBit(iic, bspInterface, ((data & 0x80U) != 0U));
        if (lStatus != DRVANLOGIIC_STATUS_OK) {
            return lStatus;
        }

        data <<= 1U;
    }

    lStatus = drvAnlogIicReadBit(iic, bspInterface, &lAckLevel);
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    return lAckLevel ? DRVANLOGIIC_STATUS_NACK : DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicReadByte(eDrvAnlogIicPortMap iic,
                                              stDrvAnlogIicBspInterface *bspInterface,
                                              bool acknowledge,
                                              uint8_t *data)
{
    uint8_t lBitIndex;
    bool lBitValue = false;
    uint8_t lData = 0U;
    eDrvAnlogIicStatus lStatus;

    if (data == NULL) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    for (lBitIndex = 0U; lBitIndex < 8U; ++lBitIndex) {
        lStatus = drvAnlogIicReadBit(iic, bspInterface, &lBitValue);
        if (lStatus != DRVANLOGIIC_STATUS_OK) {
            return lStatus;
        }

        lData <<= 1U;
        if (lBitValue) {
            lData |= 0x01U;
        }
    }

    *data = lData;
    return drvAnlogIicWriteBit(iic, bspInterface, !acknowledge);
}

static eDrvAnlogIicStatus drvAnlogIicWriteBufferLocked(eDrvAnlogIicPortMap iic,
                                                       stDrvAnlogIicBspInterface *bspInterface,
                                                       uint8_t address,
                                                       const uint8_t *buffer,
                                                       uint16_t length)
{
    uint16_t lIndex;
    eDrvAnlogIicStatus lStatus;

    lStatus = drvAnlogIicWriteByte(iic, bspInterface, (uint8_t)(address << 1U));
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    for (lIndex = 0U; lIndex < length; ++lIndex) {
        lStatus = drvAnlogIicWriteByte(iic, bspInterface, buffer[lIndex]);
        if (lStatus != DRVANLOGIIC_STATUS_OK) {
            return lStatus;
        }
    }

    return DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicWriteRawLocked(eDrvAnlogIicPortMap iic,
                                                    stDrvAnlogIicBspInterface *bspInterface,
                                                    const uint8_t *buffer,
                                                    uint16_t length)
{
    uint16_t lIndex;
    eDrvAnlogIicStatus lStatus;

    for (lIndex = 0U; lIndex < length; ++lIndex) {
        lStatus = drvAnlogIicWriteByte(iic, bspInterface, buffer[lIndex]);
        if (lStatus != DRVANLOGIIC_STATUS_OK) {
            return lStatus;
        }
    }

    return DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicReadBufferLocked(eDrvAnlogIicPortMap iic,
                                                      stDrvAnlogIicBspInterface *bspInterface,
                                                      uint8_t address,
                                                      uint8_t *buffer,
                                                      uint16_t length)
{
    uint16_t lIndex;
    eDrvAnlogIicStatus lStatus;

    lStatus = drvAnlogIicWriteByte(iic, bspInterface, (uint8_t)((address << 1U) | 0x01U));
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    for (lIndex = 0U; lIndex < length; ++lIndex) {
        lStatus = drvAnlogIicReadByte(iic,
                                      bspInterface,
                                      (lIndex + 1U) < length,
                                      &buffer[lIndex]);
        if (lStatus != DRVANLOGIIC_STATUS_OK) {
            return lStatus;
        }
    }

    return DRVANLOGIIC_STATUS_OK;
}

static eDrvAnlogIicStatus drvAnlogIicRecoverBusLocked(eDrvAnlogIicPortMap iic,
                                                      stDrvAnlogIicBspInterface *bspInterface)
{
    uint8_t lClockIndex;
    uint8_t lRecoveryClockCount;
    eDrvAnlogIicStatus lStatus;

    if (bspInterface == NULL) {
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    lRecoveryClockCount = drvAnlogIicGetRecoveryClockCount(bspInterface);

    bspInterface->setSda(iic, true);
    bspInterface->setScl(iic, true);
    drvAnlogIicDelayHalfPeriod(bspInterface);

    if (bspInterface->readSda(iic) && bspInterface->readScl(iic)) {
        return DRVANLOGIIC_STATUS_OK;
    }

    for (lClockIndex = 0U; lClockIndex < lRecoveryClockCount; ++lClockIndex) {
        bspInterface->setScl(iic, false);
        drvAnlogIicDelayHalfPeriod(bspInterface);

        lStatus = drvAnlogIicWaitSclHigh(iic, bspInterface);
        if (lStatus != DRVANLOGIIC_STATUS_OK) {
            return lStatus;
        }

        drvAnlogIicDelayHalfPeriod(bspInterface);
        if (bspInterface->readSda(iic)) {
            break;
        }
    }

    return drvAnlogIicSendStop(iic, bspInterface);
}

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
static bool drvAnlogIicInitMutex(eDrvAnlogIicPortMap iic)
{
    if (gDrvAnlogIicMutex[iic] == NULL) {
        gDrvAnlogIicMutex[iic] = xSemaphoreCreateMutex();
        if (gDrvAnlogIicMutex[iic] == NULL) {
            return false;
        }
    }

    return true;
}

static bool drvAnlogIicLockBus(eDrvAnlogIicPortMap iic)
{
    TickType_t lWaitTicks = 0U;

    if (!drvAnlogIicIsValid(iic)) {
        return false;
    }

    if (!drvAnlogIicInitMutex(iic)) {
        return false;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        lWaitTicks = pdMS_TO_TICKS(DRVANLOGIIC_LOCK_WAIT_MS);
    }

    return (xSemaphoreTake(gDrvAnlogIicMutex[iic], lWaitTicks) == pdTRUE);
}

static void drvAnlogIicUnlockBus(eDrvAnlogIicPortMap iic)
{
    if (drvAnlogIicIsValid(iic) && (gDrvAnlogIicMutex[iic] != NULL)) {
        (void)xSemaphoreGive(gDrvAnlogIicMutex[iic]);
    }
}
#else
static bool drvAnlogIicLockBus(eDrvAnlogIicPortMap iic)
{
#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    drvAnlogIicEnterCritical();
#endif

    if (!drvAnlogIicIsValid(iic)) {
#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
        drvAnlogIicExitCritical();
#endif
        return false;
    }

    if (gDrvAnlogIicBusBusy[iic]) {
#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
        drvAnlogIicExitCritical();
#endif
        return false;
    }

    gDrvAnlogIicBusBusy[iic] = true;

#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    drvAnlogIicExitCritical();
#endif
    return true;
}

static void drvAnlogIicUnlockBus(eDrvAnlogIicPortMap iic)
{
#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    drvAnlogIicEnterCritical();
#endif

    if (drvAnlogIicIsValid(iic)) {
        gDrvAnlogIicBusBusy[iic] = false;
    }

#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    drvAnlogIicExitCritical();
#endif
}
#endif

eDrvAnlogIicStatus drvAnlogIicInit(eDrvAnlogIicPortMap iic)
{
    stDrvAnlogIicBspInterface *lBspInterface;
    eDrvAnlogIicStatus lStatus;

    if (!drvAnlogIicIsValid(iic)) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    if (!drvAnlogIicHasValidBspInterface(iic)) {
        #if (DRVANLOGIIC_LOG_SUPPORT == 1)
        LOG_E(DRVANLOGIIC_LOG_TAG, "Invalid BSP interface for bus %u", (unsigned int)iic);
        #endif
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    lBspInterface = drvAnlogIicGetBspInterface(iic);
    if (lBspInterface == NULL) {
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    lBspInterface->init(iic);

    if (!drvAnlogIicLockBus(iic)) {
        return DRVANLOGIIC_STATUS_BUSY;
    }

    lStatus = drvAnlogIicRecoverBusLocked(iic, lBspInterface);
    drvAnlogIicUnlockBus(iic);
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    gDrvAnlogIicInitialized[iic] = true;
    return DRVANLOGIIC_STATUS_OK;
}

eDrvAnlogIicStatus drvAnlogIicRecoverBus(eDrvAnlogIicPortMap iic)
{
    stDrvAnlogIicBspInterface *lBspInterface;
    eDrvAnlogIicStatus lStatus;

    if (!drvAnlogIicIsValid(iic)) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    if (!gDrvAnlogIicInitialized[iic]) {
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    if (!drvAnlogIicLockBus(iic)) {
        return DRVANLOGIIC_STATUS_BUSY;
    }

    lBspInterface = drvAnlogIicGetBspInterface(iic);
    lStatus = drvAnlogIicRecoverBusLocked(iic, lBspInterface);
    drvAnlogIicUnlockBus(iic);
    return lStatus;
}

eDrvAnlogIicStatus drvAnlogIicTransfer(eDrvAnlogIicPortMap iic, const stDrvAnlogIicTransfer *transfer)
{
    stDrvAnlogIicBspInterface *lBspInterface;
    eDrvAnlogIicStatus lStatus;
    eDrvAnlogIicStatus lStopStatus;

    if (!drvAnlogIicIsValid(iic) || (transfer == NULL)) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    if (!drvAnlogIicIsValidAddress(transfer->address) ||
        !drvAnlogIicIsValidWriteBuffer(transfer->writeBuffer, transfer->writeLength) ||
        !drvAnlogIicIsValidReadBuffer(transfer->readBuffer, transfer->readLength) ||
        ((transfer->writeLength == 0U) && (transfer->readLength == 0U))) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    if (!gDrvAnlogIicInitialized[iic]) {
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    if (!drvAnlogIicLockBus(iic)) {
        return DRVANLOGIIC_STATUS_BUSY;
    }

    lBspInterface = drvAnlogIicGetBspInterface(iic);
    if (lBspInterface == NULL) {
        drvAnlogIicUnlockBus(iic);
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    lStatus = drvAnlogIicRecoverBusLocked(iic, lBspInterface);
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        drvAnlogIicUnlockBus(iic);
        return lStatus;
    }

    lStatus = drvAnlogIicSendStart(iic, lBspInterface);
    if (lStatus == DRVANLOGIIC_STATUS_OK) {
        if (transfer->writeLength > 0U) {
            lStatus = drvAnlogIicWriteBufferLocked(iic,
                                                   lBspInterface,
                                                   transfer->address,
                                                   transfer->writeBuffer,
                                                   transfer->writeLength);
        }

        if ((lStatus == DRVANLOGIIC_STATUS_OK) && (transfer->readLength > 0U)) {
            if (transfer->writeLength > 0U) {
                lStatus = drvAnlogIicSendStart(iic, lBspInterface);
            }

            if (lStatus == DRVANLOGIIC_STATUS_OK) {
                lStatus = drvAnlogIicReadBufferLocked(iic,
                                                      lBspInterface,
                                                      transfer->address,
                                                      transfer->readBuffer,
                                                      transfer->readLength);
            }
        }
    }

    lStopStatus = drvAnlogIicSendStop(iic, lBspInterface);
    drvAnlogIicUnlockBus(iic);

    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    return lStopStatus;
}

eDrvAnlogIicStatus drvAnlogIicWrite(eDrvAnlogIicPortMap iic,
                                    uint8_t address,
                                    const uint8_t *buffer,
                                    uint16_t length)
{
    stDrvAnlogIicTransfer lTransfer = {
        .address = address,
        .writeBuffer = buffer,
        .writeLength = length,
        .readBuffer = NULL,
        .readLength = 0U,
    };

    return drvAnlogIicTransfer(iic, &lTransfer);
}

eDrvAnlogIicStatus drvAnlogIicRead(eDrvAnlogIicPortMap iic,
                                   uint8_t address,
                                   uint8_t *buffer,
                                   uint16_t length)
{
    stDrvAnlogIicTransfer lTransfer = {
        .address = address,
        .writeBuffer = NULL,
        .writeLength = 0U,
        .readBuffer = buffer,
        .readLength = length,
    };

    return drvAnlogIicTransfer(iic, &lTransfer);
}

eDrvAnlogIicStatus drvAnlogIicWriteRegister(eDrvAnlogIicPortMap iic,
                                            uint8_t address,
                                            const uint8_t *registerBuffer,
                                            uint16_t registerLength,
                                            const uint8_t *buffer,
                                            uint16_t length)
{
    stDrvAnlogIicBspInterface *lBspInterface;
    eDrvAnlogIicStatus lStatus;
    eDrvAnlogIicStatus lStopStatus;

    if (!drvAnlogIicIsValid(iic)) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    if (!drvAnlogIicIsValidWriteBuffer(registerBuffer, registerLength) ||
        ((registerLength == 0U) && (length == 0U))) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    if (!drvAnlogIicIsValidAddress(address) || !drvAnlogIicIsValidWriteBuffer(buffer, length)) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    if (!gDrvAnlogIicInitialized[iic]) {
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    if (!drvAnlogIicLockBus(iic)) {
        return DRVANLOGIIC_STATUS_BUSY;
    }

    lBspInterface = drvAnlogIicGetBspInterface(iic);
    if (lBspInterface == NULL) {
        drvAnlogIicUnlockBus(iic);
        return DRVANLOGIIC_STATUS_NOT_READY;
    }

    lStatus = drvAnlogIicRecoverBusLocked(iic, lBspInterface);
    if (lStatus == DRVANLOGIIC_STATUS_OK) {
        lStatus = drvAnlogIicSendStart(iic, lBspInterface);
    }

    if (lStatus == DRVANLOGIIC_STATUS_OK) {
        lStatus = drvAnlogIicWriteByte(iic, lBspInterface, (uint8_t)(address << 1U));
    }

    if ((lStatus == DRVANLOGIIC_STATUS_OK) && (registerLength > 0U)) {
        lStatus = drvAnlogIicWriteRawLocked(iic, lBspInterface, registerBuffer, registerLength);
    }

    if ((lStatus == DRVANLOGIIC_STATUS_OK) && (length > 0U)) {
        lStatus = drvAnlogIicWriteRawLocked(iic, lBspInterface, buffer, length);
    }

    lStopStatus = drvAnlogIicSendStop(iic, lBspInterface);
    drvAnlogIicUnlockBus(iic);

    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    return lStopStatus;
}

eDrvAnlogIicStatus drvAnlogIicReadRegister(eDrvAnlogIicPortMap iic,
                                           uint8_t address,
                                           const uint8_t *registerBuffer,
                                           uint16_t registerLength,
                                           uint8_t *buffer,
                                           uint16_t length)
{
    stDrvAnlogIicTransfer lTransfer;

    if (!drvAnlogIicIsValidWriteBuffer(registerBuffer, registerLength) ||
        (registerLength == 0U) ||
        (length == 0U)) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    lTransfer.address = address;
    lTransfer.writeBuffer = registerBuffer;
    lTransfer.writeLength = registerLength;
    lTransfer.readBuffer = buffer;
    lTransfer.readLength = length;
    return drvAnlogIicTransfer(iic, &lTransfer);
}

/**************************End of file********************************/
