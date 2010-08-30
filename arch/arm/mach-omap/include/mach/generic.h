
/* I2C controller revisions */
#define OMAP_I2C_REV_2			0x20

/* I2C controller revisions present on specific hardware */
#define OMAP_I2C_REV_ON_2430		0x36
#define OMAP_I2C_REV_ON_3430		0x3C

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
