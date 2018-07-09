/*
 * Copyright (C) 2014 Freie Universität Berlin, Hinnerk van Bruinehsen
 *               2018 Matthew Blue
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_atmega1284p
 * @{
 *
 * @file
 * @brief       Implementation of the CPU initialization
 *
 * @author      Hinnerk van Bruinehsen <h.v.bruinehsen@fu-berlin.de>
 * @author      Matthew Blue <matthew.blue.neuro@gmail.com>
 * @}
 */

#include "cpu.h"
#include "periph/init.h"
#include <avr/wdt.h>

#if (__AVR_LIBC_VERSION__ >= 10700UL)
/* The proper way to set the signature is */
#include <avr/signature.h>
#else

/* signature API not available before avr-lib-1.7.0. Do it manually.*/
typedef struct {
    const unsigned char B2;
    const unsigned char B1;
    const unsigned char B0;
} __signature_t;
#define SIGNATURE __signature_t __signature __attribute__((section(".signature")))
SIGNATURE = {
    .B2 = 0x05, //SIGNATURE_2, //ATMEGA1284p
    .B1 = 0x97, //SIGNATURE_1, //128KB flash
    .B0 = 0x1E, //SIGNATURE_0, //Atmel
};
#endif /* (__AVR_LIBC_VERSION__ >= 10700UL) */

/**
 * @brief Initialize the CPU, set IRQ priorities
 */
void cpu_init(void)
{
  	/* Set WDT_Reset flag zero and disable wdt. Or else, the watchdog wont stop resetting the MCU after reboot was called.*/
	MCUSR = 0;
	wdt_disable();
	  power_all_disable();
	  /*
	  power_spi_disable();
	  power_usart0_disable();
	  power_usart1_disable();
	  power_twi_disable();
	  power_adc_disable();
	  power_timer0_disable();
	  power_timer1_disable();
	  power_timer2_disable();
	  power_timer3_disable();
	  */

	  periph_init();
}
