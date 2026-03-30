#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "system.h"

static const char *gTag = "main";
 
void app_main(void)
{
  esp_err_t lRet = nvs_flash_init();
  if (lRet == ESP_ERR_NVS_NO_FREE_PAGES || lRet == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    lRet = nvs_flash_init();
  }
  ESP_ERROR_CHECK(lRet);

  if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
    /* Raise Wi-Fi log verbosity before driver initialization when the build allows it. */
    esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
  }

  ESP_LOGI(gTag, "starting system tasks");
  systemStartTasks();
}