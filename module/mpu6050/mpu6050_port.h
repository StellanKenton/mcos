/************************************************************************************
* @file     : mpu6050_port.h
* @brief    : MPU6050 project port-layer definitions.
* @details  : This file keeps project-level bus mapping and underlying IIC
*             selection independent from the reusable MPU6050 core.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
***********************************************************************************/
#ifndef MPU6050_PORT_H
#define MPU6050_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "drvanlogiic.h"
#include "drviic.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MPU6050_PORT_RESET_DELAY_MS
#define MPU6050_PORT_RESET_DELAY_MS            100U
#endif

#ifndef MPU6050_PORT_WAKE_DELAY_MS
#define MPU6050_PORT_WAKE_DELAY_MS             10U
#endif

typedef enum eMpu6050PortIicType {
    MPU6050_PORT_IIC_TYPE_NONE = 0,
    MPU6050_PORT_IIC_TYPE_SOFTWARE,
    MPU6050_PORT_IIC_TYPE_HARDWARE,
    MPU6050_PORT_IIC_TYPE_MAX,
} eMpu6050PortIicType;

typedef enum eMpu6050DrvIicStatus {
    MPU6050_DRV_IIC_STATUS_OK = 0,
    MPU6050_DRV_IIC_STATUS_INVALID_PARAM,
    MPU6050_DRV_IIC_STATUS_NOT_READY,
    MPU6050_DRV_IIC_STATUS_BUSY,
    MPU6050_DRV_IIC_STATUS_TIMEOUT,
    MPU6050_DRV_IIC_STATUS_NACK,
    MPU6050_DRV_IIC_STATUS_UNSUPPORTED,
    MPU6050_DRV_IIC_STATUS_ERROR,
} eMpu6050DrvIicStatus;

typedef struct stMpu6050PortIicBinding {
    eMpu6050PortIicType type;
    uint8_t bus;
} stMpu6050PortIicBinding;

typedef eMpu6050DrvIicStatus (*mpu6050PortIicInitFunc)(uint8_t bus);
typedef eMpu6050DrvIicStatus (*mpu6050PortIicWriteRegisterFunc)(uint8_t bus, uint8_t address, const uint8_t *registerBuffer, uint16_t registerLength, const uint8_t *buffer, uint16_t length);
typedef eMpu6050DrvIicStatus (*mpu6050PortIicReadRegisterFunc)(uint8_t bus, uint8_t address, const uint8_t *registerBuffer, uint16_t registerLength, uint8_t *buffer, uint16_t length);

typedef struct stMpu6050PortIicInterface {
    mpu6050PortIicInitFunc init;
    mpu6050PortIicWriteRegisterFunc writeRegister;
    mpu6050PortIicReadRegisterFunc readRegister;
} stMpu6050PortIicInterface;

void mpu6050PortGetDefaultBinding(stMpu6050PortIicBinding *binding);
eMpu6050DrvIicStatus mpu6050PortSetSoftwareIic(stMpu6050PortIicBinding *binding, eDrvAnlogIicPortMap iic);
eMpu6050DrvIicStatus mpu6050PortSetHardwareIic(stMpu6050PortIicBinding *binding, eDrvIicPortMap iic);
bool mpu6050PortIsValidBinding(const stMpu6050PortIicBinding *binding);
bool mpu6050PortHasValidIicInterface(const stMpu6050PortIicBinding *binding);
const stMpu6050PortIicInterface *mpu6050PortGetIicInterface(const stMpu6050PortIicBinding *binding);
void mpu6050PortDelayMs(uint32_t delayMs);

#ifdef __cplusplus
}
#endif

#endif  // MPU6050_PORT_H
/**************************End of file********************************/

