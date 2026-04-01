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

static bool mpu6050IsValidDevice(const stMpu6050Device *device);
static bool mpu6050IsReadyForTransfer(const stMpu6050Device *device);
static bool mpu6050IsCompatibleDeviceId(uint8_t deviceId);
static eMpu6050Status mpu6050MapIicStatus(eMpu6050DrvIicStatus status);
static const stMpu6050PortIicInterface *mpu6050GetIicInterface(const stMpu6050Device *device);
static eMpu6050Status mpu6050WriteRegisterInternal(const stMpu6050Device *device,
                                                   uint8_t registerAddress,
                                                   uint8_t value);
static eMpu6050Status mpu6050ReadRegisterInternal(const stMpu6050Device *device,
                                                  uint8_t registerAddress,
                                                  uint8_t *value);
static eMpu6050Status mpu6050ReadRegistersInternal(const stMpu6050Device *device,
                                                   uint8_t registerAddress,
                                                   uint8_t *buffer,
                                                   uint16_t length);
static int16_t mpu6050ParseBigEndianInt16(const uint8_t *buffer);

void mpu6050GetDefaultConfig(stMpu6050Device *device)
{
    if (device == NULL) {
        return;
    }

    mpu6050PortGetDefaultBinding(&device->iicBinding);
    device->address = MPU6050_IIC_ADDRESS_LOW;
    device->sampleRateDivider = 0U;
    device->dlpfConfig = 3U;
    device->isOnline = false;
    device->accelRange = MPU6050_ACCEL_RANGE_2G;
    device->gyroRange = MPU6050_GYRO_RANGE_250DPS;
}

