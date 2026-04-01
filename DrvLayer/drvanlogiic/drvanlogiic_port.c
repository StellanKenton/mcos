/***********************************************************************************
* @file     : drvanlogiic_port.c
* @brief    : Software IIC port-layer BSP binding implementation.
* @details  : This file owns the project-level bus map and must be updated with
*             board-specific SDA and SCL hooks before the bus can be used.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "drvanlogiic.h"

#include <stddef.h>

stDrvAnlogIicBspInterface gDrvAnlogIicBspInterface[DRVANLOGIIC_MAX] = {
    [DRVANLOGIIC_BUS0] = {
        .init = NULL,
        .setScl = NULL,
        .setSda = NULL,
        .readScl = NULL,
        .readSda = NULL,
        .delayUs = NULL,
        .halfPeriodUs = DRVANLOGIIC_DEFAULT_HALF_PERIOD_US,
        .clockStretchTimeoutUs = DRVANLOGIIC_DEFAULT_STRETCH_TIMEOUT_US,
        .recoveryClockCount = DRVANLOGIIC_DEFAULT_RECOVERY_CLOCKS,
    },
};

/**************************End of file********************************/
