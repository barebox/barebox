/*
 * Machine type definitions for legacy platforms.
 */

#ifndef __ASM_ARM_MACH_TYPE_H
#define __ASM_ARM_MACH_TYPE_H

#ifndef __ASSEMBLY__
/* The type of machine we're running on */
extern unsigned int __machine_arch_type;
#endif

/* see arch/arm/kernel/arch.c for a description of these */
#define MACH_TYPE_LUBBOCK              89
#define MACH_TYPE_VERSATILE_PB         387
#define MACH_TYPE_CSB337               399
#define MACH_TYPE_MAINSTONE            406
#define MACH_TYPE_NOMADIK              420
#define MACH_TYPE_EDB9312              451
#define MACH_TYPE_EDB9301              462
#define MACH_TYPE_EDB9315              463
#define MACH_TYPE_SCB9328              508
#define MACH_TYPE_EDB9302              538
#define MACH_TYPE_EDB9307              607
#define MACH_TYPE_AT91RM9200EK         705
#define MACH_TYPE_PCM027               732
#define MACH_TYPE_EDB9315A             772
#define MACH_TYPE_AT91SAM9261EK        848
#define MACH_TYPE_AT91SAM9260EK        1099
#define MACH_TYPE_EDB9302A             1127
#define MACH_TYPE_EDB9307A             1128
#define MACH_TYPE_PM9261               1187
#define MACH_TYPE_AT91SAM9263EK        1202
#define MACH_TYPE_ZYLONITE             1233
#define MACH_TYPE_MIOA701              1257
#define MACH_TYPE_PM9263               1475
#define MACH_TYPE_OMAP3EVM             1535
#define MACH_TYPE_OMAP3_BEAGLE         1546
#define MACH_TYPE_AT91SAM9G20EK        1624
#define MACH_TYPE_USB_A9260            1709
#define MACH_TYPE_USB_A9263            1710
#define MACH_TYPE_QIL_A9260            1711
#define MACH_TYPE_PICOCOM1             1751
#define MACH_TYPE_AT91SAM9M10G45EK     1830
#define MACH_TYPE_USB_A9G20            1841
#define MACH_TYPE_QIL_A9G20            1844
#define MACH_TYPE_CHUMBY               1937
#define MACH_TYPE_TNY_A9260            2058
#define MACH_TYPE_TNY_A9G20            2059
#define MACH_TYPE_MX51_BABBAGE         2125
#define MACH_TYPE_TNY_A9263            2140
#define MACH_TYPE_AT91SAM9G10EK        2159
#define MACH_TYPE_TX25                 2177
#define MACH_TYPE_MX23EVK              2629
#define MACH_TYPE_PM9G45               2672
#define MACH_TYPE_OMAP4_PANDA          2791
#define MACH_TYPE_PCAAL1               2843
#define MACH_TYPE_ARMADA_XP_DB         3036
#define MACH_TYPE_TX28                 3043
#define MACH_TYPE_BCM2708              3138
#define MACH_TYPE_MX53_LOCO            3273
#define MACH_TYPE_TX53                 3279
#define MACH_TYPE_CCMX53               3346
#define MACH_TYPE_CCWMX53              3348
#define MACH_TYPE_VMX53                3359
#define MACH_TYPE_PCM049               3364
#define MACH_TYPE_DSS11                3787
#define MACH_TYPE_BEAGLEBONE           3808
#define MACH_TYPE_PCAAXL2              3912
#define MACH_TYPE_MX6Q_SABRESD         3980
#define MACH_TYPE_TQMA53               4004
#define MACH_TYPE_IMX233_OLINUXINO     4105
#define MACH_TYPE_CFA10036             4142
#define MACH_TYPE_PCM051               4144
#define MACH_TYPE_HABA_KNX_LITE        4310
#define MACH_TYPE_VAR_SOM_MX6          4419
#define MACH_TYPE_PCAAXS1              4526
#define MACH_TYPE_PFLA03               4575

#ifdef CONFIG_ARCH_VERSATILE_PB
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_VERSATILE_PB
# endif
# define machine_is_versatile_pb()	(machine_arch_type == MACH_TYPE_VERSATILE_PB)
#else
# define machine_is_versatile_pb()	(0)
#endif

