#ifndef __ARCH_ARMLINUX_H
#define __ARCH_ARMLINUX_H

#include <asm/memory.h>

#if defined CONFIG_ARM_LINUX
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

struct image_data;

void start_linux(void *adr, int swap, unsigned long initrd_address,
		unsigned long initrd_size, void *oftree);

#endif /* __ARCH_ARMLINUX_H */
