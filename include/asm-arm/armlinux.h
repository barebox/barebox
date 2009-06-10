#ifndef __ARCH_ARMLINUX_H
#define __ARCH_ARMLINUX_H

#ifdef CONFIG_CMD_BOOTM
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
