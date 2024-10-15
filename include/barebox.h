/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __BAREBOX_H_
#define __BAREBOX_H_

#include <linux/compiler.h>

/* For use when unrelocated */
static inline void __hang(void)
{
	while (1);
}
void __noreturn hang (void);

/*
 * Function pointer to the main barebox function. Defaults
 * to run_shell() when a shell is enabled.
 */
extern int (*barebox_main)(void);

enum autoboot_state {
	AUTOBOOT_COUNTDOWN,
	AUTOBOOT_ABORT,
	AUTOBOOT_MENU,
	AUTOBOOT_BOOT,
	AUTOBOOT_UNKNOWN,
};

void set_autoboot_state(enum autoboot_state autoboot);
enum autoboot_state do_autoboot_countdown(void);

void __noreturn start_barebox(void);
void shutdown_barebox(void);

#endif
