/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ARCH_ARMLINUX_H
#define __ARCH_ARMLINUX_H

#include <asm/memory.h>
#include <asm/setup.h>
#include <asm/secure.h>
#include <linux/bug.h>

struct tag;

#if defined CONFIG_BOOT_ATAGS
void armlinux_set_bootparams(void *params);
struct tag *armlinux_get_bootparams(void);
void armlinux_set_architecture(unsigned architecture);
unsigned armlinux_get_architecture(void);
void armlinux_set_revision(unsigned int);
void armlinux_set_serial(u64);
void armlinux_setup_tags(unsigned long initrd_address,
			 unsigned long initrd_size, int swap);
#else
static inline void armlinux_set_bootparams(void *params)
{
}

static inline struct tag *armlinux_get_bootparams(void)
{
	BUG();
}

static inline void armlinux_set_architecture(unsigned architecture)
{
}

static inline unsigned armlinux_get_architecture(void)
{
	return 0;
}

static inline void armlinux_set_revision(unsigned int rev)
{
}

static inline void armlinux_set_serial(u64 serial)
{
}

static inline void armlinux_setup_tags(unsigned long initrd_address,
				       unsigned long initrd_size, int swap)
{
}
#endif
#if defined CONFIG_ARM_BOARD_APPEND_ATAG
void armlinux_set_atag_appender(struct tag *(*)(struct tag *));
#else
static inline void armlinux_set_atag_appender(struct tag *(*func)(struct tag *))
{
}
#endif

struct image_data;

void start_linux(void *adr, int swap, unsigned long initrd_address,
		 unsigned long initrd_size, void *oftree,
		 enum arm_security_state, void *optee);

#endif /* __ARCH_ARMLINUX_H */
