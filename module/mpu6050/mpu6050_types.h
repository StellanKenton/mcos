/***********************************************************************************
* @file     : mpu6050_types.h
* @brief    : MPU6050 shared public type definitions.
* @details  : This file keeps public MPU6050 map types separate from driver and
*             port-layer headers to avoid circular include dependencies.
* @author   : GitHub Copilot
* @date     : 2026-04-02
* @version  : V1.0.0
**********************************************************************************/
#ifndef MPU6050_TYPES_H
#define MPU6050_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eMPU6050DevMap {
    MPU6050_DEV0 = 0,
    MPU6050_DEV1,
    MPU6050_DEV_MAX,
} eMPU6050MapType;

#ifdef __cplusplus
}
#endif

#endif  // MPU6050_TYPES_H
/**************************End of file********************************/