#ifdef CONFIG_MACH_CSB337
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_CSB337
# endif
# define machine_is_csb337()	(machine_arch_type == MACH_TYPE_CSB337)
#else
# define machine_is_csb337()	(0)
#endif

#ifdef CONFIG_MACH_SCB9328
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_SCB9328
# endif
# define machine_is_scb9328()	(machine_arch_type == MACH_TYPE_SCB9328)
#else
# define machine_is_scb9328()	(0)
#endif

#ifdef CONFIG_MACH_AT91RM9200EK
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_AT91RM9200EK
# endif
# define machine_is_at91rm9200ek()	(machine_arch_type == MACH_TYPE_AT91RM9200EK)
#else
# define machine_is_at91rm9200ek()	(0)
#endif

#ifdef CONFIG_MACH_AT91SAM9261EK
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_AT91SAM9261EK
# endif
# define machine_is_at91sam9261ek()	(machine_arch_type == MACH_TYPE_AT91SAM9261EK)
#else
# define machine_is_at91sam9261ek()	(0)
#endif

#ifdef CONFIG_MACH_AT91SAM9260EK
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_AT91SAM9260EK
# endif
# define machine_is_at91sam9260ek()	(machine_arch_type == MACH_TYPE_AT91SAM9260EK)
#else
# define machine_is_at91sam9260ek()	(0)
#endif

#ifdef CONFIG_MACH_PM9261
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_PM9261
# endif
# define machine_is_pm9261()	(machine_arch_type == MACH_TYPE_PM9261)
#else
# define machine_is_pm9261()	(0)
#endif

#ifdef CONFIG_MACH_AT91SAM9263EK
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_AT91SAM9263EK
# endif
# define machine_is_at91sam9263ek()	(machine_arch_type == MACH_TYPE_AT91SAM9263EK)
#else
# define machine_is_at91sam9263ek()	(0)
#endif

#ifdef CONFIG_MACH_PM9263
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_PM9263
# endif
# define machine_is_pm9263()	(machine_arch_type == MACH_TYPE_PM9263)
#else
# define machine_is_pm9263()	(0)
#endif

#ifdef CONFIG_MACH_OMAP3_BEAGLE
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_OMAP3_BEAGLE
# endif
# define machine_is_omap3_beagle()	(machine_arch_type == MACH_TYPE_OMAP3_BEAGLE)
#else
# define machine_is_omap3_beagle()	(0)
#endif

#ifdef CONFIG_MACH_AT91SAM9G20EK
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_AT91SAM9G20EK
# endif
# define machine_is_at91sam9g20ek()	(machine_arch_type == MACH_TYPE_AT91SAM9G20EK)
#else
# define machine_is_at91sam9g20ek()	(0)
#endif

#ifdef CONFIG_MACH_USB_A9260
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_USB_A9260
# endif
# define machine_is_usb_a9260()	(machine_arch_type == MACH_TYPE_USB_A9260)
#else
# define machine_is_usb_a9260()	(0)
#endif

#ifdef CONFIG_MACH_USB_A9263
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_USB_A9263
# endif
# define machine_is_usb_a9263()	(machine_arch_type == MACH_TYPE_USB_A9263)
#else
# define machine_is_usb_a9263()	(0)
#endif

#ifdef CONFIG_MACH_QIL_A9260
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_QIL_A9260
# endif
# define machine_is_qil_a9260()	(machine_arch_type == MACH_TYPE_QIL_A9260)
#else
# define machine_is_qil_a9260()	(0)
#endif

#ifdef CONFIG_MACH_PICOCOM1
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_PICOCOM1
# endif
# define machine_is_picocom1()	(machine_arch_type == MACH_TYPE_PICOCOM1)
#else
# define machine_is_picocom1()	(0)
#endif

#ifdef CONFIG_MACH_AT91SAM9M10G45EK
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_AT91SAM9M10G45EK
# endif
# define machine_is_at91sam9m10g45ek()	(machine_arch_type == MACH_TYPE_AT91SAM9M10G45EK)
#else
# define machine_is_at91sam9m10g45ek()	(0)
#endif

