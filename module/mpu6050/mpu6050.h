/************************************************************************************
* @file     : mpu6050.h
* @brief    : MPU6050 sensor driver built on the shared IIC drv layer.
* @details  : This module exposes a small blocking interface for basic device
*             initialization and raw sensor data acquisition through either
*             the software or hardware IIC drv implementation.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
***********************************************************************************/
#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>

#define MPU6050_IIC_INTERFACE_SOFTWARE    1
#define MPU6050_IIC_INTERFACE_HARDWARE    2

#ifndef MPU6050_IIC_INTERFACE
#define MPU6050_IIC_INTERFACE             MPU6050_IIC_INTERFACE_HARDWARE
#endif

#if (MPU6050_IIC_INTERFACE == MPU6050_IIC_INTERFACE_HARDWARE)
#include "drviic.h"
typedef eDrvIicPortMap eMpu6050IicPort;
#define MPU6050_IIC_BUS0                  DRVIIC_BUS0
#define MPU6050_IIC_MAX                   DRVIIC_MAX
typedef eDrvIicStatus eMpu6050DrvIicStatus;
#define MPU6050_DRV_IIC_STATUS_OK         DRVIIC_STATUS_OK
#define MPU6050_DRV_IIC_STATUS_INVALID_PARAM DRVIIC_STATUS_INVALID_PARAM
#define MPU6050_DRV_IIC_STATUS_NOT_READY  DRVIIC_STATUS_NOT_READY
#define MPU6050_DRV_IIC_STATUS_BUSY       DRVIIC_STATUS_BUSY
#define MPU6050_DRV_IIC_STATUS_TIMEOUT    DRVIIC_STATUS_TIMEOUT
#define MPU6050_DRV_IIC_STATUS_NACK       DRVIIC_STATUS_NACK
#define MPU6050_DRV_IIC_STATUS_UNSUPPORTED DRVIIC_STATUS_UNSUPPORTED
#define MPU6050_DRV_IIC_STATUS_ERROR      DRVIIC_STATUS_ERROR
#else
#include "drvanlogiic.h"
typedef eDrvAnlogIicPortMap eMpu6050IicPort;
#define MPU6050_IIC_BUS0                  DRVANLOGIIC_BUS0
#define MPU6050_IIC_MAX                   DRVANLOGIIC_MAX
typedef eDrvAnlogIicStatus eMpu6050DrvIicStatus;
#define MPU6050_DRV_IIC_STATUS_OK         DRVANLOGIIC_STATUS_OK
#define MPU6050_DRV_IIC_STATUS_INVALID_PARAM DRVANLOGIIC_STATUS_INVALID_PARAM
#define MPU6050_DRV_IIC_STATUS_NOT_READY  DRVANLOGIIC_STATUS_NOT_READY
#define MPU6050_DRV_IIC_STATUS_BUSY       DRVANLOGIIC_STATUS_BUSY
#define MPU6050_DRV_IIC_STATUS_TIMEOUT    DRVANLOGIIC_STATUS_TIMEOUT
#define MPU6050_DRV_IIC_STATUS_NACK       DRVANLOGIIC_STATUS_NACK
#define MPU6050_DRV_IIC_STATUS_UNSUPPORTED DRVANLOGIIC_STATUS_UNSUPPORTED
#define MPU6050_DRV_IIC_STATUS_ERROR      DRVANLOGIIC_STATUS_ERROR
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MPU6050_IIC_ADDRESS_LOW          0x68U
#define MPU6050_IIC_ADDRESS_HIGH         0x69U
#define MPU6050_WHO_AM_I_EXPECTED        0x68U
#define MPU6050_WHO_AM_I_COMPATIBLE_6500 0x70U

#define MPU6050_REG_SMPLRT_DIV           0x19U
#define MPU6050_REG_CONFIG               0x1AU
#define MPU6050_REG_GYRO_CONFIG          0x1BU
#define MPU6050_REG_ACCEL_CONFIG         0x1CU
#define MPU6050_REG_ACCEL_XOUT_H         0x3BU
#define MPU6050_REG_PWR_MGMT_1           0x6BU
#define MPU6050_REG_PWR_MGMT_2           0x6CU
#define MPU6050_REG_WHO_AM_I             0x75U

#define MPU6050_PWR1_DEVICE_RESET_BIT    0x80U
#define MPU6050_PWR1_SLEEP_BIT           0x40U
#define MPU6050_PWR1_CLKSEL_PLL_XGYRO    0x01U

#define MPU6050_RESET_DELAY_MS           100U
#define MPU6050_WAKE_DELAY_MS            10U
#define MPU6050_SAMPLE_BYTES             14U

typedef enum eMpu6050Status {
    MPU6050_STATUS_OK = 0,
    MPU6050_STATUS_INVALID_PARAM,
    MPU6050_STATUS_NOT_READY,
    MPU6050_STATUS_BUSY,
    MPU6050_STATUS_TIMEOUT,
    MPU6050_STATUS_NACK,
    MPU6050_STATUS_DEVICE_ID_MISMATCH,
    MPU6050_STATUS_ERROR,
} eMpu6050Status;

typedef enum eMpu6050AccelRange {
    MPU6050_ACCEL_RANGE_2G = 0,
    MPU6050_ACCEL_RANGE_4G,
    MPU6050_ACCEL_RANGE_8G,
    MPU6050_ACCEL_RANGE_16G,
    MPU6050_ACCEL_RANGE_MAX,
} eMpu6050AccelRange;

typedef enum eMpu6050GyroRange {
    MPU6050_GYRO_RANGE_250DPS = 0,
    MPU6050_GYRO_RANGE_500DPS,
    MPU6050_GYRO_RANGE_1000DPS,
    MPU6050_GYRO_RANGE_2000DPS,
    MPU6050_GYRO_RANGE_MAX,
} eMpu6050GyroRange;

typedef struct stMpu6050Config {
    eMpu6050IicPort iic;
    uint8_t address;
    uint8_t sampleRateDivider;
    uint8_t dlpfConfig;
    eMpu6050AccelRange accelRange;
    eMpu6050GyroRange gyroRange;
} stMpu6050Config;

typedef struct stMpu6050Context {
    stMpu6050Config config;
    bool isInitialized;
} stMpu6050Context;

typedef struct stMpu6050RawSample {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t temperature;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
} stMpu6050RawSample;

void mpu6050GetDefaultConfig(stMpu6050Config *config);
eMpu6050Status mpu6050Init(const stMpu6050Config *config);
bool mpu6050IsReady(void);
eMpu6050Status mpu6050ReadWhoAmI(uint8_t *deviceId);
eMpu6050Status mpu6050ReadRegister(uint8_t registerAddress, uint8_t *value);
eMpu6050Status mpu6050WriteRegister(uint8_t registerAddress, uint8_t value);
eMpu6050Status mpu6050SetSleepEnabled(bool enable);
eMpu6050Status mpu6050ReadRawSample(stMpu6050RawSample *sample);
eMpu6050Status mpu6050ReadTemperatureCentiDegC(int32_t *temperatureCentiDegC);

#ifdef __cplusplus
}
#endif

#endif  // MPU6050_H
/**************************End of file********************************/
