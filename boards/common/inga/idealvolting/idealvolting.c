/*
 * Copyright (C) 2017 Rasmus Antons <r.antons@tu-bs.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     idealvolting
 * @{
 *
 * @file
 * @brief       Idealvolting implementation.
 *
 * @author      Rasmus Antons <r.antons@tu-bs.de>
 *
 * @}
 */

#include "idealvolting.h"
#include "idealvolting_config.h"
#include "idealvolting_frame.h"
#include "alu_check.h"
#include "temp.h"
#include "xtimer.h"
#include "thread.h"
#include "periph/i2c.h"
#include <stdio.h>
#include <assert.h>
#include <mutex.h>

#define IV_THREAD_PRIORITY 0
#define IV_THREAD_FLAGS THREAD_CREATE_STACKTEST

static char iv_thread_stack[THREAD_STACKSIZE_MAIN];
static mutex_t iv_mutex, iv_disable;
static struct {
	uint8_t is_running;
	uint8_t debug;
	uint8_t vreg;
	uint8_t osccal;
	uint8_t temp;
	uint8_t table;
} iv_state;
uint8_t swr_detection = 0;

uint8_t check_reset(void)
{
	if (MCUSR & 0b00000001) { //Power On
		MCUSR &= 0b11111110;
		return 0;
	} else if (MCUSR & 0b00000010) { //HW Reset
		MCUSR &= 0b11111101;
		return 1;
	} else if (swr_detection) { //SW Reset
		return 2;
	}
	return 0;
}

void wait_si_ready(void)
{
	uint8_t si_state;
	do {
		i2c_acquire(IV_I2C_DEV);
		i2c_read_reg(IV_I2C_DEV, SI_I2C_ADDR,
				SI_REG_LOCK, &si_state);
		i2c_release(IV_I2C_DEV);
		xtimer_usleep(100);
	} while (si_state != SI_READY);
}

void send_si_req(iv_req_t *req, iv_res_t *res)
{
	i2c_acquire(IV_I2C_DEV);
	i2c_write_regs(IV_I2C_DEV, SI_I2C_ADDR,
			SI_REG_REQUEST, req, sizeof(*req));
	i2c_release(IV_I2C_DEV);
	wait_si_ready();
	i2c_acquire(IV_I2C_DEV);
	i2c_read_regs(IV_I2C_DEV, SI_I2C_ADDR,
			SI_REG_REPLY, res, sizeof(*res));
	i2c_release(IV_I2C_DEV);
}

void prepare_si_req(iv_req_t *req) {
	req->checksum = MCU_CHECK();
	req->temperature = get_temp();
	iv_state.temp = req->temperature;
	req->osccal = OSCCAL;
	req->alt_byte ^= 1;
}

void *iv_thread(void *arg)
{
	(void) arg;
	static vscale_t vscale_dev;
	iv_req_t req;
	iv_res_t res;


	setup_temp();

	mutex_init(&iv_mutex);
	mutex_lock(&iv_mutex);
	req.alt_byte = 0;
	req.rst_flags = check_reset();
	swr_detection = 1;
	req.rst_disable = 0x00;
	VSCALE_INIT(&vscale_dev);
	wait_si_ready();
	mutex_unlock(&iv_mutex);

	xtimer_ticks32_t last_wakeup = xtimer_now();
	while (1) {
		xtimer_periodic_wakeup(&last_wakeup, 1000000);
		mutex_lock(&iv_mutex);
		mutex_lock(&iv_disable);
		mutex_unlock(&iv_disable);
		prepare_si_req(&req);
		send_si_req(&req, &res);
		req.rst_flags = 0x00;
		iv_state.vreg = res.voltage;
		iv_state.osccal = res.osccal;
		iv_state.table = (res.debug >> 6) & (3);
		VSCALE_SET_REG(&vscale_dev, res.voltage);
#ifdef BOARD_INGA_BLUE
		assert(res.osccal >= IV_OSCCAL_MIN && res.osccal <= IV_OSCCAL_MAX);
		OSCCAL = res.osccal;
#endif
		mutex_unlock(&iv_mutex);
	}

	return NULL;
}

void idealvolting_init(void)
{
	iv_state.is_running = 0;
	iv_state.debug = 0;

	i2c_init_master(IV_I2C_DEV, SI_I2C_SPEED);

	thread_create(iv_thread_stack, sizeof(iv_thread_stack),
			IV_THREAD_PRIORITY, IV_THREAD_FLAGS,
			iv_thread, NULL, "idealvolting");

	iv_state.is_running = 1;
}

void idealvolting_enable(void)
{
	mutex_lock(&iv_mutex);
	mutex_unlock(&iv_disable);
	iv_state.is_running = 1;
	mutex_unlock(&iv_mutex);
}

void idealvolting_disable(void)
{
	mutex_lock(&iv_mutex);
	mutex_unlock(&iv_disable);
	iv_state.is_running = 0;
	mutex_unlock(&iv_mutex);
}

void idealvolting_set_debug(uint8_t state)
{
	mutex_lock(&iv_mutex);
	iv_state.debug = state;
	mutex_unlock(&iv_mutex);
}

void idealvolting_print_status(void)
{
	mutex_lock(&iv_mutex);
	puts("--------IdealVolting status--------");
	printf("is_running  = %s\n", iv_state.is_running ? "true" : "false");
	printf("Vreg        = %d\n", iv_state.vreg);
	printf("OSCCAL      = %d\n", iv_state.osccal);
	printf("Temperature = %d\n", iv_state.temp);
	printf("Table used  = %s\n", iv_state.table ? "true" : "false");
	mutex_unlock(&iv_mutex);
}