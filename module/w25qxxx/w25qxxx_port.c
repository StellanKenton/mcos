/************************************************************************************
* @file     : w25qxxx_port.c
* @brief    : W25Qxxx module port-layer drvspi binding.
* @details  : This file binds the reusable W25Qxxx module to drvspi and provides
*             an RTOS-aware millisecond delay helper for busy polling.
***********************************************************************************/
#include "w25qxxx_port.h"

#include <stddef.h>

#include "Rep/rep_config.h"

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
#include "gd32f4xx.h"
#endif

static eW25qxxxPortStatus w25qxxxPortDrvSpiInit(const stW25qxxxPortBinding *binding)
{
    if ((binding == NULL) || (binding->type != W25QXXX_PORT_DRVSPI) || (binding->spi >= DRVSPI_MAX)) {
        return DRV_STATUS_INVALID_PARAM;
    }

    return drvSpiInit(binding->spi);
}

static eW25qxxxPortStatus w25qxxxPortDrvSpiTransfer(const stW25qxxxPortBinding *binding, const uint8_t *writeBuffer, uint16_t writeLength, const uint8_t *secondWriteBuffer, uint16_t secondWriteLength, uint8_t *readBuffer, uint16_t readLength, uint8_t readFillData)
{
    stDrvSpiTransfer lTransfer;

    if ((binding == NULL) || (binding->type != W25QXXX_PORT_DRVSPI) || (binding->spi >= DRVSPI_MAX)) {
        return DRV_STATUS_INVALID_PARAM;
    }

    lTransfer.writeBuffer = writeBuffer;
    lTransfer.writeLength = writeLength;
    lTransfer.secondWriteBuffer = secondWriteBuffer;
    lTransfer.secondWriteLength = secondWriteLength;
    lTransfer.readBuffer = readBuffer;
    lTransfer.readLength = readLength;
    lTransfer.readFillData = readFillData;

    return drvSpiTransfer(binding->spi, &lTransfer);
}

#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
static bool gW25qxxxPortCycleCounterReady = false;

static void w25qxxxPortEnableCycleCounter(void)
{
    if (gW25qxxxPortCycleCounterReady) {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    gW25qxxxPortCycleCounterReady = true;
}

static void w25qxxxPortDelayMsBareMetal(uint32_t delayMs)
{
    uint32_t lStartCycles;
    uint32_t lCyclesPerMs;
    uint32_t lWaitCycles;

    if (delayMs == 0U) {
        return;
    }

    w25qxxxPortEnableCycleCounter();
    lCyclesPerMs = SystemCoreClock / 1000U;
    if (lCyclesPerMs == 0U) {
        lCyclesPerMs = 1U;
    }

    lStartCycles = DWT->CYCCNT;
    lWaitCycles = lCyclesPerMs * delayMs;
    while ((DWT->CYCCNT - lStartCycles) < lWaitCycles) {
    }
}
#endif

static const stW25qxxxPortInterface gW25qxxxDrvSpiInterface = {
    .init = w25qxxxPortDrvSpiInit,
    .transfer = w25qxxxPortDrvSpiTransfer,
    .delayMs = w25qxxxPortDelayMs,
};

stW25qxxxPortBinding w25qxxxPortGetDefaultBinding(void)
{
    stW25qxxxPortBinding lBinding;

    lBinding.type = W25QXXX_PORT_DRVSPI;
    lBinding.spi = DRVSPI_BUS0;
    return lBinding;
}

void w25qxxxPortSetHardwareSpi(stW25qxxxPortBinding *binding, eDrvSpiPortMap spi)
{
    if (binding == NULL) {
        return;
    }

    binding->type = W25QXXX_PORT_DRVSPI;
    binding->spi = spi;
}

bool w25qxxxPortIsValidBinding(const stW25qxxxPortBinding *binding)
{
    if (binding == NULL) {
        return false;
    }

    return (binding->type == W25QXXX_PORT_DRVSPI) && (binding->spi < DRVSPI_MAX);
}

const stW25qxxxPortInterface *w25qxxxPortGetInterface(const stW25qxxxPortBinding *binding)
{
    if (!w25qxxxPortIsValidBinding(binding)) {
        return NULL;
    }

    if (binding->type == W25QXXX_PORT_DRVSPI) {
        return &gW25qxxxDrvSpiInterface;
    }

    return NULL;
}

void w25qxxxPortDelayMs(uint32_t delayMs)
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
    w25qxxxPortDelayMsBareMetal(delayMs);
#else
    volatile uint32_t lOuter;
    volatile uint32_t lInner;

    for (lOuter = 0U; lOuter < delayMs; ++lOuter) {
        for (lInner = 0U; lInner < 2000U; ++lInner) {
        }
    }
#endif
}
/**************************End of file********************************/
