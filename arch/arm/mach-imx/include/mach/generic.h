
u64 imx_uid(void);

enum imx_bootsource {
	bootsource_unknown,
	bootsource_nand,
	bootsource_nor,
	bootsource_mmc,
	bootsource_i2c,
	bootsource_spi,
	bootsource_serial,
	bootsource_onenand,
};

enum imx_bootsource imx_bootsource(void);
void imx_set_bootsource(enum imx_bootsource src);

int imx_25_35_boot_save_loc(unsigned int ctrl, unsigned int type);
void imx_27_boot_save_loc(void __iomem *sysctrl_base);
int imx51_boot_save_loc(void __iomem *src_base);

#ifdef CONFIG_ARCH_IMX1
#define cpu_is_mx1()	(1)
#else
#define cpu_is_mx1()	(0)
#endif

#ifdef CONFIG_ARCH_IMX21
#define cpu_is_mx21()	(1)
#else
#define cpu_is_mx21()	(0)
#endif

#ifdef CONFIG_ARCH_IMX25
#define cpu_is_mx25()	(1)
#else
#define cpu_is_mx25()	(0)
#endif

#ifdef CONFIG_ARCH_IMX27
#define cpu_is_mx27()	(1)
#else
#define cpu_is_mx27()	(0)
#endif

#ifdef CONFIG_ARCH_IMX31
#define cpu_is_mx31()	(1)
#else
#define cpu_is_mx31()	(0)
#endif

#ifdef CONFIG_ARCH_IMX35
#define cpu_is_mx35()	(1)
#else
#define cpu_is_mx35()	(0)
#endif

#ifdef CONFIG_ARCH_IMX51
#define cpu_is_mx51()	(1)
#else
#define cpu_is_mx51()	(0)
#endif

#ifdef CONFIG_ARCH_IMX53
#define cpu_is_mx53()	(1)
#else
#define cpu_is_mx53()	(0)
#endif

#ifdef CONFIG_ARCH_IMX6
#define cpu_is_mx6()	(1)
#else
#define cpu_is_mx6()	(0)
#endif

#define cpu_is_mx23()	(0)
#define cpu_is_mx28()	(0)
