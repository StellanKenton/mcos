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
typedef eMpu6050DrvIicStatus (*mpu6050PortIicWriteRegFunc)(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, const uint8_t *buffer, uint16_t length);
typedef eMpu6050DrvIicStatus (*mpu6050PortIicReadRegFunc)(uint8_t bus, uint8_t address, const uint8_t *regBuf, uint16_t regLen, uint8_t *buffer, uint16_t length);

typedef struct stMpu6050PortIicInterface {
    mpu6050PortIicInitFunc init;
    mpu6050PortIicWriteRegFunc writeReg;
    mpu6050PortIicReadRegFunc readReg;
} stMpu6050PortIicInterface;

void mpu6050PortGetDefBind(stMpu6050PortIicBinding *bind);
eMpu6050DrvIicStatus mpu6050PortSetSoftIic(stMpu6050PortIicBinding *bind, eDrvAnlogIicPortMap iic);
eMpu6050DrvIicStatus mpu6050PortSetHardIic(stMpu6050PortIicBinding *bind, eDrvIicPortMap iic);
bool mpu6050PortIsValidBind(const stMpu6050PortIicBinding *bind);
bool mpu6050PortHasValidIicIf(const stMpu6050PortIicBinding *bind);
const stMpu6050PortIicInterface *mpu6050PortGetIicIf(const stMpu6050PortIicBinding *bind);
void mpu6050PortDelayMs(uint32_t delayMs);

#ifdef __cplusplus
}
#endif

#endif  // MPU6050_PORT_H
/**************************End of file********************************/

