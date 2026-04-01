/***********************************************************************************
* @file     : mpu6050.c
* @brief    : MPU6050 sensor driver implementation.
* @details  : The driver uses the reusable software IIC drv layer to configure
*             and read a single MPU6050 device instance.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
**********************************************************************************/
#include "mpu6050.h"

#include <stddef.h>

#include "rep_config.h"

#if (REP_RTOS_SYSTEM == REP_RTOS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
#include "gd32f4xx.h"
#endif


static stMpu6050Context gMpu6050Context;
static bool gMpu6050CycleCounterReady = false;

static bool mpu6050IsValidConfig(const stMpu6050Config *config);
static bool mpu6050IsCompatibleDeviceId(uint8_t deviceId);
static eMpu6050Status mpu6050MapIicStatus(eDrvAnlogIicStatus status);
static void mpu6050DelayMs(uint32_t delayMs);
static void mpu6050EnableCycleCounter(void);
static eMpu6050Status mpu6050WriteRegisterInternal(eDrvAnlogIicPortMap iic,
                                                   uint8_t address,
                                                   uint8_t registerAddress,
                                                   uint8_t value);
static eMpu6050Status mpu6050ReadRegisterInternal(eDrvAnlogIicPortMap iic,
                                                  uint8_t address,
                                                  uint8_t registerAddress,
                                                  uint8_t *value);
static eMpu6050Status mpu6050ReadRegistersInternal(eDrvAnlogIicPortMap iic,
                                                   uint8_t address,
                                                   uint8_t registerAddress,
                                                   uint8_t *buffer,
                                                   uint16_t length);
static int16_t mpu6050ParseBigEndianInt16(const uint8_t *buffer);

void mpu6050GetDefaultConfig(stMpu6050Config *config)
{
    if (config == NULL) {
        return;
    }

    config->iic = DRVANLOGIIC_BUS0;
    config->address = MPU6050_IIC_ADDRESS_LOW;
    config->sampleRateDivider = 0U;
    config->dlpfConfig = 3U;
    config->accelRange = MPU6050_ACCEL_RANGE_2G;
    config->gyroRange = MPU6050_GYRO_RANGE_250DPS;
}

