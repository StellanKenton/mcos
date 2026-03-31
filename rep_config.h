/************************************************************************************
* @file     : rep_config.h
* @brief    : Global repository configuration.
* @details  : Stores the current MCU platform, RTOS type, and compiled log level.
* @author   : GitHub Copilot
* @date     : 2026-03-31
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef REP_CONFIG_H
#define REP_CONFIG_H

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif


#define REP_MCU_PLATFORM_ESP32        1
#define REP_MCU_PLATFORM_STM32        2
#define REP_MCU_PLATFORM_GD32         3

#define REP_RTOS_FREERTOS             1
#define REP_RTOS_RTTHREAD             2
#define REP_RTOS_BAREMETAL            3

#define REP_LOG_LEVEL_NONE            0
#define REP_LOG_LEVEL_ERROR           1
#define REP_LOG_LEVEL_WARN            2
#define REP_LOG_LEVEL_INFO            3
#define REP_LOG_LEVEL_DEBUG           4

#define REP_LOG_LEVEL                 REP_LOG_LEVEL_INFO

#define REP_MCU_PLATFORM              REP_MCU_PLATFORM_ESP32
#define REP_RTOS_SYSTEM               REP_RTOS_FREERTOS


#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_ESP32)
#define MCU_PLATFORM_ESP32
#endif
#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_STM32)
#define MCU_PLATFORM_STM32
#endif
#if (REP_MCU_PLATFORM == REP_MCU_PLATFORM_GD32)
#define MCU_PLATFORM_GD32
#endif


#ifdef __cplusplus
}
#endif

#endif  // REP_CONFIG_H
/**************************End of file********************************/