#ifdef CONFIG_MACH_USB_A9G20
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_USB_A9G20
# endif
# define machine_is_usb_a9g20()	(machine_arch_type == MACH_TYPE_USB_A9G20)
#else
# define machine_is_usb_a9g20()	(0)
#endif

#ifdef CONFIG_MACH_QIL_A9G20
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_QIL_A9G20
# endif
# define machine_is_qil_a9g20()	(machine_arch_type == MACH_TYPE_QIL_A9G20)
#else
# define machine_is_qil_a9g20()	(0)
#endif

#ifdef CONFIG_MACH_CHUMBY
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_CHUMBY
# endif
# define machine_is_chumby()	(machine_arch_type == MACH_TYPE_CHUMBY)
#else
# define machine_is_chumby()	(0)
#endif

#ifdef CONFIG_MACH_TNY_A9260
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_TNY_A9260
# endif
# define machine_is_tny_a9260()	(machine_arch_type == MACH_TYPE_TNY_A9260)
#else
# define machine_is_tny_a9260()	(0)
#endif

#ifdef CONFIG_MACH_TNY_A9G20
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_TNY_A9G20
# endif
# define machine_is_tny_a9g20()	(machine_arch_type == MACH_TYPE_TNY_A9G20)
#else
# define machine_is_tny_a9g20()	(0)
#endif

#ifdef CONFIG_MACH_MX51_BABBAGE
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_MX51_BABBAGE
# endif
# define machine_is_mx51_babbage()	(machine_arch_type == MACH_TYPE_MX51_BABBAGE)
#else
# define machine_is_mx51_babbage()	(0)
#endif

#ifdef CONFIG_MACH_TNY_A9263
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_TNY_A9263
# endif
# define machine_is_tny_a9263()	(machine_arch_type == MACH_TYPE_TNY_A9263)
#else
# define machine_is_tny_a9263()	(0)
#endif

#ifdef CONFIG_MACH_AT91SAM9G10EK
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_AT91SAM9G10EK
# endif
# define machine_is_at91sam9g10ek()	(machine_arch_type == MACH_TYPE_AT91SAM9G10EK)
#else
# define machine_is_at91sam9g10ek()	(0)
#endif

#ifdef CONFIG_MACH_TX25
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_TX25
# endif
# define machine_is_tx25()	(machine_arch_type == MACH_TYPE_TX25)
#else
# define machine_is_tx25()	(0)
#endif

#ifdef CONFIG_MACH_MX23EVK
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_MX23EVK
# endif
# define machine_is_mx23evk()	(machine_arch_type == MACH_TYPE_MX23EVK)
#else
# define machine_is_mx23evk()	(0)
#endif

#ifdef CONFIG_MACH_PM9G45
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_PM9G45
# endif
# define machine_is_pm9g45()	(machine_arch_type == MACH_TYPE_PM9G45)
#else
# define machine_is_pm9g45()	(0)
#endif

#ifdef CONFIG_MACH_OMAP4_PANDA
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_OMAP4_PANDA
# endif
# define machine_is_omap4_panda()	(machine_arch_type == MACH_TYPE_OMAP4_PANDA)
#else
# define machine_is_omap4_panda()	(0)
#endif

#ifdef CONFIG_MACH_ARMADA_XP_DB
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_ARMADA_XP_DB
# endif
# define machine_is_armada_xp_db()	(machine_arch_type == MACH_TYPE_ARMADA_XP_DB)
#else
# define machine_is_armada_xp_db()	(0)
#endif

#ifdef CONFIG_MACH_TX28
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_TX28
# endif
# define machine_is_tx28()	(machine_arch_type == MACH_TYPE_TX28)
#else
# define machine_is_tx28()	(0)
#endif

#ifdef CONFIG_MACH_BCM2708
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_BCM2708
# endif
# define machine_is_bcm2708()	(machine_arch_type == MACH_TYPE_BCM2708)
#else
# define machine_is_bcm2708()	(0)
#endif

#ifdef CONFIG_MACH_MX53_LOCO
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_MX53_LOCO
# endif
# define machine_is_mx53_loco()	(machine_arch_type == MACH_TYPE_MX53_LOCO)
#else
# define machine_is_mx53_loco()	(0)
#endif

