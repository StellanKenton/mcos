/***********************************************************************************
* @file     : mpu6050.c
* @brief    : MPU6050 sensor driver implementation.
* @details  : The driver configures and reads one explicitly selected MPU6050
*             device instance through the shared project port binding.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
**********************************************************************************/
#include "mpu6050.h"

#include <stddef.h>

static bool mpu6050IsValidDev(const stMpu6050Device *device);
static bool mpu6050IsReadyXfer(const stMpu6050Device *device);
static bool mpu6050IsCompatDevId(uint8_t devId);
static eMpu6050Status mpu6050MapIicStat(eMpu6050DrvIicStatus status);
static const stMpu6050PortIicInterface *mpu6050GetIicIf(const stMpu6050Device *device);
static eMpu6050Status mpu6050WriteRegInt(const stMpu6050Device *device, uint8_t regAddr, uint8_t value);
static eMpu6050Status mpu6050ReadRegInt(const stMpu6050Device *device, uint8_t regAddr, uint8_t *value);
static eMpu6050Status mpu6050ReadRegsInt(const stMpu6050Device *device, uint8_t regAddr, uint8_t *buffer, uint16_t length);
static int16_t mpu6050ParseBe16(const uint8_t *buffer);

void mpu6050GetDefCfg(stMpu6050Device *device)
{
    if (device == NULL) {
        return;
    }

    mpu6050PortGetDefBind(&device->iicBind);
    device->address = MPU6050_IIC_ADDRESS_LOW;
    device->sampleRateDiv = 0U;
    device->dlpfCfg = 3U;
    device->isReady = false;
    device->accelRange = MPU6050_ACCEL_RANGE_2G;
    device->gyroRange = MPU6050_GYRO_RANGE_250DPS;
}

