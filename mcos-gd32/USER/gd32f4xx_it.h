/*!
    \file  gd32f4xx_it.h
    \brief the header file of the ISR
*/

/*
    Copyright (C) 2016 GigaDevice

    2016-10-19, V1.0.0, demo for GD32F4xx
*/

#ifndef GD32F4XX_IT_H
#define GD32F4XX_IT_H



/* function declarations */
/* NMI handle function */
void NMI_Handler(void);
/* HardFault handle function */
void HardFault_Handler(void);
/* MemManage handle function */
void MemManage_Handler(void);
/* BusFault handle function */
void BusFault_Handler(void);
/* UsageFault handle function */
void UsageFault_Handler(void);
/* SVC handle function */
void SVC_Handler(void);
/* DebugMon handle function */
void DebugMon_Handler(void);
/* PendSV handle function */
void PendSV_Handler(void);
/* SysTick handle function */
void SysTick_Handler(void);
/* this function handles EXTI5_9_IRQ Handler */
void EXTI5_9_IRQHandler(void);
/* this function handles USB wakeup interrupt handler */
void USBHS_WKUP_IRQHandler(void);
/* this function handles USBHS IRQ Handler */
void USBHS_IRQHandler(void);

#ifdef USBHS_DEDICATED_EP1_ENABLED

/* dedicated IN endpoint1 ISR handler */
void USBHS_EP1_In_IRQHandler(void);
/* dedicated OUT endpoint1 ISR handler */
void USBHS_EP1_Out_IRQHandler(void);

#endif /* USBHS_DEDICATED_EP1_ENABLED */

#endif /* GD32F4XX_IT_H */

