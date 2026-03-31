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

static void process(void);
static BaseType_t createTask(TaskFunction_t taskFunction,
    const char *taskName,
    configSTACK_DEPTH_TYPE stackDepth,
    UBaseType_t taskPriority,
    TaskHandle_t *taskHandle);
static void sensorTaskCallback(void *parameter);
static void consoleTaskCallback(void *parameter);
static void guardTaskCallback(void *parameter);
static void powerTaskCallback(void *parameter);
static void memoryTaskCallback(void *parameter);
static void createTasks(void);

static void process(void)
{
    switch (systemGetMode()) {
        case eSYSTEM_INIT_MODE:
            LOG_I(SYSTEM_TAG, "System initialized");
            systemSetMode(eSYSTEM_SELF_CHECK_MODE);
            break;
        case eSYSTEM_SELF_CHECK_MODE:  
            LOG_I(SYSTEM_TAG, "Self-check passed");
            systemSetMode(eSYSTEM_STANDBY_MODE);
            createTasks();
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
void defaultTaskCallback(void *parameter)
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
void systemTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        process();
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_SYSTEM_TASK_PERIOD_MS));
    }
}

static BaseType_t createTask(TaskFunction_t taskFunction,
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

static void createTasks(void)
{
    (void)createTask(sensorTaskCallback,
        "SensorTask",
        SYSTEM_PORT_SENSOR_TASK_STACK_SIZE,
        SYSTEM_PORT_SENSOR_TASK_PRIORITY,
        &gSensorTaskHandle);
    (void)createTask(consoleTaskCallback,
        "ConsoleTask",
        SYSTEM_PORT_CONSOLE_TASK_STACK_SIZE,
        SYSTEM_PORT_CONSOLE_TASK_PRIORITY,
        &gConsoleTaskHandle);
    (void)createTask(guardTaskCallback,
        "GuardTask",
        SYSTEM_PORT_GUARD_TASK_STACK_SIZE,
        SYSTEM_PORT_GUARD_TASK_PRIORITY,
        &gGuardTaskHandle);
    (void)createTask(powerTaskCallback,
        "PowerTask",
        SYSTEM_PORT_POWER_TASK_STACK_SIZE,
        SYSTEM_PORT_POWER_TASK_PRIORITY,
        &gPowerTaskHandle);
    (void)createTask(memoryTaskCallback,
        "MemoryTask",
        SYSTEM_PORT_MEMORY_TASK_STACK_SIZE,
        SYSTEM_PORT_MEMORY_TASK_PRIORITY,
        &gMemoryTaskHandle);
}

static void sensorTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_SENSOR_TASK_PERIOD_MS));
    }
}

static void consoleTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_CONSOLE_TASK_PERIOD_MS));
    }
}

static void guardTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_GUARD_TASK_PERIOD_MS));
    }
}

static void powerTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_POWER_TASK_PERIOD_MS));
    }
}

static void memoryTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_PORT_MEMORY_TASK_PERIOD_MS));
    }
}
/**************************End of file********************************/
