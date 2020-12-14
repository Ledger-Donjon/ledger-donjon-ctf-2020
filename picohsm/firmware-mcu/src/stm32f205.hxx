/**
 * This file is part of picoHSM
 * 
 * picoHSM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2020 Ledger SAS, written by Olivier Hériveaux
 */

#ifndef _STM32F205_HXX_
#define _STM32F205_HXX_


#include <stdint.h>


/**
 * Registers for Reset and Clock Control of STM32F205
 */
struct rcc_regs_t {
    /** Clock Control Register */
    uint32_t cr;
    /** PLL Configuration Register */
    uint32_t pllcfgr;
    /** Clock Configuration Register */
    uint32_t cfgr;
    /** Clock Interrupt Register */
    uint32_t cir;
    /** AHB1 Peripheral Reset Register */
    uint32_t ahb1rstr;
    /** AHB2 Peripheral Reset Register */
    uint32_t ahb2rstr;
    /** AHB3 Peripheral Reset Register */
    uint32_t ahb3rstr;
    uint32_t reserved_1c;
    /** APB1 Peripheral Reset Register */
    uint32_t apb1rstr;
    /** APB2 Peripheral Reset Register */
    uint32_t apb2rstr;
    uint32_t reserved_28;
    uint32_t reserved_2c;
    /** AHB1 Peripheral Clock Enable Register */
    uint32_t ahb1enr;
    /** AHB2 Peripheral Clock Enable Register */
    uint32_t ahb2enr;
    /** AHB3 Peripheral Clock Enable Register */
    uint32_t ahb3enr;
    uint32_t reserved_3c;
    /** APB1 Peripheral Clock Enable Register */
    uint32_t apb1enr;
    /** APB2 Peripheral Clock Enable Register */
    uint32_t apb2enr;
    uint32_t reserved_48;
    uint32_t reserved_4c;
    /** AHB1 Peripheral Clock Enable in Low Power Mode Register */
    uint32_t ahb1lpenr;
    /** AHB2 Peripheral Clock Enable in Low Power Mode Register */
    uint32_t ahb2lpenr;
    /** AHB3 Peripheral Clock Enable in Low Power Mode Register */
    uint32_t ahb3lpenr;
    uint32_t reserved_5c;
    /** APB1 Peripheral Clock Enable in Low Power Mode Register */
    uint32_t apb1lpenr;
    /** APB2 Peripheral Clock Enable in Low Power Mode Register */
    uint32_t apb2lpenr;
    uint32_t reserved_68;
    uint32_t reserved_6c;
    /** Backup Domain Control Register */
    uint32_t bdcr;
    /** Clock Control & Status Register */
    uint32_t csr;
    uint32_t reserved_78;
    uint32_t reserved_7c;
    /** Spread Spectrum Clock Generation Register */
    uint32_t sscgr;
    /** PLLI2S Configuration Register */
    uint32_t plli2scfgr;
};


/**
 * Registers of a GPIO peripheral for STM32F205
 */
struct gpio_regs_t {
    /** Port Mode Register */
    uint32_t moder;
    /** Port Output Type Register */
    uint32_t otyper;
    /** Port Output Speed Register */
    uint32_t ospeedr;
    /** Pull-up / Pull-down Register */
    uint32_t pupdr;
    /** Input Data Register */
    uint32_t idr;
    /** Output Data Register */
    uint32_t odr;
    /** Bit Set / Register Reset Register */
    uint32_t bsrr;
    /** Port Configuration Lock Register */
    uint32_t lckr;
    /** Alternate Function Register Low */
    uint32_t afrl;
    /** Alternate Function Register High */
    uint32_t afrh;
};


struct gpio_mode_t {
    enum value_t {
        input = 0,
        general_purpose_output = 1,
        alternate_function = 2,
        analog = 3
    };
};


struct gpio_speed_t {
    enum value_t {
        low = 0,
        medium = 1,
        high = 2,
        very_high = 3
    };
};


/**
 * Registers for the SPI peripheral of STM32F205
 */
struct spi_regs_t {
    /** Control Register 1 */
    uint32_t cr1;
    /** Control Register 2 */
    uint32_t cr2;
    /** Status Register */
    uint32_t sr;
    /** Data Register */
    uint32_t dr;
    /** CRC Polynomial Register */
    uint32_t crcpr;
    /** RX CRC Register */
    uint32_t rxcrcr;
    /** TX CRC Register */
    uint32_t txcrcr;
    /** I²S Configuration Register */
    uint32_t i2scfgr;
    /** I²S Prescaler Register */
    uint32_t i2spr;
};


