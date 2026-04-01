/***********************************************************************************
* @file     : mpu6050_port.c
* @brief    : MPU6050 project port-layer implementation.
* @details  : This file binds each MPU6050 device instance to either the
*             hardware or software IIC drv implementation at runtime.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
**********************************************************************************/
#include "mpu6050_port.h"

#include <stdbool.h>

#include "rep_config.h"

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
#include "gd32f4xx.h"
#endif

static bool gMpu6050PortCycleCntReady = false;

static void mpu6050PortEnableCycleCnt(void);
static eMpu6050DrvIicStatus mpu6050PortSoftIicInitAdpt(uint8_t bus);
static eMpu6050DrvIicStatus mpu6050PortSoftIicWriteRegAdpt(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, const uint8_t *buffer, uint16_t length);
static eMpu6050DrvIicStatus mpu6050PortSoftIicReadRegAdpt(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, uint8_t *buffer, uint16_t length);
static eMpu6050DrvIicStatus mpu6050PortHardIicInitAdpt(uint8_t bus);
static eMpu6050DrvIicStatus mpu6050PortHardIicWriteRegAdpt(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, const uint8_t *buffer, uint16_t length);
static eMpu6050DrvIicStatus mpu6050PortHardIicReadRegAdpt(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, uint8_t *buffer, uint16_t length);
static eMpu6050DrvIicStatus mpu6050PortMapDrvIicStat(eDrvIicStatus status);
static eMpu6050DrvIicStatus mpu6050PortMapDrvAnlogIicStat(eDrvAnlogIicStatus status);

static const stMpu6050PortIicInterface gMpu6050PortIicInterfaces[MPU6050_PORT_IIC_TYPE_MAX] = {
    [MPU6050_PORT_IIC_TYPE_SOFTWARE] = {
        .init = mpu6050PortSoftIicInitAdpt,
        .writeReg = mpu6050PortSoftIicWriteRegAdpt,
        .readReg = mpu6050PortSoftIicReadRegAdpt,
    },
    [MPU6050_PORT_IIC_TYPE_HARDWARE] = {
        .init = mpu6050PortHardIicInitAdpt,
        .writeReg = mpu6050PortHardIicWriteRegAdpt,
        .readReg = mpu6050PortHardIicReadRegAdpt,
    },
};

void mpu6050PortGetDefBind(stMpu6050PortIicBinding *bind)
{
    if (bind == NULL) {
        return;
    }

    bind->type = MPU6050_PORT_IIC_TYPE_HARDWARE;
    bind->bus = (uint8_t)DRVIIC_BUS0;
}

eMpu6050DrvIicStatus mpu6050PortSetSoftIic(stMpu6050PortIicBinding *bind, eDrvAnlogIicPortMap iic)
{
    if ((bind == NULL) || ((uint8_t)iic >= (uint8_t)DRVANLOGIIC_MAX)) {
        return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
    }

    bind->type = MPU6050_PORT_IIC_TYPE_SOFTWARE;
    bind->bus = (uint8_t)iic;
    return MPU6050_DRV_IIC_STATUS_OK;
}

eMpu6050DrvIicStatus mpu6050PortSetHardIic(stMpu6050PortIicBinding *bind, eDrvIicPortMap iic)
{
    if ((bind == NULL) || ((uint8_t)iic >= (uint8_t)DRVIIC_MAX)) {
        return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
    }

    bind->type = MPU6050_PORT_IIC_TYPE_HARDWARE;
    bind->bus = (uint8_t)iic;
    return MPU6050_DRV_IIC_STATUS_OK;
}

void mpu6050PortDelayMs(uint32_t delayMs)
{
#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
    TickType_t lDelayTicks;

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        lDelayTicks = pdMS_TO_TICKS(delayMs);
        if ((delayMs > 0U) && (lDelayTicks == 0U)) {
            lDelayTicks = 1U;
        }
        vTaskDelay(lDelayTicks);
        return;
    }
#endif

#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    uint32_t lCyclesPerMs;
    uint32_t lStartCycles;
    uint32_t lWaitCycles;

    if (delayMs == 0U) {
        return;
    }

    mpu6050PortEnableCycleCnt();
    lCyclesPerMs = SystemCoreClock / 1000U;
    if (lCyclesPerMs == 0U) {
        lCyclesPerMs = 1U;
    }

    lWaitCycles = lCyclesPerMs * delayMs;
    lStartCycles = DWT->CYCCNT;
    while ((DWT->CYCCNT - lStartCycles) < lWaitCycles) {
        __NOP();
    }
#else
    volatile uint32_t lOuter;
    volatile uint32_t lInner;

    for (lOuter = 0U; lOuter < delayMs; ++lOuter) {
        for (lInner = 0U; lInner < 5000U; ++lInner) {
        }
    }
#endif
}

bool mpu6050PortIsValidBind(const stMpu6050PortIicBinding *bind)
{
    if (bind == NULL) {
        return false;
    }

    switch (bind->type) {
        case MPU6050_PORT_IIC_TYPE_SOFTWARE:
            return bind->bus < (uint8_t)DRVANLOGIIC_MAX;
        case MPU6050_PORT_IIC_TYPE_HARDWARE:
            return bind->bus < (uint8_t)DRVIIC_MAX;
        default:
            return false;
    }
}