eMpu6050Status mpu6050Init(const stMpu6050Config *config)
{
    uint8_t lWhoAmI;
    uint8_t lValue;
    eMpu6050Status lStatus;

    if (!mpu6050IsValidConfig(config)) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    lStatus = mpu6050MapIicStatus(drvAnlogIicInit(config->iic));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    gMpu6050Context.config = *config;
    gMpu6050Context.isInitialized = false;

    lStatus = mpu6050ReadRegisterInternal(config->iic,
                                          config->address,
                                          MPU6050_REG_WHO_AM_I,
                                          &lWhoAmI);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    if (!mpu6050IsCompatibleDeviceId(lWhoAmI)) {
        return MPU6050_STATUS_DEVICE_ID_MISMATCH;
    }

    lStatus = mpu6050WriteRegisterInternal(config->iic,
                                           config->address,
                                           MPU6050_REG_PWR_MGMT_1,
                                           MPU6050_PWR1_DEVICE_RESET_BIT);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    mpu6050DelayMs(MPU6050_RESET_DELAY_MS);

    lValue = MPU6050_PWR1_CLKSEL_PLL_XGYRO;
    lStatus = mpu6050WriteRegisterInternal(config->iic,
                                           config->address,
                                           MPU6050_REG_PWR_MGMT_1,
                                           lValue);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    mpu6050DelayMs(MPU6050_WAKE_DELAY_MS);

    lStatus = mpu6050WriteRegisterInternal(config->iic,
                                           config->address,
                                           MPU6050_REG_PWR_MGMT_2,
                                           0U);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegisterInternal(config->iic,
                                           config->address,
                                           MPU6050_REG_SMPLRT_DIV,
                                           config->sampleRateDivider);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegisterInternal(config->iic,
                                           config->address,
                                           MPU6050_REG_CONFIG,
                                           (uint8_t)(config->dlpfConfig & 0x07U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegisterInternal(config->iic,
                                           config->address,
                                           MPU6050_REG_GYRO_CONFIG,
                                           (uint8_t)((uint8_t)config->gyroRange << 3U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegisterInternal(config->iic,
                                           config->address,
                                           MPU6050_REG_ACCEL_CONFIG,
                                           (uint8_t)((uint8_t)config->accelRange << 3U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    gMpu6050Context.isInitialized = true;
    return MPU6050_STATUS_OK;
}

bool mpu6050IsReady(void)
{
    return gMpu6050Context.isInitialized;
}

eMpu6050Status mpu6050ReadWhoAmI(uint8_t *deviceId)
{
    if (deviceId == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    if (!gMpu6050Context.isInitialized) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050ReadRegisterInternal(gMpu6050Context.config.iic,
                                       gMpu6050Context.config.address,
                                       MPU6050_REG_WHO_AM_I,
                                       deviceId);
}

eMpu6050Status mpu6050ReadRegister(uint8_t registerAddress, uint8_t *value)
{
    if (value == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    if (!gMpu6050Context.isInitialized) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050ReadRegisterInternal(gMpu6050Context.config.iic,
                                       gMpu6050Context.config.address,
                                       registerAddress,
                                       value);
}

eMpu6050Status mpu6050WriteRegister(uint8_t registerAddress, uint8_t value)
{
    if (!gMpu6050Context.isInitialized) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050WriteRegisterInternal(gMpu6050Context.config.iic,
                                        gMpu6050Context.config.address,
                                        registerAddress,
                                        value);
}

eMpu6050Status mpu6050SetSleepEnabled(bool enable)
{
    uint8_t lValue;
    eMpu6050Status lStatus;

    if (!gMpu6050Context.isInitialized) {
        return MPU6050_STATUS_NOT_READY;
    }

    lStatus = mpu6050ReadRegister(MPU6050_REG_PWR_MGMT_1, &lValue);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    if (enable) {
        lValue |= MPU6050_PWR1_SLEEP_BIT;
    } else {
        lValue &= (uint8_t)(~MPU6050_PWR1_SLEEP_BIT);
        lValue = (uint8_t)((lValue & (uint8_t)(~0x07U)) | MPU6050_PWR1_CLKSEL_PLL_XGYRO);
    }

    return mpu6050WriteRegister(MPU6050_REG_PWR_MGMT_1, lValue);
}

eMpu6050Status mpu6050ReadRawSample(stMpu6050RawSample *sample)
{
    uint8_t lBuffer[MPU6050_SAMPLE_BYTES];
    eMpu6050Status lStatus;

    if (sample == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    if (!gMpu6050Context.isInitialized) {
        return MPU6050_STATUS_NOT_READY;
    }

    lStatus = mpu6050ReadRegistersInternal(gMpu6050Context.config.iic,
                                           gMpu6050Context.config.address,
                                           MPU6050_REG_ACCEL_XOUT_H,
                                           lBuffer,
                                           MPU6050_SAMPLE_BYTES);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    sample->accelX = mpu6050ParseBigEndianInt16(&lBuffer[0]);
    sample->accelY = mpu6050ParseBigEndianInt16(&lBuffer[2]);
    sample->accelZ = mpu6050ParseBigEndianInt16(&lBuffer[4]);
    sample->temperature = mpu6050ParseBigEndianInt16(&lBuffer[6]);
    sample->gyroX = mpu6050ParseBigEndianInt16(&lBuffer[8]);
    sample->gyroY = mpu6050ParseBigEndianInt16(&lBuffer[10]);
    sample->gyroZ = mpu6050ParseBigEndianInt16(&lBuffer[12]);
    return MPU6050_STATUS_OK;
}

eMpu6050Status mpu6050ReadTemperatureCentiDegC(int32_t *temperatureCentiDegC)
{
    stMpu6050RawSample lSample;
    eMpu6050Status lStatus;

    if (temperatureCentiDegC == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    lStatus = mpu6050ReadRawSample(&lSample);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    *temperatureCentiDegC = (((int32_t)lSample.temperature * 100) / 340) + 3653;
    return MPU6050_STATUS_OK;
}

static bool mpu6050IsValidConfig(const stMpu6050Config *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->iic >= DRVANLOGIIC_MAX) {
        return false;
    }

    if ((config->address != MPU6050_IIC_ADDRESS_LOW) &&
        (config->address != MPU6050_IIC_ADDRESS_HIGH)) {
        return false;
    }

    if (config->dlpfConfig > 6U) {
        return false;
    }

    if (config->accelRange >= MPU6050_ACCEL_RANGE_MAX) {
        return false;
    }

    if (config->gyroRange >= MPU6050_GYRO_RANGE_MAX) {
        return false;
    }

    return true;
}

static bool mpu6050IsCompatibleDeviceId(uint8_t deviceId)
{
    return (deviceId == MPU6050_WHO_AM_I_EXPECTED) ||
           (deviceId == MPU6050_WHO_AM_I_COMPATIBLE_6500);
}

static eMpu6050Status mpu6050MapIicStatus(eDrvAnlogIicStatus status)
{
    switch (status) {
        case DRVANLOGIIC_STATUS_OK:
            return MPU6050_STATUS_OK;
        case DRVANLOGIIC_STATUS_INVALID_PARAM:
            return MPU6050_STATUS_INVALID_PARAM;
        case DRVANLOGIIC_STATUS_NOT_READY:
            return MPU6050_STATUS_NOT_READY;
        case DRVANLOGIIC_STATUS_BUSY:
            return MPU6050_STATUS_BUSY;
        case DRVANLOGIIC_STATUS_TIMEOUT:
            return MPU6050_STATUS_TIMEOUT;
        case DRVANLOGIIC_STATUS_NACK:
            return MPU6050_STATUS_NACK;
        case DRVANLOGIIC_STATUS_UNSUPPORTED:
        case DRVANLOGIIC_STATUS_ERROR:
        default:
            return MPU6050_STATUS_ERROR;
    }
}

static void mpu6050EnableCycleCounter(void)
{
#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
    if (gMpu6050CycleCounterReady) {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    gMpu6050CycleCounterReady = true;
#endif
}

static void mpu6050DelayMs(uint32_t delayMs)
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

    mpu6050EnableCycleCounter();
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

static eMpu6050Status mpu6050WriteRegisterInternal(eDrvAnlogIicPortMap iic,
                                                   uint8_t address,
                                                   uint8_t registerAddress,
                                                   uint8_t value)
{
    return mpu6050MapIicStatus(drvAnlogIicWriteRegister(iic,
                                                        address,
                                                        &registerAddress,
                                                        1U,
                                                        &value,
                                                        1U));
}

static eMpu6050Status mpu6050ReadRegisterInternal(eDrvAnlogIicPortMap iic,
                                                  uint8_t address,
                                                  uint8_t registerAddress,
                                                  uint8_t *value)
{
    if (value == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    return mpu6050MapIicStatus(drvAnlogIicReadRegister(iic,
                                                       address,
                                                       &registerAddress,
                                                       1U,
                                                       value,
                                                       1U));
}

static eMpu6050Status mpu6050ReadRegistersInternal(eDrvAnlogIicPortMap iic,
                                                   uint8_t address,
                                                   uint8_t registerAddress,
                                                   uint8_t *buffer,
                                                   uint16_t length)
{
    if ((buffer == NULL) || (length == 0U)) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    return mpu6050MapIicStatus(drvAnlogIicReadRegister(iic,
                                                       address,
                                                       &registerAddress,
                                                       1U,
                                                       buffer,
                                                       length));
}

static int16_t mpu6050ParseBigEndianInt16(const uint8_t *buffer)
{
    uint16_t lValue;

    lValue = ((uint16_t)buffer[0] << 8U) | (uint16_t)buffer[1];
    return (int16_t)lValue;
}

/**************************End of file********************************/
