.syntax unified
.section .vectors
.thumb
    .org 0
    /* Stack top is declared in linker script */
    .long stack_top
vectors:
    .long mystart+1 // Reset
    .long empty_irq+1 // NMI
    .long empty_irq+1 // HardFault
    .long empty_irq+1 // MemManage
    .long empty_irq+1 // BusFault
    .long empty_irq+1 // UsageFault
    .long empty_irq+1 // Reserved 0x1C
    .long empty_irq+1 // Reserved 0x20
    .long empty_irq+1 // Reserved 0x24
    .long empty_irq+1 // Reserved 0x28
    .long empty_irq+1 // SVCall
    .long empty_irq+1 // Debug Monitor
    .long empty_irq+1 // Reserved 0x34
    .long empty_irq+1 // PendSV
    .long empty_irq+1 // SysTick
    .long empty_irq+1 // WWDG
    .long empty_irq+1 // PVD
    .long empty_irq+1 // TAMP_STAMP
    .long empty_irq+1 // RTC_WKUP
    .long empty_irq+1 // FLASH
    .long empty_irq+1 // RCC
    .long empty_irq+1 // EXTI0
    .long empty_irq+1 // EXTI1
    .long empty_irq+1 // EXTI2
    .long empty_irq+1 // EXTI3
    .long empty_irq+1 // EXTI4
    .long empty_irq+1 // DMA1_Stream0
    .long empty_irq+1 // DMA1_Stream1
    .long empty_irq+1 // DMA1_Stream2
    .long empty_irq+1 // DMA1_Stream3
    .long empty_irq+1 // DMA1_Stream4
    .long empty_irq+1 // DMA1_Stream5
    .long empty_irq+1 // DMA1_Stream6
    .long empty_irq+1 // ADC
    .long empty_irq+1 // CAN1_TX
    .long empty_irq+1 // CAN1_RX0
    .long empty_irq+1 // CAN1_RX1
    .long empty_irq+1 // CAN1_SCE
    .long empty_irq+1 // EXTI9_5
    .long empty_irq+1 // TIM1_BRK_TIM9
    .long empty_irq+1 // TIM1_UP_TIM10
    .long empty_irq+1 // TIM1_TRG_COM_TIM11
    .long empty_irq+1 // TIM1_CC
    .long empty_irq+1 // TIM2
    .long empty_irq+1 // TIM3
    .long empty_irq+1 // TIM4
    .long empty_irq+1 // I2C1_EV
    .long empty_irq+1 // I2C1_ER
    .long empty_irq+1 // I2C2_EV
    .long empty_irq+1 // I2C2_ER
    .long empty_irq+1 // SPI1
    .long empty_irq+1 // SPI2
    .long usart1_handler+1 // USART1
    .long usart2_handler+1 // USART2
    .long empty_irq+1 // USART3
    .long empty_irq+1 // EXTI15_10
    .long empty_irq+1 // RTC_Alarm
    .long empty_irq+1 // OTG_FS_WKUP
    .long empty_irq+1 // TIM8_BRK_TIM12
    .long empty_irq+1 // TIM8_UP_TIM13
    .long empty_irq+1 // TIM8_TRG_COM_TIM14
    .long empty_irq+1 // TIM8_CC
    .long empty_irq+1 // DMA1_Stream7
    .long empty_irq+1 // FSMC
    .long empty_irq+1 // SDIO
    .long empty_irq+1 // TIM5
    .long empty_irq+1 // SPI3
    .long empty_irq+1 // UART4
    .long empty_irq+1 // UART5
    .long empty_irq+1 // TIM6_DAC
    .long empty_irq+1 // TIM7
    .long empty_irq+1 // DMA2_Stream0
    .long empty_irq+1 // DMA2_Stream1
    .long empty_irq+1 // DMA2_Stream2
    .long empty_irq+1 // DMA2_Stream3
    .long empty_irq+1 // DMA2_Stream4
    .long empty_irq+1 // ETH
    .long empty_irq+1 // ETH_WKUP
    .long empty_irq+1 // CAN2_TX
    .long empty_irq+1 // CAN2_RX0
    .long empty_irq+1 // CAN2_RX1

empty_irq:
    mov pc, lr

mystart:
    b main

usart1_irq:
    b usart1_handler

usart2_irq:
    b usart2_handler
