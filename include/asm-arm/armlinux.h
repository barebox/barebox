#ifndef __ARCH_ARMLINUX_H
#define __ARCH_ARMLINUX_H

#if defined CONFIG_CMD_BOOTM || defined CONFIG_CMD_BOOTZ || \
	defined CONFIG_CMD_BOOTU
void armlinux_set_bootparams(void *params);
void armlinux_set_architecture(int architecture);
void armlinux_add_dram(struct device_d *dev);
#else
static inline void armlinux_set_bootparams(void *params)
{
}

static inline void armlinux_set_architecture(int architecture)
{
}

static inline void armlinux_add_dram(struct device_d *dev)
{
}
#endif

#endif /* __ARCH_ARMLINUX_H */
