#ifndef _MACH_GENERIC_H
#define _MACH_GENERIC_H

/* I2C controller revisions */
#define OMAP_I2C_OMAP1_REV_2            0x20

/* I2C controller revisions present on specific hardware */
#define OMAP_I2C_REV_ON_2430		0x00000036
#define OMAP_I2C_REV_ON_3430_3530       0x0000003C
#define OMAP_I2C_REV_ON_3630            0x00000040
#define OMAP_I2C_REV_ON_4430_PLUS       0x50400002

#ifdef CONFIG_ARCH_OMAP
#define cpu_is_omap2430()	(1)
#else
#define cpu_is_omap2430()	(0)
#endif

#ifdef CONFIG_ARCH_OMAP3
#define cpu_is_omap34xx()	(1)
#else
#define cpu_is_omap34xx()	(0)
#endif

#ifdef CONFIG_ARCH_OMAP4
#define cpu_is_omap4xxx()	(1)
#else
#define cpu_is_omap4xxx()	(0)
#endif

#ifdef CONFIG_ARCH_AM33XX
#define cpu_is_am33xx()		(1)
#else
#define cpu_is_am33xx()		(0)
#endif

struct omap_barebox_part {
	unsigned int nand_offset;
	unsigned int nand_size;
	unsigned int nor_offset;
	unsigned int nor_size;
};

#ifdef CONFIG_SHELL_NONE
int omap_set_barebox_part(struct omap_barebox_part *part);
int omap_set_mmc_dev(const char *mmcdev);
#else
static inline int omap_set_barebox_part(struct omap_barebox_part *part)
{
	return 0;
}
static inline int omap_set_mmc_dev(const char *mmcdev)
{
	return 0;
}
#endif

extern uint32_t omap_bootinfo[3];
void omap_save_bootinfo(void *data);
void __noreturn omap_start_barebox(void *barebox);

void omap_set_bootmmc_devname(const char *devname);
const char *omap_get_bootmmc_devname(void);

#endif
