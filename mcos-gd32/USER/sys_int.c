#include "gd32f4xx.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_misc.h"
#include "gd32f4xx_rcu.h"
#include "sys_int.h"

static void status_led_init(void)
{
	rcu_periph_clock_enable(RCU_GPIOE);
	gpio_mode_set(STATUS_LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STATUS_LED_PIN);
	gpio_output_options_set(STATUS_LED_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, STATUS_LED_PIN);
	gpio_bit_reset(STATUS_LED_GPIO_PORT, STATUS_LED_PIN);
}

/***********************************************************************
  * @ ������  �� BSP_Init
  * @ ����˵���� �弶�����ʼ�������а����ϵĳ�ʼ�����ɷ��������������
  * @ ����    ��   
  * @ ����ֵ  �� ��
  *********************************************************************/
void BSP_Init(void)
{
	nvic_vector_table_set(NVIC_VECTTAB_FLASH, 0x0);
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	status_led_init();
}