bool mpu6050PortHasValidIicIf(const stMpu6050PortIicBinding *bind)
{
    const stMpu6050PortIicInterface *lInterface;

    lInterface = mpu6050PortGetIicIf(bind);
    return (lInterface != NULL) &&
           (lInterface->init != NULL) &&
           (lInterface->writeReg != NULL) &&
           (lInterface->readReg != NULL);
}

const stMpu6050PortIicInterface *mpu6050PortGetIicIf(const stMpu6050PortIicBinding *bind)
{
    if (!mpu6050PortIsValidBind(bind)) {
        return NULL;
    }

    return &gMpu6050PortIicInterfaces[bind->type];
}

static eMpu6050DrvIicStatus mpu6050PortSoftIicInitAdpt(uint8_t bus)
{
    if (bus >= (uint8_t)DRVANLOGIIC_MAX) {
        return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
    }

    return mpu6050PortMapDrvAnlogIicStat(drvAnlogIicInit((eDrvAnlogIicPortMap)bus));
}

static eMpu6050DrvIicStatus mpu6050PortSoftIicWriteRegAdpt(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, const uint8_t *buffer, uint16_t length)
{
    if (bus >= (uint8_t)DRVANLOGIIC_MAX) {
        return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
    }

    return mpu6050PortMapDrvAnlogIicStat(drvAnlogIicWriteRegister((eDrvAnlogIicPortMap)bus, address, regBuf, regLen, buffer, length));
}

static eMpu6050DrvIicStatus mpu6050PortSoftIicReadRegAdpt(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, uint8_t *buffer, uint16_t length)
{
    if (bus >= (uint8_t)DRVANLOGIIC_MAX) {
        return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
    }

    return mpu6050PortMapDrvAnlogIicStat(drvAnlogIicReadRegister((eDrvAnlogIicPortMap)bus, address, regBuf, regLen, buffer, length));
}

static eMpu6050DrvIicStatus mpu6050PortHardIicInitAdpt(uint8_t bus)
{
    if (bus >= (uint8_t)DRVIIC_MAX) {
        return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
    }

    return mpu6050PortMapDrvIicStat(drvIicInit((eDrvIicPortMap)bus));
}

static eMpu6050DrvIicStatus mpu6050PortHardIicWriteRegAdpt(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, const uint8_t *buffer, uint16_t length)
{
    if (bus >= (uint8_t)DRVIIC_MAX) {
        return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
    }

    return mpu6050PortMapDrvIicStat(drvIicWriteRegister((eDrvIicPortMap)bus, address, regBuf, regLen, buffer, length));
}

static eMpu6050DrvIicStatus mpu6050PortHardIicReadRegAdpt(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, uint8_t *buffer, uint16_t length)
{
    if (bus >= (uint8_t)DRVIIC_MAX) {
        return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
    }

    return mpu6050PortMapDrvIicStat(drvIicReadRegister((eDrvIicPortMap)bus, address, regBuf, regLen, buffer, length));
}

static eMpu6050DrvIicStatus mpu6050PortMapDrvIicStat(eDrvIicStatus status)
{
    switch (status) {
        case DRVIIC_STATUS_OK:
            return MPU6050_DRV_IIC_STATUS_OK;
        case DRVIIC_STATUS_INVALID_PARAM:
            return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
        case DRVIIC_STATUS_NOT_READY:
            return MPU6050_DRV_IIC_STATUS_NOT_READY;
        case DRVIIC_STATUS_BUSY:
            return MPU6050_DRV_IIC_STATUS_BUSY;
        case DRVIIC_STATUS_TIMEOUT:
            return MPU6050_DRV_IIC_STATUS_TIMEOUT;
        case DRVIIC_STATUS_NACK:
            return MPU6050_DRV_IIC_STATUS_NACK;
        case DRVIIC_STATUS_UNSUPPORTED:
            return MPU6050_DRV_IIC_STATUS_UNSUPPORTED;
        case DRVIIC_STATUS_ERROR:
        default:
            return MPU6050_DRV_IIC_STATUS_ERROR;
    }
}

static eMpu6050DrvIicStatus mpu6050PortMapDrvAnlogIicStat(eDrvAnlogIicStatus status)
{
    switch (status) {
        case DRVANLOGIIC_STATUS_OK:
            return MPU6050_DRV_IIC_STATUS_OK;
        case DRVANLOGIIC_STATUS_INVALID_PARAM:
            return MPU6050_DRV_IIC_STATUS_INVALID_PARAM;
        case DRVANLOGIIC_STATUS_NOT_READY:
            return MPU6050_DRV_IIC_STATUS_NOT_READY;
        case DRVANLOGIIC_STATUS_BUSY:
            return MPU6050_DRV_IIC_STATUS_BUSY;
        case DRVANLOGIIC_STATUS_TIMEOUT:
            return MPU6050_DRV_IIC_STATUS_TIMEOUT;
        case DRVANLOGIIC_STATUS_NACK:
            return MPU6050_DRV_IIC_STATUS_NACK;
        case DRVANLOGIIC_STATUS_UNSUPPORTED:
            return MPU6050_DRV_IIC_STATUS_UNSUPPORTED;
        case DRVANLOGIIC_STATUS_ERROR:
        default:
            return MPU6050_DRV_IIC_STATUS_ERROR;
    }
}

static void mpu6050PortEnableCycleCnt(void)
{
#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    if (gMpu6050PortCycleCntReady) {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    gMpu6050PortCycleCntReady = true;
#endif
}

/**************************End of file********************************/

