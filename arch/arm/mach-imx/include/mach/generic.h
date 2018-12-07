#ifndef __MACH_GENERIC_H
#define __MACH_GENERIC_H

#include <linux/compiler.h>
#include <linux/types.h>
#include <bootsource.h>
#include <mach/imx_cpu_types.h>

u64 imx_uid(void);

void imx25_boot_save_loc(void);
void imx35_boot_save_loc(void);
void imx27_boot_save_loc(void);
void imx51_boot_save_loc(void);
void imx53_boot_save_loc(void);
void imx6_boot_save_loc(void);
void imx7_boot_save_loc(void);
void vf610_boot_save_loc(void);
void imx8_boot_save_loc(void);

void imx25_get_boot_source(enum bootsource *src, int *instance);
void imx27_get_boot_source(enum bootsource *src, int *instance);
void imx35_get_boot_source(enum bootsource *src, int *instance);
void imx51_get_boot_source(enum bootsource *src, int *instance);
void imx53_get_boot_source(enum bootsource *src, int *instance);
void imx6_get_boot_source(enum bootsource *src, int *instance);
void imx7_get_boot_source(enum bootsource *src, int *instance);
void vf610_get_boot_source(enum bootsource *src, int *instance);
void imx8_get_boot_source(enum bootsource *src, int *instance);

int imx1_init(void);
int imx21_init(void);
int imx25_init(void);
int imx27_init(void);
int imx31_init(void);
int imx35_init(void);
int imx50_init(void);
int imx51_init(void);
int imx53_init(void);
int imx6_init(void);
int imx7_init(void);
int vf610_init(void);
int imx8mq_init(void);

int imx1_devices_init(void);
int imx21_devices_init(void);
int imx25_devices_init(void);
int imx27_devices_init(void);
int imx31_devices_init(void);
int imx35_devices_init(void);
int imx50_devices_init(void);
int imx51_devices_init(void);
int imx53_devices_init(void);
int imx6_devices_init(void);

void imx5_cpu_lowlevel_init(void);
void imx6_cpu_lowlevel_init(void);
void imx6ul_cpu_lowlevel_init(void);
void imx7_cpu_lowlevel_init(void);
void vf610_cpu_lowlevel_init(void);

/* There's a off-by-one betweem the gpio bank number and the gpiochip */
/* range e.g. GPIO_1_5 is gpio 5 under linux */
#define IMX_GPIO_NR(bank, nr)		(((bank) - 1) * 32 + (nr))

extern unsigned int __imx_cpu_type;

#ifdef CONFIG_ARCH_IMX1
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX1
# endif
# define cpu_is_mx1()		(imx_cpu_type == IMX_CPU_IMX1)
#else
# define cpu_is_mx1()		(0)
#endif

#ifdef CONFIG_ARCH_IMX21
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX21
# endif
# define cpu_is_mx21()		(imx_cpu_type == IMX_CPU_IMX21)
#else
# define cpu_is_mx21()		(0)
#endif

#ifdef CONFIG_ARCH_IMX25
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX25
# endif
# define cpu_is_mx25()		(imx_cpu_type == IMX_CPU_IMX25)
#else
# define cpu_is_mx25()		(0)
#endif

#ifdef CONFIG_ARCH_IMX27
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX27
# endif
# define cpu_is_mx27()		(imx_cpu_type == IMX_CPU_IMX27)
#else
# define cpu_is_mx27()		(0)
#endif

#ifdef CONFIG_ARCH_IMX31
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX31
# endif
# define cpu_is_mx31()		(imx_cpu_type == IMX_CPU_IMX31)
#else
# define cpu_is_mx31()		(0)
#endif

#ifdef CONFIG_ARCH_IMX35
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX35
# endif
# define cpu_is_mx35()		(imx_cpu_type == IMX_CPU_IMX35)
#else
# define cpu_is_mx35()		(0)
#endif

#ifdef CONFIG_ARCH_IMX50
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX50
# endif
# define cpu_is_mx50()		(imx_cpu_type == IMX_CPU_IMX50)
#else
# define cpu_is_mx50()		(0)
#endif


#ifdef CONFIG_ARCH_IMX51
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX51
# endif
# define cpu_is_mx51()		(imx_cpu_type == IMX_CPU_IMX51)
#else
# define cpu_is_mx51()		(0)
#endif

#ifdef CONFIG_ARCH_IMX53
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX53
# endif
# define cpu_is_mx53()		(imx_cpu_type == IMX_CPU_IMX53)
#else
# define cpu_is_mx53()		(0)
#endif

#ifdef CONFIG_ARCH_IMX6
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX6
# endif
# define cpu_is_mx6()		(imx_cpu_type == IMX_CPU_IMX6)
#else
# define cpu_is_mx6()		(0)
#endif

#ifdef CONFIG_ARCH_IMX7
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX7
# endif
# define cpu_is_mx7()		(imx_cpu_type == IMX_CPU_IMX7)
#else
# define cpu_is_mx7()		(0)
#endif

#ifdef CONFIG_ARCH_IMX8MQ
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_IMX8MQ
# endif
# define cpu_is_mx8mq()	(imx_cpu_type == IMX_CPU_IMX8MQ)
#else
# define cpu_is_mx8mq()	(0)
#endif

#ifdef CONFIG_ARCH_VF610
# ifdef imx_cpu_type
#  undef imx_cpu_type
#  define imx_cpu_type __imx_cpu_type
# else
#  define imx_cpu_type IMX_CPU_VF610
# endif
# define cpu_is_vf610()		(imx_cpu_type == IMX_CPU_VF610)
#else
# define cpu_is_vf610()		(0)
#endif

#define cpu_is_mx23()	(0)
#define cpu_is_mx28()	(0)

#endif /* __MACH_GENERIC_H */
