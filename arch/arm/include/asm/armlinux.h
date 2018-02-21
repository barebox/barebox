#ifndef __ARCH_ARMLINUX_H
#define __ARCH_ARMLINUX_H

#include <asm/memory.h>
#include <asm/setup.h>
#include <asm/secure.h>

#if defined CONFIG_ARM_LINUX && defined CONFIG_CPU_32
void armlinux_set_bootparams(void *params);
void armlinux_set_architecture(int architecture);
void armlinux_set_revision(unsigned int);
void armlinux_set_serial(u64);
#else
static inline void armlinux_set_bootparams(void *params)
{
}

static inline void armlinux_set_architecture(int architecture)
{
}

static inline void armlinux_set_revision(unsigned int rev)
{
}

static inline void armlinux_set_serial(u64 serial)
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
		 enum arm_security_state);

#endif /* __ARCH_ARMLINUX_H */
