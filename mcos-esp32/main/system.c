#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "app_wifi.h"
#include "drvgpio.h"
#include "system.h"

#define SYSTEM_TASK_STACK_SIZE 3072
#define SYSTEM_TASK_PRIORITY   5
#define WIFI_TASK_STACK_SIZE   6144
#define WIFI_TASK_PRIORITY     6

static const char *gTag = "system";

static void systemTask(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(gTag, "system task started");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void systemStartTasks(void)
{
    drvGpioInit();

    BaseType_t lResult = xTaskCreate(systemTask,
                                     "system_task",
                                     SYSTEM_TASK_STACK_SIZE,
                                     NULL,
                                     SYSTEM_TASK_PRIORITY,
                                     NULL);
    configASSERT(lResult == pdPASS);

    lResult = xTaskCreate(appWifiTask,
                          "wifi_task",
                          WIFI_TASK_STACK_SIZE,
                          NULL,
                          WIFI_TASK_PRIORITY,
                          NULL);
    configASSERT(lResult == pdPASS);

    ESP_LOGI(gTag, "system and wifi tasks created");
}