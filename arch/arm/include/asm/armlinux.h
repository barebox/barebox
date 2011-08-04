#ifndef __ARCH_ARMLINUX_H
#define __ARCH_ARMLINUX_H

#include <asm/memory.h>

#if defined CONFIG_CMD_BOOTM || defined CONFIG_CMD_BOOTZ || \
	defined CONFIG_CMD_BOOTU
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

void start_linux(void *adr, int swap, struct image_data *data);

struct device_d *arm_add_mem_device(const char* name, resource_size_t start,
				    resource_size_t size);

#endif /* __ARCH_ARMLINUX_H */
