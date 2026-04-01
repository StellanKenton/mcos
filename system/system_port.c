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
#include "console.h"
#include "drvlayer/DrvGpio/drvgpio.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gd32f4xx_gpio.h"
#include "system.h"
#include "system_debug.h"
#include "sys_int.h"
#include "drvgpio_debug.h"
#include "drvuart_debug.h"
#include "drvanlogiic.h"
#include "mpu6050.h"

#define SENSOR_TASK_TAG "SensorTask"

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
static bool createTasks(void);
static bool initializeConsole(void);
static eMpu6050Status initializeSensorMpu6050(void);
static eDrvAnlogIicStatus readSensorWhoAmI(uint8_t address, uint8_t *deviceId);

static void process(void)
{
    switch (systemGetMode()) {
        case eSYSTEM_INIT_MODE:
            LOG_I(SYSTEM_TAG, "System initialized");
            systemSetMode(eSYSTEM_SELF_CHECK_MODE);
            break;
        case eSYSTEM_SELF_CHECK_MODE:
            if (!createTasks()) {
                break;
            }
            LOG_I(SYSTEM_TAG, "Self-check passed");
            systemSetMode(eSYSTEM_STANDBY_MODE);
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
        vTaskDelay(pdMS_TO_TICKS(DEFAULT_TASK_PERIOD_MS));
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
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_TASK_PERIOD_MS));
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

static bool createTasks(void)
{
    bool lResult = true;

    if (initializeConsole()) {
        if (pdPASS != createTask(consoleTaskCallback,
            "ConsoleTask",
            CONSOLE_TASK_STACK_SIZE,
            CONSOLE_TASK_PRIORITY,
            &gConsoleTaskHandle)) {
            lResult = false;
        }
    } else {
        lResult = false;
    }

    if (pdPASS != createTask(sensorTaskCallback,
        "SensorTask",
        SENSOR_TASK_STACK_SIZE,
        SENSOR_TASK_PRIORITY,
        &gSensorTaskHandle)) {
        lResult = false;
    }

    if (pdPASS != createTask(guardTaskCallback,
        "GuardTask",
        GUARD_TASK_STACK_SIZE,
        GUARD_TASK_PRIORITY,
        &gGuardTaskHandle)) {
        lResult = false;
    }

    if (pdPASS != createTask(powerTaskCallback,
        "PowerTask",
        POWER_TASK_STACK_SIZE,
        POWER_TASK_PRIORITY,
        &gPowerTaskHandle)) {
        lResult = false;
    }

    if (pdPASS != createTask(memoryTaskCallback,
        "MemoryTask",
        MEMORY_TASK_STACK_SIZE,
        MEMORY_TASK_PRIORITY,
        &gMemoryTaskHandle)) {
        lResult = false;
    }

    return lResult;
}

static bool initializeConsole(void)
{
    static bool gConsoleReady = false;

    if (gConsoleReady) {
        return true;
    }

    if (!consoleInit()) {
        LOG_E(SYSTEM_TAG, "Console init failed");
        return false;
    }

    if (!systemDebugConsoleRegister()) {
        LOG_E(SYSTEM_TAG, "Register system console command failed");
        return false;
    }

    if (!drvGpioDebugConsoleRegister()) {
        LOG_E(SYSTEM_TAG, "Register GPIO console command failed");
        return false;
    }

    if (!drvUartDebugConsoleRegister()) {
        LOG_E(SYSTEM_TAG, "Register UART console command failed");
        return false;
    }

    gConsoleReady = true;
    LOG_I(SYSTEM_TAG, "Console initialized");
    return true;
}

static void sensorTaskCallback(void *parameter)
{
#if (SENSOR_TASK_MPU6050_LOG_SUPPORT == 1)
    TickType_t lLastWakeTime;
    stMpu6050RawSample lSample;
    eMpu6050Status lStatus;
    bool lSensorReady = false;
    uint32_t lDelayMs = SENSOR_TASK_PERIOD_MS;
#endif

    (void)parameter;

#if (SENSOR_TASK_MPU6050_LOG_SUPPORT == 1)
    lLastWakeTime = xTaskGetTickCount();

    for (;;) {
        if (!lSensorReady) {
            lStatus = initializeSensorMpu6050();
            lSensorReady = (lStatus == MPU6050_STATUS_OK);
            lDelayMs = lSensorReady ? SENSOR_TASK_PERIOD_MS : SENSOR_TASK_INIT_RETRY_PERIOD_MS;
        } else {
            lStatus = mpu6050ReadRawSample(&lSample);
            if (lStatus == MPU6050_STATUS_OK) {
                LOG_I(SENSOR_TASK_TAG,
                      "ax=%d ay=%d az=%d gx=%d gy=%d gz=%d tempRaw=%d",
                      (int)lSample.accelX,
                      (int)lSample.accelY,
                      (int)lSample.accelZ,
                      (int)lSample.gyroX,
                      (int)lSample.gyroY,
                      (int)lSample.gyroZ,
                      (int)lSample.temperature);
                lDelayMs = SENSOR_TASK_PERIOD_MS;
            } else {
                LOG_E(SENSOR_TASK_TAG, "Read MPU6050 sample failed: %d", (int)lStatus);
                lSensorReady = false;
                lDelayMs = SENSOR_TASK_INIT_RETRY_PERIOD_MS;
            }
        }

        vTaskDelayUntil(&lLastWakeTime, pdMS_TO_TICKS(lDelayMs));
    }
#else
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
    }
