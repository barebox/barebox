/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __BAREBOX_H_
#define __BAREBOX_H_

#include <linux/compiler.h>

/* For use when unrelocated */
static inline __noreturn void __hang(void)
{
	while (1);
}
void __noreturn hang (void);

/*
 * Function pointer to the main barebox function. Defaults
 * to run_init() when shell and command support are enabled.
 */
extern int (*barebox_main)(void);

/**
 * enum autoboot_state - autoboot action after init
 * @AUTOBOOT_COUNTDOWN: Count down to automatic boot action
 * @AUTOBOOT_ABORT: Abort boot and drop to barebox shell
 * @AUTOBOOT_HALT: Halt boot and drop to barebox shell, _without_ running
 *                 any interactive hooks (e.g. bringing up network)
 * AUTOBOOT_MENU: Show main menu directly
 * @AUTOBOOT_BOOT: Boot right away without an interruptible countdown
 * @AUTOBOOT_UNKNOWN: Boot right away without an interruptible countdown
 *
 * This enum descibes what action to take after barebox initialization
 * has completed and it's time to boot.
 */
enum autoboot_state {
	AUTOBOOT_COUNTDOWN,
	AUTOBOOT_ABORT,
	AUTOBOOT_HALT,
	AUTOBOOT_MENU,
	AUTOBOOT_BOOT,
	AUTOBOOT_UNKNOWN,
};

void set_autoboot_state(enum autoboot_state autoboot);
enum autoboot_state do_autoboot_countdown(void);

void __noreturn start_barebox(void);
void shutdown_barebox(void);

long get_ram_size(volatile long *base, long size);

enum system_states {
	BAREBOX_STARTING,
	BAREBOX_RUNNING,
	BAREBOX_EXITING,
};

#if IN_PROPER
extern enum system_states barebox_system_state;
#else
#define barebox_system_state	BAREBOX_STARTING
#endif

#endif
