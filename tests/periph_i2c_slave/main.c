#include "periph/uart.h"
#include "uart_stdio.h"
#include "periph/i2c_slave.h"
#include "msg.h"
#include "periph/pm.h"
#include "thread.h"
#include "board.h"
#include "avr/sleep.h"
#include "periph_cpu.h"

#define MEGA_ADDR 0x2d

kernel_pid_t main_pid;

void rcb(uint8_t n_received, uint8_t *data_received)
{
	(void) n_received;
	(void) data_received;
	msg_t msg;
	msg_send(&msg, main_pid);
}

uint8_t tcb(uint8_t *txbuffer)
{
	sprintf((char *) txbuffer, "ok");
	return 3;
}

int main(void)
{
	msg_t msg;
#ifdef BOARD_REAPER
	LED1_OFF;
	LED2_OFF;
#endif
	main_pid = thread_getpid();
	uart_poweroff(UART_STDIO_DEV);
	i2c_init_slave(MEGA_ADDR, rcb, tcb);

	//pm_block(PM_SLEEPMODE_PWR_DOWN);
	while (1) {
		msg_receive(&msg);
		__builtin_avr_delay_cycles(1000000);
	}

	return 0;
}
