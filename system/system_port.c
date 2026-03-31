/***********************************************************************************
* @file     : system_port.c
* @brief    : System task callback implementation.
* @details  : Contains task callback bodies used by the application startup.
* @author   : GitHub Copilot
* @date     : 2026-03-31
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "system_port.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gd32f4xx_gpio.h"
#include "system.h"
#include "sys_int.h"

static TaskHandle_t gSensorTaskHandle = NULL;
static TaskHandle_t gConsoleTaskHandle = NULL;
static TaskHandle_t gGuardTaskHandle = NULL;
static TaskHandle_t gPowerTaskHandle = NULL;
static TaskHandle_t gMemoryTaskHandle = NULL;

static void systemPortProcess(void);
static BaseType_t systemPortCreateTask(TaskFunction_t taskFunction,
    const char *taskName,
    configSTACK_DEPTH_TYPE stackDepth,
    UBaseType_t taskPriority,
    TaskHandle_t *taskHandle);
static void systemPortSensorTaskCallback(void *parameter);
static void systemPortConsoleTaskCallback(void *parameter);
static void systemPortGuardTaskCallback(void *parameter);
static void systemPortPowerTaskCallback(void *parameter);
static void systemPortMemoryTaskCallback(void *parameter);
static void systemPortCreateTasks(void);

static void systemPortProcess(void)
{
    switch (systemGetMode()) {
        case eSYSTEM_INIT_MODE:
            LOG_I(SYSTEM_TAG, "System initialized");
            systemSetMode(eSYSTEM_SELF_CHECK_MODE);
            break;
        case eSYSTEM_SELF_CHECK_MODE:  
            LOG_I(SYSTEM_TAG, "Self-check passed");
            systemSetMode(eSYSTEM_STANDBY_MODE);
            systemPortCreateTasks();
            break;
        case eSYSTEM_STANDBY_MODE:
            break;
        case eSYSTEM_NORMAL_MODE:
        case eSYSTEM_UPDATE_MODE:
        case eSYSTEM_DIAGNOSTIC_MODE:
            break;
        default:
            LOG_W(SYSTEM_TAG, "Unknown system mode: %d", (int)systemGetMode());
            systemSetMode(eSYSTEM_INIT_MODE);
            break;
    }
}
/**
* @brief : Default task callback.
* @param : parameter - task parameter, unused.
* @return: None
**/
void systemPortDefaultTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        gpio_bit_toggle(STATUS_LED_GPIO_PORT, STATUS_LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_DEFAULT_TASK_PERIOD_MS));
    }
}

/**
* @brief : System task callback.
* @param : parameter - task parameter, unused.
* @return: None
**/
void systemPortSystemTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        systemPortProcess();
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_SYSTEM_TASK_PERIOD_MS));
    }
}

static BaseType_t systemPortCreateTask(TaskFunction_t taskFunction,
    const char *taskName,
    configSTACK_DEPTH_TYPE stackDepth,
    UBaseType_t taskPriority,
    TaskHandle_t *taskHandle)
{
    BaseType_t lReturn;

    if ((NULL == taskFunction) || (NULL == taskName) || (NULL == taskHandle)) {
        return pdFAIL;
    }

    if (NULL != *taskHandle) {
        return pdPASS;
    }

    lReturn = xTaskCreate(taskFunction,
        taskName,
        stackDepth,
        NULL,
        taskPriority,
        taskHandle);
    if (pdPASS != lReturn) {
        LOG_E(SYSTEM_TAG, "Create task failed: %s", taskName);
    }

    return lReturn;
}

static void systemPortCreateTasks(void)
{
    (void)systemPortCreateTask(systemPortSensorTaskCallback,
        "SensorTask",
        SYSTEM_PORT_SENSOR_TASK_STACK_SIZE,
        SYSTEM_PORT_SENSOR_TASK_PRIORITY,
        &gSensorTaskHandle);
    (void)systemPortCreateTask(systemPortConsoleTaskCallback,
        "ConsoleTask",
        SYSTEM_PORT_CONSOLE_TASK_STACK_SIZE,
        SYSTEM_PORT_CONSOLE_TASK_PRIORITY,
        &gConsoleTaskHandle);
    (void)systemPortCreateTask(systemPortGuardTaskCallback,
        "GuardTask",
        SYSTEM_PORT_GUARD_TASK_STACK_SIZE,
        SYSTEM_PORT_GUARD_TASK_PRIORITY,
        &gGuardTaskHandle);
    (void)systemPortCreateTask(systemPortPowerTaskCallback,
        "PowerTask",
        SYSTEM_PORT_POWER_TASK_STACK_SIZE,
        SYSTEM_PORT_POWER_TASK_PRIORITY,
        &gPowerTaskHandle);
    (void)systemPortCreateTask(systemPortMemoryTaskCallback,
        "MemoryTask",
        SYSTEM_PORT_MEMORY_TASK_STACK_SIZE,
        SYSTEM_PORT_MEMORY_TASK_PRIORITY,
        &gMemoryTaskHandle);
}

static void systemPortSensorTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_SENSOR_TASK_PERIOD_MS));
    }
}

static void systemPortConsoleTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_CONSOLE_TASK_PERIOD_MS));
    }
}

static void systemPortGuardTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_GUARD_TASK_PERIOD_MS));
    }
}

static void systemPortPowerTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_POWER_TASK_PERIOD_MS));
    }
}

static void systemPortMemoryTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_MEMORY_TASK_PERIOD_MS));
    }
}
/**************************End of file********************************/
