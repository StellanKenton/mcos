/* Minimal FreeRTOS entry: keep only DefaultTask and SystemTask. */

#include "FreeRTOS.h"
#include "task.h"
#include "gd32f4xx_gpio.h"
#include "sys_int.h"

static TaskHandle_t DefaultTaskHandle = NULL;
static TaskHandle_t SystemTaskHandle = NULL;

static void DefaultTask(void *parameter)
{
	(void)parameter;

	for(;;)
	{
		gpio_bit_toggle(STATUS_LED_GPIO_PORT, STATUS_LED_PIN);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

static void SystemTask(void *parameter)
{
	(void)parameter;

	for(;;)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

int main(void)
{
	BaseType_t xReturn;

	BSP_Init();

	xReturn = xTaskCreate(DefaultTask,
		"DefaultTask",
		configMINIMAL_STACK_SIZE,
		NULL,
		tskIDLE_PRIORITY + 1U,
		&DefaultTaskHandle);
	if(pdPASS != xReturn)
	{
		return -1;
	}

	xReturn = xTaskCreate(SystemTask,
		"SystemTask",
		configMINIMAL_STACK_SIZE,
		NULL,
		tskIDLE_PRIORITY + 2U,
		&SystemTaskHandle);
	if(pdPASS != xReturn)
	{
		return -1;
	}

	vTaskStartScheduler();

	for(;;)
	{
	}
}
/********************************END OF FILE****************************/
