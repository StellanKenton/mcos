#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "app_wifi.h"
#include "bsp_wifi.h"

static const char *gTag = "app_wifi";

void appWifiTask(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(gTag, "wifi task started");
    wifi_init_sta();

    ESP_LOGI(gTag, "wifi task completed");
    vTaskDelete(NULL);
}