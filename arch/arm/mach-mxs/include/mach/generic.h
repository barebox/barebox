/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2010 Juergen Beisert, Pengutronix */

#ifdef CONFIG_ARCH_IMX23
# define cpu_is_mx23()	(1)
#else
# define cpu_is_mx23()	(0)
#endif

#ifdef CONFIG_ARCH_IMX28
# define cpu_is_mx28()	(1)
#else
# define cpu_is_mx28()	(0)
#endif

#define cpu_is_mx1()	(0)
#define cpu_is_mx21()	(0)
#define cpu_is_mx25()	(0)
#define cpu_is_mx27()	(0)
#define cpu_is_mx31()	(0)
#define cpu_is_mx35()	(0)
#define cpu_is_mx51()	(0)
#define cpu_is_mx53()	(0)
#define cpu_is_mx6()	(0)
