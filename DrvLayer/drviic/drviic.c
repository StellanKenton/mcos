/***********************************************************************************
* @file     : drviic.c
* @brief    : Reusable hardware IIC driver abstraction implementation.
* @details  : This module validates public parameters, serializes bus access,
*             and forwards transactions to the project-specific BSP hook table.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drviic.h"

#include <stdbool.h>
#include <stddef.h>

#include "rep_config.h"

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#elif (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
#include "gd32f4xx.h"
#endif

static bool gDrvIicInitialized[DRVIIC_MAX];

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
static SemaphoreHandle_t gDrvIicMutex[DRVIIC_MAX];
#else
static volatile bool gDrvIicBusBusy[DRVIIC_MAX];
#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
static uint32_t gDrvIicCriticalState = 0U;
static uint32_t gDrvIicCriticalDepth = 0U;
#endif
#endif

extern stDrvIicBspInterface gDrvIicBspInterface[DRVIIC_MAX];

#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
static void drvIicEnterCritical(void)
{
    uint32_t lPrimask = __get_PRIMASK();

    __set_PRIMASK(1U);
    if (gDrvIicCriticalDepth == 0U) {
        gDrvIicCriticalState = lPrimask;
    }
    gDrvIicCriticalDepth++;
}

static void drvIicExitCritical(void)
{
    if (gDrvIicCriticalDepth > 0U) {
        gDrvIicCriticalDepth--;
        if (gDrvIicCriticalDepth == 0U) {
            __set_PRIMASK(gDrvIicCriticalState);
        }
    }
}
#endif

static bool drvIicIsValid(eDrvIicPortMap iic)
{
    return iic < DRVIIC_MAX;
}

static bool drvIicIsValidAddress(uint8_t address)
{
    return address < 0x80U;
}

static bool drvIicIsValidConstBuffer(const uint8_t *buffer, uint16_t length)
{
    return ((buffer != NULL) || (length == 0U));
}

static bool drvIicIsValidMutableBuffer(uint8_t *buffer, uint16_t length)
{
    return ((buffer != NULL) || (length == 0U));
}

static bool drvIicHasWritePhase(const stDrvIicTransfer *transfer)
{
    return (transfer != NULL) &&
           ((transfer->writeLength > 0U) || (transfer->secondWriteLength > 0U));
}

static bool drvIicHasReadPhase(const stDrvIicTransfer *transfer)
{
    return (transfer != NULL) && (transfer->readLength > 0U);
}

static bool drvIicIsValidTransfer(const stDrvIicTransfer *transfer)
{
    if (transfer == NULL) {
        return false;
    }

    if (!drvIicIsValidAddress(transfer->address)) {
        return false;
    }

    if (!drvIicIsValidConstBuffer(transfer->writeBuffer, transfer->writeLength)) {
        return false;
    }

    if (!drvIicIsValidConstBuffer(transfer->secondWriteBuffer, transfer->secondWriteLength)) {
        return false;
    }

    if (!drvIicIsValidMutableBuffer(transfer->readBuffer, transfer->readLength)) {
        return false;
    }

    return drvIicHasWritePhase(transfer) || drvIicHasReadPhase(transfer);
}

static bool drvIicIsInitialized(eDrvIicPortMap iic)
{
    return drvIicIsValid(iic) && gDrvIicInitialized[iic];
}

static stDrvIicBspInterface *drvIicGetBspInterface(eDrvIicPortMap iic)
{
    if (!drvIicIsValid(iic)) {
        return NULL;
    }

    return &gDrvIicBspInterface[iic];
}

static bool drvIicHasValidBspInterface(eDrvIicPortMap iic)
{
    stDrvIicBspInterface *lBspInterface = drvIicGetBspInterface(iic);

    return (lBspInterface != NULL) &&
           (lBspInterface->init != NULL) &&
           (lBspInterface->transfer != NULL);
}

static uint32_t drvIicGetTimeoutMs(const stDrvIicBspInterface *bspInterface, uint32_t timeoutMs)
{
    if (timeoutMs > 0U) {
        return timeoutMs;
    }

    if ((bspInterface != NULL) && (bspInterface->defaultTimeoutMs > 0U)) {
        return bspInterface->defaultTimeoutMs;
    }

    return DRVIIC_DEFAULT_TIMEOUT_MS;
}

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
static eDrvIicStatus drvIicEnsureMutex(eDrvIicPortMap iic)
{
    if (!drvIicIsValid(iic)) {
        return DRVIIC_STATUS_INVALID_PARAM;
    }

    if (gDrvIicMutex[iic] == NULL) {
        gDrvIicMutex[iic] = xSemaphoreCreateMutex();
        if (gDrvIicMutex[iic] == NULL) {
            return DRVIIC_STATUS_ERROR;
        }
    }

    return DRVIIC_STATUS_OK;
}
#endif

static eDrvIicStatus drvIicLockBus(eDrvIicPortMap iic)
{
#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
    if (drvIicEnsureMutex(iic) != DRVIIC_STATUS_OK) {
        return DRVIIC_STATUS_ERROR;
    }

    if (xSemaphoreTake(gDrvIicMutex[iic], pdMS_TO_TICKS(DRVIIC_LOCK_WAIT_MS)) != pdTRUE) {
        return DRVIIC_STATUS_BUSY;
    }

    return DRVIIC_STATUS_OK;
#else
    if (!drvIicIsValid(iic)) {
        return DRVIIC_STATUS_INVALID_PARAM;
    }

#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    drvIicEnterCritical();
#endif

    if (gDrvIicBusBusy[iic]) {
#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
        drvIicExitCritical();
#endif
        return DRVIIC_STATUS_BUSY;
    }

    gDrvIicBusBusy[iic] = true;

#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    drvIicExitCritical();
#endif
    return DRVIIC_STATUS_OK;
#endif
}

static void drvIicUnlockBus(eDrvIicPortMap iic)
{
#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
    if (drvIicIsValid(iic) && (gDrvIicMutex[iic] != NULL)) {
        (void)xSemaphoreGive(gDrvIicMutex[iic]);
    }
#else
    if (!drvIicIsValid(iic)) {
        return;
    }

#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    drvIicEnterCritical();
#endif

    gDrvIicBusBusy[iic] = false;

#if (REP_RTOS_SYSTEM == REP_RTOS_NONE) && (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    drvIicExitCritical();
#endif
#endif
}

static eDrvIicStatus drvIicTransferLocked(eDrvIicPortMap iic,
                                          const stDrvIicTransfer *transfer,
                                          uint32_t timeoutMs)
{
    stDrvIicBspInterface *lBspInterface = drvIicGetBspInterface(iic);

    if ((lBspInterface == NULL) || (lBspInterface->transfer == NULL)) {
        return DRVIIC_STATUS_NOT_READY;
    }

    return lBspInterface->transfer(iic, transfer, drvIicGetTimeoutMs(lBspInterface, timeoutMs));
}

eDrvIicStatus drvIicInit(eDrvIicPortMap iic)
{
    stDrvIicBspInterface *lBspInterface = NULL;
    eDrvIicStatus lStatus;

    if (!drvIicIsValid(iic)) {
        return DRVIIC_STATUS_INVALID_PARAM;
    }

    if (drvIicIsInitialized(iic)) {
        return DRVIIC_STATUS_OK;
    }

    if (!drvIicHasValidBspInterface(iic)) {
        return DRVIIC_STATUS_NOT_READY;
    }

    lBspInterface = drvIicGetBspInterface(iic);
    if (lBspInterface == NULL) {
        return DRVIIC_STATUS_NOT_READY;
    }

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
    lStatus = drvIicEnsureMutex(iic);
    if (lStatus != DRVIIC_STATUS_OK) {
        return lStatus;
    }
#endif

    lStatus = lBspInterface->init(iic);
    if (lStatus != DRVIIC_STATUS_OK) {
        return lStatus;
    }

    gDrvIicInitialized[iic] = true;
    return DRVIIC_STATUS_OK;
}

eDrvIicStatus drvIicRecoverBus(eDrvIicPortMap iic)
{
    stDrvIicBspInterface *lBspInterface = NULL;
    eDrvIicStatus lStatus;

    if (!drvIicIsValid(iic)) {
        return DRVIIC_STATUS_INVALID_PARAM;
    }

    if (!drvIicIsInitialized(iic)) {
        return DRVIIC_STATUS_NOT_READY;
    }

    lBspInterface = drvIicGetBspInterface(iic);
    if ((lBspInterface == NULL) || (lBspInterface->recoverBus == NULL)) {
        return DRVIIC_STATUS_UNSUPPORTED;
    }

    lStatus = drvIicLockBus(iic);
    if (lStatus != DRVIIC_STATUS_OK) {
        return lStatus;
    }

    lStatus = lBspInterface->recoverBus(iic);
    drvIicUnlockBus(iic);
    return lStatus;
}

eDrvIicStatus drvIicTransferTimeout(eDrvIicPortMap iic,
                                    const stDrvIicTransfer *transfer,
                                    uint32_t timeoutMs)
{
    eDrvIicStatus lStatus;

    if (!drvIicIsValid(iic) || !drvIicIsValidTransfer(transfer)) {
        return DRVIIC_STATUS_INVALID_PARAM;
    }

    if (!drvIicIsInitialized(iic)) {
        return DRVIIC_STATUS_NOT_READY;
    }

    lStatus = drvIicLockBus(iic);
    if (lStatus != DRVIIC_STATUS_OK) {
        return lStatus;
    }

    lStatus = drvIicTransferLocked(iic, transfer, timeoutMs);
    drvIicUnlockBus(iic);
    return lStatus;
}

eDrvIicStatus drvIicTransfer(eDrvIicPortMap iic, const stDrvIicTransfer *transfer)
{
    return drvIicTransferTimeout(iic, transfer, 0U);
}

eDrvIicStatus drvIicWriteTimeout(eDrvIicPortMap iic,
                                 uint8_t address,
                                 const uint8_t *buffer,
                                 uint16_t length,
                                 uint32_t timeoutMs)
{
    stDrvIicTransfer lTransfer;

    lTransfer.address = address;
    lTransfer.writeBuffer = buffer;
    lTransfer.writeLength = length;
    lTransfer.readBuffer = NULL;
    lTransfer.readLength = 0U;
    lTransfer.secondWriteBuffer = NULL;
    lTransfer.secondWriteLength = 0U;

    return drvIicTransferTimeout(iic, &lTransfer, timeoutMs);
}

eDrvIicStatus drvIicWrite(eDrvIicPortMap iic,
                          uint8_t address,
                          const uint8_t *buffer,
                          uint16_t length)
{
    return drvIicWriteTimeout(iic, address, buffer, length, 0U);
}

eDrvIicStatus drvIicReadTimeout(eDrvIicPortMap iic,
                                uint8_t address,
                                uint8_t *buffer,
                                uint16_t length,
                                uint32_t timeoutMs)
{
    stDrvIicTransfer lTransfer;

    lTransfer.address = address;
    lTransfer.writeBuffer = NULL;
    lTransfer.writeLength = 0U;
    lTransfer.readBuffer = buffer;
    lTransfer.readLength = length;
    lTransfer.secondWriteBuffer = NULL;
    lTransfer.secondWriteLength = 0U;

    return drvIicTransferTimeout(iic, &lTransfer, timeoutMs);
}

eDrvIicStatus drvIicRead(eDrvIicPortMap iic,
                         uint8_t address,
                         uint8_t *buffer,
                         uint16_t length)
{
    return drvIicReadTimeout(iic, address, buffer, length, 0U);
}

eDrvIicStatus drvIicWriteRegisterTimeout(eDrvIicPortMap iic,
                                         uint8_t address,
                                         const uint8_t *registerBuffer,
                                         uint16_t registerLength,
                                         const uint8_t *buffer,
                                         uint16_t length,
                                         uint32_t timeoutMs)
{
    stDrvIicTransfer lTransfer;

    lTransfer.address = address;
    lTransfer.writeBuffer = registerBuffer;
    lTransfer.writeLength = registerLength;
    lTransfer.readBuffer = NULL;
    lTransfer.readLength = 0U;
    lTransfer.secondWriteBuffer = buffer;
    lTransfer.secondWriteLength = length;

    return drvIicTransferTimeout(iic, &lTransfer, timeoutMs);
}

eDrvIicStatus drvIicWriteRegister(eDrvIicPortMap iic,
                                  uint8_t address,
                                  const uint8_t *registerBuffer,
                                  uint16_t registerLength,
                                  const uint8_t *buffer,
                                  uint16_t length)
{
    return drvIicWriteRegisterTimeout(iic, address, registerBuffer, registerLength, buffer, length, 0U);
}

eDrvIicStatus drvIicReadRegisterTimeout(eDrvIicPortMap iic,
                                        uint8_t address,
                                        const uint8_t *registerBuffer,
                                        uint16_t registerLength,
                                        uint8_t *buffer,
                                        uint16_t length,
                                        uint32_t timeoutMs)
{
    stDrvIicTransfer lTransfer;

    lTransfer.address = address;
    lTransfer.writeBuffer = registerBuffer;
    lTransfer.writeLength = registerLength;
    lTransfer.readBuffer = buffer;
    lTransfer.readLength = length;
    lTransfer.secondWriteBuffer = NULL;
    lTransfer.secondWriteLength = 0U;

    return drvIicTransferTimeout(iic, &lTransfer, timeoutMs);
}

eDrvIicStatus drvIicReadRegister(eDrvIicPortMap iic,
                                 uint8_t address,
                                 const uint8_t *registerBuffer,
                                 uint16_t registerLength,
                                 uint8_t *buffer,
                                 uint16_t length)
{
    return drvIicReadRegisterTimeout(iic, address, registerBuffer, registerLength, buffer, length, 0U);
}

/**************************End of file********************************/