eMpu6050Status mpu6050Init(stMpu6050Device *device)
{
    const stMpu6050PortIicInterface *lIicInterface;
    uint8_t lWhoAmI;
    uint8_t lValue;
    eMpu6050Status lStatus;

    if (!mpu6050IsValidDevice(device)) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    if (!mpu6050PortHasValidIicInterface(&device->iicBinding)) {
        return mpu6050PortIsValidBinding(&device->iicBinding) ?
               MPU6050_STATUS_NOT_READY :
               MPU6050_STATUS_INVALID_PARAM;
    }

    lIicInterface = mpu6050PortGetIicInterface(&device->iicBinding);
    lStatus = mpu6050MapIicStatus(lIicInterface->init(device->iicBinding.bus));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    device->isOnline = false;

    lStatus = mpu6050ReadRegisterInternal(device, MPU6050_REG_WHO_AM_I, &lWhoAmI);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    if (!mpu6050IsCompatibleDeviceId(lWhoAmI)) {
        return MPU6050_STATUS_DEVICE_ID_MISMATCH;
    }

    lStatus = mpu6050WriteRegisterInternal(device,
                                           MPU6050_REG_PWR_MGMT_1,
                                           MPU6050_PWR1_DEVICE_RESET_BIT);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    mpu6050PortDelayMs(MPU6050_PORT_RESET_DELAY_MS);

    lValue = MPU6050_PWR1_CLKSEL_PLL_XGYRO;
    lStatus = mpu6050WriteRegisterInternal(device, MPU6050_REG_PWR_MGMT_1, lValue);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    mpu6050PortDelayMs(MPU6050_PORT_WAKE_DELAY_MS);

    lStatus = mpu6050WriteRegisterInternal(device, MPU6050_REG_PWR_MGMT_2, 0U);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegisterInternal(device,
                                           MPU6050_REG_SMPLRT_DIV,
                                           device->sampleRateDivider);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegisterInternal(device,
                                           MPU6050_REG_CONFIG,
                                           (uint8_t)(device->dlpfConfig & 0x07U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegisterInternal(device,
                                           MPU6050_REG_GYRO_CONFIG,
                                           (uint8_t)((uint8_t)device->gyroRange << 3U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    lStatus = mpu6050WriteRegisterInternal(device,
                                           MPU6050_REG_ACCEL_CONFIG,
                                           (uint8_t)((uint8_t)device->accelRange << 3U));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    device->isOnline = true;
    return MPU6050_STATUS_OK;
}

bool mpu6050IsReady(const stMpu6050Device *device)
{
    return mpu6050IsReadyForTransfer(device);
}

eMpu6050Status mpu6050ReadWhoAmI(stMpu6050Device *device, uint8_t *deviceId)
{
    const stMpu6050PortIicInterface *lIicInterface;
    eMpu6050Status lStatus;

    if ((deviceId == NULL) || !mpu6050IsValidDevice(device)) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    lIicInterface = mpu6050PortGetIicInterface(&device->iicBinding);
    if (lIicInterface == NULL) {
        return MPU6050_STATUS_NOT_READY;
    }

    lStatus = mpu6050MapIicStatus(lIicInterface->init(device->iicBinding.bus));
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    return mpu6050ReadRegisterInternal(device, MPU6050_REG_WHO_AM_I, deviceId);
}

eMpu6050Status mpu6050ReadRegister(stMpu6050Device *device, uint8_t registerAddress, uint8_t *value)
{
    if (!mpu6050IsReadyForTransfer(device)) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050ReadRegisterInternal(device, registerAddress, value);
}

eMpu6050Status mpu6050WriteRegister(stMpu6050Device *device, uint8_t registerAddress, uint8_t value)
{
    if (!mpu6050IsReadyForTransfer(device)) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050WriteRegisterInternal(device, registerAddress, value);
}

eMpu6050Status mpu6050SetSleepEnabled(stMpu6050Device *device, bool enable)
{
    uint8_t lValue;
    eMpu6050Status lStatus;

    if (!mpu6050IsReadyForTransfer(device)) {
        return MPU6050_STATUS_NOT_READY;
    }

    lStatus = mpu6050ReadRegisterInternal(device, MPU6050_REG_PWR_MGMT_1, &lValue);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    if (enable) {
        lValue |= MPU6050_PWR1_SLEEP_BIT;
    } else {
        lValue &= (uint8_t)(~MPU6050_PWR1_SLEEP_BIT);
        lValue = (uint8_t)((lValue & (uint8_t)(~0x07U)) | MPU6050_PWR1_CLKSEL_PLL_XGYRO);
    }

    return mpu6050WriteRegisterInternal(device, MPU6050_REG_PWR_MGMT_1, lValue);
}

eMpu6050Status mpu6050ReadRawSample(stMpu6050Device *device, stMpu6050RawSample *sample)
{
    uint8_t lBuffer[MPU6050_SAMPLE_BYTES];
    eMpu6050Status lStatus;

    if (sample == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    if (!mpu6050IsReadyForTransfer(device)) {
        return MPU6050_STATUS_NOT_READY;
    }

    lStatus = mpu6050ReadRegistersInternal(device,
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

eMpu6050Status mpu6050ReadTemperatureCentiDegC(stMpu6050Device *device, int32_t *temperatureCentiDegC)
{
    stMpu6050RawSample lSample;
    eMpu6050Status lStatus;

    if (temperatureCentiDegC == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    lStatus = mpu6050ReadRawSample(device, &lSample);
    if (lStatus != MPU6050_STATUS_OK) {
        return lStatus;
    }

    *temperatureCentiDegC = (((int32_t)lSample.temperature * 100) / 340) + 3653;
    return MPU6050_STATUS_OK;
}

static bool mpu6050IsValidDevice(const stMpu6050Device *device)
{
    if (device == NULL) {
        return false;
    }

    if (!mpu6050PortIsValidBinding(&device->iicBinding)) {
        return false;
    }

    if ((device->address != MPU6050_IIC_ADDRESS_LOW) &&
        (device->address != MPU6050_IIC_ADDRESS_HIGH)) {
        return false;
    }

    if (device->dlpfConfig > 6U) {
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

static bool mpu6050IsReadyForTransfer(const stMpu6050Device *device)
{
    return (device != NULL) &&
           device->isOnline &&
           mpu6050PortHasValidIicInterface(&device->iicBinding);
}

static bool mpu6050IsCompatibleDeviceId(uint8_t deviceId)
{
    return (deviceId == MPU6050_WHO_AM_I_EXPECTED) ||
           (deviceId == MPU6050_WHO_AM_I_COMPATIBLE_6500);
}

static eMpu6050Status mpu6050MapIicStatus(eMpu6050DrvIicStatus status)
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

static const stMpu6050PortIicInterface *mpu6050GetIicInterface(const stMpu6050Device *device)
{
    if ((device == NULL) || !mpu6050PortHasValidIicInterface(&device->iicBinding)) {
        return NULL;
    }

    return mpu6050PortGetIicInterface(&device->iicBinding);
}

static eMpu6050Status mpu6050WriteRegisterInternal(const stMpu6050Device *device,
                                                   uint8_t registerAddress,
                                                   uint8_t value)
{
    const stMpu6050PortIicInterface *lIicInterface;

    lIicInterface = mpu6050GetIicInterface(device);
    if ((lIicInterface == NULL) || (lIicInterface->writeRegister == NULL)) {
        return MPU6050_STATUS_NOT_READY;
    }

    return mpu6050MapIicStatus(lIicInterface->writeRegister(device->iicBinding.bus,
                                                            device->address,
                                                            &registerAddress,
                                                            1U,
                                                            &value,
                                                            1U));
}

static eMpu6050Status mpu6050ReadRegisterInternal(const stMpu6050Device *device,
                                                  uint8_t registerAddress,
                                                  uint8_t *value)
{
    const stMpu6050PortIicInterface *lIicInterface;

    lIicInterface = mpu6050GetIicInterface(device);
    if ((lIicInterface == NULL) || (lIicInterface->readRegister == NULL)) {
        return MPU6050_STATUS_NOT_READY;
    }

    if (value == NULL) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    return mpu6050MapIicStatus(lIicInterface->readRegister(device->iicBinding.bus,
                                                           device->address,
                                                           &registerAddress,
                                                           1U,
                                                           value,
                                                           1U));
}

static eMpu6050Status mpu6050ReadRegistersInternal(const stMpu6050Device *device,
                                                   uint8_t registerAddress,
                                                   uint8_t *buffer,
                                                   uint16_t length)
{
    const stMpu6050PortIicInterface *lIicInterface;

    lIicInterface = mpu6050GetIicInterface(device);
    if ((lIicInterface == NULL) || (lIicInterface->readRegister == NULL)) {
        return MPU6050_STATUS_NOT_READY;
    }

    if ((buffer == NULL) || (length == 0U)) {
        return MPU6050_STATUS_INVALID_PARAM;
    }

    return mpu6050MapIicStatus(lIicInterface->readRegister(device->iicBinding.bus,
                                                           device->address,
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