#ifdef CONFIG_MACH_TX53
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_TX53
# endif
# define machine_is_tx53()	(machine_arch_type == MACH_TYPE_TX53)
#else
# define machine_is_tx53()	(0)
#endif

#ifdef CONFIG_MACH_CCMX53
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_CCMX53
# endif
# define machine_is_ccmx53()	(machine_arch_type == MACH_TYPE_CCMX53)
#else
# define machine_is_ccmx53()	(0)
#endif

#ifdef CONFIG_MACH_CCWMX53
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_CCWMX53
# endif
# define machine_is_ccwmx53()	(machine_arch_type == MACH_TYPE_CCWMX53)
#else
# define machine_is_ccwmx53()	(0)
#endif

#ifdef CONFIG_MACH_VMX53
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_VMX53
# endif
# define machine_is_vmx53()	(machine_arch_type == MACH_TYPE_VMX53)
#else
# define machine_is_vmx53()	(0)
#endif

#ifdef CONFIG_MACH_DSS11
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_DSS11
# endif
# define machine_is_dss11()	(machine_arch_type == MACH_TYPE_DSS11)
#else
# define machine_is_dss11()	(0)
#endif

#ifdef CONFIG_MACH_BEAGLEBONE
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_BEAGLEBONE
# endif
# define machine_is_beaglebone()	(machine_arch_type == MACH_TYPE_BEAGLEBONE)
#else
# define machine_is_beaglebone()	(0)
#endif

#ifdef CONFIG_MACH_PCAAXL2
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_PCAAXL2
# endif
# define machine_is_pcaaxl2()	(machine_arch_type == MACH_TYPE_PCAAXL2)
#else
# define machine_is_pcaaxl2()	(0)
#endif

#ifdef CONFIG_MACH_MX6Q_SABRESD
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_MX6Q_SABRESD
# endif
# define machine_is_mx6q_sabresd()	(machine_arch_type == MACH_TYPE_MX6Q_SABRESD)
#else
# define machine_is_mx6q_sabresd()	(0)
#endif

#ifdef CONFIG_MACH_TQMA53
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_TQMA53
# endif
# define machine_is_tqma53()	(machine_arch_type == MACH_TYPE_TQMA53)
#else
# define machine_is_tqma53()	(0)
#endif

#ifdef CONFIG_MACH_IMX233_OLINUXINO
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_IMX233_OLINUXINO
# endif
# define machine_is_imx233_olinuxino()	(machine_arch_type == MACH_TYPE_IMX233_OLINUXINO)
#else
# define machine_is_imx233_olinuxino()	(0)
#endif

#ifdef CONFIG_MACH_CFA10036
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_CFA10036
# endif
# define machine_is_cfa10036()	(machine_arch_type == MACH_TYPE_CFA10036)
#else
# define machine_is_cfa10036()	(0)
#endif

#ifdef CONFIG_MACH_PCM051
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_PCM051
# endif
# define machine_is_pcm051()	(machine_arch_type == MACH_TYPE_PCM051)
#else
# define machine_is_pcm051()	(0)
#endif

#ifdef CONFIG_MACH_HABA_KNX_LITE
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_HABA_KNX_LITE
# endif
# define machine_is_haba_knx_lite()	(machine_arch_type == MACH_TYPE_HABA_KNX_LITE)
#else
# define machine_is_haba_knx_lite()	(0)
#endif

#ifdef CONFIG_MACH_VAR_SOM_MX6
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_VAR_SOM_MX6
# endif
# define machine_is_var_som_mx6()	(machine_arch_type == MACH_TYPE_VAR_SOM_MX6)
#else
# define machine_is_var_som_mx6()	(0)
#endif

#ifdef CONFIG_MACH_PCAAXS1
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_PCAAXS1
# endif
# define machine_is_pcaaxs1()	(machine_arch_type == MACH_TYPE_PCAAXS1)
#else
# define machine_is_pcaaxs1()	(0)
#endif

#ifdef CONFIG_MACH_PFLA03
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_PFLA03
# endif
# define machine_is_pfla03()	(machine_arch_type == MACH_TYPE_PFLA03)
#else
# define machine_is_pfla03()	(0)
#endif

/*
 * These have not yet been registered
 */

#ifndef machine_arch_type
#define machine_arch_type	__machine_arch_type
#endif

#endif
