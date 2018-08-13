/*
 * Copyright (C) 2014 Lucas Stach, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

static inline void enable_arm_errata_709718_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c10, c2, 0\n"
		"bic	r0, #3 << 16\n"
		"mcr	p15, 0, r0, c10, c2, 0\n"
		"mrc	p15, 0, r0, c1, c0, 0\n"
		"orr	r0, r0, #1 << 28\n"
		"mcr	p15, 0, r0, c1, c0, 0\n"
	);
}

static inline void enable_arm_errata_716044_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c1, c0, 0\n"
		"orr	r0, r0, #1 << 11\n"
		"mcr	p15, 0, r0, c1, c0, 0\n"
	);
}

static inline void enable_arm_errata_742230_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 4\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

static inline void enable_arm_errata_743622_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 6\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

static inline void enable_arm_errata_751472_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 11\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

static inline void enable_arm_errata_761320_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 21\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

static inline void enable_arm_errata_794072_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 4\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

static inline void enable_arm_errata_845369_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 22\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

static inline void enable_arm_errata_cortexa8_enable_ibe(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c1, c0, 1\n"
		"orr	r0, r0, #1 << 6\n"
		"mcr	p15, 0, r0, c1, c0, 1\n"
	);
}