#endif
}

static eMpu6050Status initializeSensorMpu6050(void)
{
    stMpu6050Config lMpu6050Config;
    eDrvAnlogIicStatus lIicStatus;
    uint8_t lWhoAmI = 0U;
    uint8_t lAddressIndex;
    const uint8_t lProbeAddressList[2] = {
        MPU6050_IIC_ADDRESS_LOW,
        MPU6050_IIC_ADDRESS_HIGH,
    };
    eMpu6050Status lStatus;

    mpu6050GetDefaultConfig(&lMpu6050Config);

    for (lAddressIndex = 0U; lAddressIndex < 2U; ++lAddressIndex) {
        lMpu6050Config.address = lProbeAddressList[lAddressIndex];
        lIicStatus = readSensorWhoAmI(lMpu6050Config.address, &lWhoAmI);
        if (lIicStatus == DRVANLOGIIC_STATUS_NACK) {
            continue;
        }

        if (lIicStatus != DRVANLOGIIC_STATUS_OK) {
            LOG_E(SENSOR_TASK_TAG,
                  "Probe addr=0x%02X failed: %d",
                  (unsigned int)lMpu6050Config.address,
                  (int)lIicStatus);
            return MPU6050_STATUS_ERROR;
        }

        LOG_I(SENSOR_TASK_TAG,
              "Probe addr=0x%02X who_am_i=0x%02X",
              (unsigned int)lMpu6050Config.address,
              (unsigned int)lWhoAmI);
        if ((lWhoAmI != MPU6050_WHO_AM_I_EXPECTED) &&
            (lWhoAmI != MPU6050_WHO_AM_I_COMPATIBLE_6500)) {
            continue;
        }

        lStatus = mpu6050Init(&lMpu6050Config);
        if (lStatus == MPU6050_STATUS_OK) {
            LOG_I(SENSOR_TASK_TAG,
                  "MPU6050 initialized at addr=0x%02X",
                  (unsigned int)lMpu6050Config.address);
            return MPU6050_STATUS_OK;
        }

        LOG_E(SENSOR_TASK_TAG,
              "Init MPU6050 failed at addr=0x%02X: %d",
              (unsigned int)lMpu6050Config.address,
              (int)lStatus);
        return lStatus;
    }

    LOG_E(SENSOR_TASK_TAG, "No compatible MPU6050 detected on bus");
    return MPU6050_STATUS_DEVICE_ID_MISMATCH;
}

static eDrvAnlogIicStatus readSensorWhoAmI(uint8_t address, uint8_t *deviceId)
{
    uint8_t lRegisterAddress = MPU6050_REG_WHO_AM_I;
    eDrvAnlogIicStatus lStatus;

    if (deviceId == NULL) {
        return DRVANLOGIIC_STATUS_INVALID_PARAM;
    }

    lStatus = drvAnlogIicInit(DRVANLOGIIC_BUS0);
    if (lStatus != DRVANLOGIIC_STATUS_OK) {
        return lStatus;
    }

    return drvAnlogIicReadRegister(DRVANLOGIIC_BUS0,
                                   address,
                                   &lRegisterAddress,
                                   1U,
                                   deviceId,
                                   1U);
}

static void consoleTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {      
        consoleProcess();
        vTaskDelay(pdMS_TO_TICKS(CONSOLE_TASK_PERIOD_MS));
    }
}

static void guardTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(GUARD_TASK_PERIOD_MS));
    }
}

static void powerTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(POWER_TASK_PERIOD_MS));
    }
}

static void memoryTaskCallback(void *parameter)
{
    (void)parameter;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(MEMORY_TASK_PERIOD_MS));
    }
}
/**************************End of file********************************/