/**
 * Registers for the USART peripherals of STM32F205
 */
struct usart_regs_t {
    /** Status Register */
    uint32_t sr;
    /** Data Register */
    uint32_t dr;
    /** Baud rate Register */
    uint32_t brr;
    /** Control Register 1 */
    uint32_t cr1;
    /** Control Register 2 */
    uint32_t cr2;
    /** Control Register 3 */
    uint32_t cr3;
    /** Guard Time and Prescaler Register */
    uint32_t gtpr;
};


/**
 * Registers for the IWDG (Independant watchdog) fo STM32F205
 */
struct iwdg_regs_t {
    /** Key register */
    uint32_t kr;
    /** Prescaler register */
    uint32_t pr;
    /** Reload register */
    uint32_t rlr;
    /** Status register */
    uint32_t sr;
};


/**
 * Registers for the WWDG (Window watchdog) of STM32F205
 */
struct wwdg_regs_t {
    /** Control Register */
    uint32_t cr;
    /** Configuraton Register */
    uint32_t cfr;
    /** Status Register */
    uint32_t sr;
};


/**
 * Registers for the RNG of STM32F205
 */
struct rng_regs_t {
    /** Control Register */
    uint32_t cr;
    /** Status Register */
    uint32_t sr;
    /** Data Register */
    uint32_t dr;
};


struct dwt_regs_t {
    /** Control Register */
    uint32_t ctrl;
    /** Cycle Count Register */
    uint32_t cyccnt;
    /** CPI Count Register */
    uint32_t cpicnt;
    /** Exception Overhead Count Register */
    uint32_t exccnt;
    /** Sleep Count Register */
    uint32_t sleepcnt;
    /** LSU Count Register */
    uint32_t lsucnt;
    /** Folder-instruction Count Register */
    uint32_t foldcnt;
    /** Program Counter Sample Register */
    uint32_t pcsr;
    /** Comparator Register 0 */
    uint32_t comp0;
    /** Mask Register 0 */
    uint32_t mask0;
    /** Function Register 0 */
    uint32_t function0;
    uint32_t unused0;
    /** Comparator Register 1 */
    uint32_t comp1;
    /** Mask Register 1 */
    uint32_t mask1;
    /** Function Register 1 */
    uint32_t function1;
    uint32_t unused1;
    /** Comparator Register 2 */
    uint32_t comp2;
    /** Mask Register 2 */
    uint32_t mask2;
    /** Function Register 2 */
    uint32_t function2;
    uint32_t unused2;
    /** Comparator Register 3 */
    uint32_t comp3;
    /** Mask Register 3 */
    uint32_t mask3;
    /** Function Register 3 */
    uint32_t function3;
};


#define rcc (*((volatile rcc_regs_t*)0x40023800))
#define gpioa (*((volatile gpio_regs_t*)0x40020000))
#define gpiob (*((volatile gpio_regs_t*)0x40020400))
#define gpioc (*((volatile gpio_regs_t*)0x40020800))
#define spi1 (*((volatile spi_regs_t*)0x40013000))
#define usart1 (*((volatile usart_regs_t*)0x40011000))
#define usart2 (*((volatile usart_regs_t*)0x40004400))
#define iwdg (*((volatile iwdg_regs_t*)0x40003000))
#define wwdg (*((volatile wwdg_regs_t*)0x40002c00))
#define rng (*((volatile rng_regs_t*)0x50060800))

#define scb_icsr (*((volatile uint32_t*)0xe000ed04))
#define scb_vtor (*((volatile uint32_t*)0xe000ed08))
#define scb_aircr (*((volatile uint32_t*)0xe000ed0c))
#define scb_demcr (*((volatile uint32_t*)0xe000edfc))
#define dwt (*((volatile dwt_regs_t*)0xe0001000))

#define flash_acr (*((volatile uint32_t*)0x40023c00))

#define nvic_iser ((volatile uint32_t*)0xe000e100)
#define nvic_icer ((volatile uint32_t*)0xe000e180)
#define nvic_ispr ((volatile uint32_t*)0xe000e200)
#define nvic_icpr ((volatile uint32_t*)0xe000e280)
#define nvic_iabr ((volatile uint32_t*)0xe000e300)
#define nvic_ipr ((volatile uint32_t*)0xe000e400)
#define nvic_stic (*((volatile uint32_t*)0xe000ef00))


#endif