eMpu6050Status mpu6050Init(stMpu6050Device *device)
{
    const stMpu6050PortIicInterface *lIicIf;
    uint8_t lDevId;
    uint8_t lValue;
    eMpu6050Status lStatus;

    if (!mpu6050IsValidDev(device)) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    if (!mpu6050PortHasValidIicIf(&device->iicBind)) {
        return mpu6050PortIsValidBind(&device->iicBind) ?
               MPU6050_STATUS_NOT_READY :
               MPU6050_STATUS_INVALID_PARAM;
    }

    lIicIf = mpu6050PortGetIicIf(&device->iicBind);
    lStatus = mpu6050MapIicStat(lIicIf->init(device->iicBind.bus));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    device->isReady = false;

    lStatus = mpu6050ReadRegInt(device, MPU6050_REG_WHO_AM_I, &lDevId);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    if (!mpu6050IsCompatDevId(lDevId)) {
        return MPU6050_STATUS_DEVICE_ID_MISMATCH;
    }

    lStatus = mpu6050WriteRegInt(device, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR1_DEVICE_RESET_BIT);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    mpu6050PortDelayMs(MPU6050_PORT_RESET_DELAY_MS);

    lValue = MPU6050_PWR1_CLKSEL_PLL_XGYRO;
    lStatus = mpu6050WriteRegInt(device, MPU6050_REG_PWR_MGMT_1, lValue);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    mpu6050PortDelayMs(MPU6050_PORT_WAKE_DELAY_MS);

    lStatus = mpu6050WriteRegInt(device, MPU6050_REG_PWR_MGMT_2, 0U);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegInt(device, MPU6050_REG_SMPLRT_DIV, device->sampleRateDiv);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegInt(device, MPU6050_REG_CONFIG, (uint8_t)(device->dlpfCfg & 0x07U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegInt(device, MPU6050_REG_GYRO_CONFIG, (uint8_t)((uint8_t)device->gyroRange << 3U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegInt(device, MPU6050_REG_ACCEL_CONFIG, (uint8_t)((uint8_t)device->accelRange << 3U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    device->isReady = true;
    return MPU6050_STATUS_OK;
}

bool mpu6050IsReady(const stMpu6050Device *device)
{
    return mpu6050IsReadyXfer(device);
}

eMpu6050Status mpu6050ReadId(stMpu6050Device *device, uint8_t *devId)
{
    const stMpu6050PortIicInterface *lIicIf;
    eMpu6050Status lStatus;

    if ((devId == NULL) || !mpu6050IsValidDev(device)) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    lIicIf = mpu6050PortGetIicIf(&device->iicBind);
    if (lIicIf == NULL) {
        return MPU6050_STATUS_NOT_READY;
    }

    lStatus = mpu6050MapIicStat(lIicIf->init(device->iicBind.bus));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    return mpu6050ReadRegInt(device, MPU6050_REG_WHO_AM_I, devId);
}

eMpu6050Status mpu6050ReadReg(stMpu6050Device *device, uint8_t regAddr, uint8_t *value)
{
    if (!mpu6050IsReadyXfer(device)) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050ReadRegInt(device, regAddr, value);
}

eMpu6050Status mpu6050WriteReg(stMpu6050Device *device, uint8_t regAddr, uint8_t value)
{
    if (!mpu6050IsReadyXfer(device)) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050WriteRegInt(device, regAddr, value);
}

eMpu6050Status mpu6050SetSleep(stMpu6050Device *device, bool enable)
{
    uint8_t lValue;
    eMpu6050Status lStatus;

    if (!mpu6050IsReadyXfer(device)) {
        return MPU6050_STATUS_NOT_READY;
    }

    lStatus = mpu6050ReadRegInt(device, MPU6050_REG_PWR_MGMT_1, &lValue);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    if (enable) {
        lValue |= MPU6050_PWR1_SLEEP_BIT;
    } else {
        lValue &= (uint8_t)(~MPU6050_PWR1_SLEEP_BIT);
        lValue = (uint8_t)((lValue & (uint8_t)(~0x07U)) | MPU6050_PWR1_CLKSEL_PLL_XGYRO);
    }

    return mpu6050WriteRegInt(device, MPU6050_REG_PWR_MGMT_1, lValue);
}

eMpu6050Status mpu6050ReadRaw(stMpu6050Device *device, stMpu6050RawSample *sample)
{
    uint8_t lBuffer[MPU6050_SAMPLE_BYTES];
    eMpu6050Status lStatus;

    if (sample == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    if (!mpu6050IsReadyXfer(device)) {
        return MPU6050_STATUS_NOT_READY;
    }

    lStatus = mpu6050ReadRegsInt(device, MPU6050_REG_ACCEL_XOUT_H, lBuffer, MPU6050_SAMPLE_BYTES);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    sample->accelX = mpu6050ParseBe16(&lBuffer[0]);
    sample->accelY = mpu6050ParseBe16(&lBuffer[2]);
    sample->accelZ = mpu6050ParseBe16(&lBuffer[4]);
    sample->temperature = mpu6050ParseBe16(&lBuffer[6]);
    sample->gyroX = mpu6050ParseBe16(&lBuffer[8]);
    sample->gyroY = mpu6050ParseBe16(&lBuffer[10]);
    sample->gyroZ = mpu6050ParseBe16(&lBuffer[12]);
    return MPU6050_STATUS_OK;
}

eMpu6050Status mpu6050ReadTempCdC(stMpu6050Device *device, int32_t *tempCdC)
{
    stMpu6050RawSample lSample;
    eMpu6050Status lStatus;

    if (tempCdC == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    lStatus = mpu6050ReadRaw(device, &lSample);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    *tempCdC = (((int32_t)lSample.temperature * 100) / 340) + 3653;
    return MPU6050_STATUS_OK;
}

static bool mpu6050IsValidDev(const stMpu6050Device *device)
{
    if (device == NULL) {
        return false;
    }

    if (!mpu6050PortIsValidBind(&device->iicBind)) {
        return false;
    }

    if ((device->address != MPU6050_IIC_ADDRESS_LOW) &&
        (device->address != MPU6050_IIC_ADDRESS_HIGH)) {
        return false;
    }

    if (device->dlpfCfg > 6U) {
        return false;
    }

    if (device->accelRange >= MPU6050_ACCEL_RANGE_MAX) {
        return false;
    }

    if (device->gyroRange >= MPU6050_GYRO_RANGE_MAX) {
        return false;
    }

    return true;
}

static bool mpu6050IsReadyXfer(const stMpu6050Device *device)
{
    return (device != NULL) &&
           device->isReady &&
           mpu6050PortHasValidIicIf(&device->iicBind);
}

static bool mpu6050IsCompatDevId(uint8_t devId)
{
    return (devId == MPU6050_WHO_AM_I_EXPECTED) ||
           (devId == MPU6050_WHO_AM_I_COMPATIBLE_6500);
}

static eMpu6050Status mpu6050MapIicStat(eMpu6050DrvIicStatus status)
{
    switch (status) {
        case MPU6050_DRV_IIC_STATUS_OK:
            return MPU6050_STATUS_OK;
        case MPU6050_DRV_IIC_STATUS_INVALID_PARAM:
            return MPU6050_STATUS_INVALID_PARAM;
        case MPU6050_DRV_IIC_STATUS_NOT_READY:
            return MPU6050_STATUS_NOT_READY;
        case MPU6050_DRV_IIC_STATUS_BUSY:
            return MPU6050_STATUS_BUSY;
        case MPU6050_DRV_IIC_STATUS_TIMEOUT:
            return MPU6050_STATUS_TIMEOUT;
        case MPU6050_DRV_IIC_STATUS_NACK:
            return MPU6050_STATUS_NACK;
        case MPU6050_DRV_IIC_STATUS_UNSUPPORTED:
        case MPU6050_DRV_IIC_STATUS_ERROR:
        default:
            return MPU6050_STATUS_ERROR;
    }
}

static const stMpu6050PortIicInterface *mpu6050GetIicIf(const stMpu6050Device *device)
{
    if ((device == NULL) || !mpu6050PortHasValidIicIf(&device->iicBind)) {
        return NULL;
    }

    return mpu6050PortGetIicIf(&device->iicBind);
}

static eMpu6050Status mpu6050WriteRegInt(const stMpu6050Device *device, uint8_t regAddr, uint8_t value)
{
    const stMpu6050PortIicInterface *lIicIf;

    lIicIf = mpu6050GetIicIf(device);
    if ((lIicIf == NULL) || (lIicIf->writeReg == NULL)) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050MapIicStat(lIicIf->writeReg(device->iicBind.bus, device->address, &regAddr, 1U, &value, 1U));
}

static eMpu6050Status mpu6050ReadRegInt(const stMpu6050Device *device, uint8_t regAddr, uint8_t *value)
{
    const stMpu6050PortIicInterface *lIicIf;

    lIicIf = mpu6050GetIicIf(device);
    if ((lIicIf == NULL) || (lIicIf->readReg == NULL)) {
        return MPU6050_STATUS_NOT_READY;
    }

    if (value == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    return mpu6050MapIicStat(lIicIf->readReg(device->iicBind.bus, device->address, &regAddr, 1U, value, 1U));
}

static eMpu6050Status mpu6050ReadRegsInt(const stMpu6050Device *device, uint8_t regAddr, uint8_t *buffer, uint16_t length)
{
    const stMpu6050PortIicInterface *lIicIf;

    lIicIf = mpu6050GetIicIf(device);
    if ((lIicIf == NULL) || (lIicIf->readReg == NULL)) {
        return MPU6050_STATUS_NOT_READY;
    }

    if ((buffer == NULL) || (length == 0U)) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    return mpu6050MapIicStat(lIicIf->readReg(device->iicBind.bus, device->address, &regAddr, 1U, buffer, length));
}

static int16_t mpu6050ParseBe16(const uint8_t *buffer)
{
    uint16_t lValue;

    lValue = ((uint16_t)buffer[0] << 8U) | (uint16_t)buffer[1];
    return (int16_t)lValue;
}

/**************************End of file********************************/

