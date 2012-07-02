/*
 * Copyright (C) 2012 Alexey Galakhov
 *
 * Based on code from u-boot found somewhere on the web
 * that seems to originate from Samsung
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

#include <config.h>
#include <common.h>
#include <io.h>
#include <init.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-iomap.h>

#define S5P_DMC_CONCONTROL	0x00
#define S5P_DMC_MEMCONTROL	0x04
#define S5P_DMC_MEMCONFIG0	0x08
#define S5P_DMC_MEMCONFIG1	0x0C
#define S5P_DMC_DIRECTCMD	0x10
#define S5P_DMC_PRECHCONFIG	0x14
#define S5P_DMC_PHYCONTROL0	0x18
#define S5P_DMC_PHYCONTROL1	0x1C
#define S5P_DMC_PWRDNCONFIG	0x28
#define S5P_DMC_TIMINGAREF	0x30
#define S5P_DMC_TIMINGROW	0x34
#define S5P_DMC_TIMINGDATA	0x38
#define S5P_DMC_TIMINGPOWER	0x3C
#define S5P_DMC_PHYSTATUS	0x40
#define S5P_DMC_MRSTATUS	0x54

/* DRAM commands */
#define CMD(x)  ((x) << 24)
#define BANK(x) ((x) << 16)
#define CHIP(x) ((x) << 20)
#define ADDR(x) (x)

/**
 *  MR definition:
 *  1 11
 *  2 1098 7654 3210
 *      |     |  ^^^- burst length, 010=4, 011=8
 *      |     | ^- burst type 0=sequnential, 1=interleaved
 *      |   ^^^-- CAS latency
 *      |  ^----- test, 0=normal, 1=test
 *      |^---- DLL reset, 1=yes
 *    ^^^----- WR, 1=2, 2=3 etc.
 *  ^------- PD, 0=fast exit, 1=low power
 *
 *  EMR1 definition:
 *  1 11
 *  2 1098 7654 3210
 *         |       ^- DLL, 0=enable
 *         |      ^-- output strength, 0=full, 1=reduced
 *         |^.. .^--- Rtt, 00=off, 01=75, 10=150, 11=50 Ohm
 *         | ^^ ^-- Posted CAS# AL, 0-6
 *      ^^ ^------ OCD: 000=OCD exit, 111=enable defaults
 *     ^------ DQS#, 0=enable, 1=disable
 *    ^------- RDQS enable, 0=no, 1=yes
 *  ^-------- outputs, 0=enabled, 1=disabled
 *
 *  EMR2 definition:
 *  bit 7
 *  1 1
 *  2 1098 7654 3210
 *         ^-- SRT, 0=1x (0-85 deg.C), 1=2x (>85 deg.C)
 *  all other bits = 0
 *
 *  EMR3 definition: all bits 0
 */

#define MRS	CMD(0x0)
#define PALL	CMD(0x1)
#define PRE	CMD(0x2)
#define DPD	CMD(0x3)
#define REFS	CMD(0x4)
#define REFA	CMD(0x5)
#define CKEL	CMD(0x6)
#define NOP	CMD(0x7)
#define REFSX	CMD(0x8)
#define MRR	CMD(0x9)

#define EMRS1 (MRS | BANK(1))
#define EMRS2 (MRS | BANK(2))
#define EMRS3 (MRS | BANK(3))

/* Burst is (1 << S5P_DRAM_BURST), i.e. S5P_DRAM_BURST=2 for burst 4 */
#ifndef S5P_DRAM_BURST
/* (LP)DDR2 supports burst 4 only, make it default */
# define S5P_DRAM_BURST 2
#endif

/**
 * Initialization sequences for different kinds of DRAM
 */
#define dcmd(x) writel((x) | CHIP(chip), base + S5P_DMC_DIRECTCMD)

static void __bare_init s5p_dram_init_seq_lpddr(phys_addr_t base, unsigned chip)
{
	const uint32_t emr = 0x400; /* DQS disable */
	const uint32_t mr = (((S5P_DRAM_WR) - 1) << 9)
			  | ((S5P_DRAM_CAS) << 4)
			  | (S5P_DRAM_BURST);
	/* TODO this sequence is untested */
	dcmd(PALL); dcmd(REFA); dcmd(REFA);
	dcmd(MRS   | ADDR(mr));
	dcmd(EMRS1 | ADDR(emr));
}

static void __bare_init s5p_dram_init_seq_lpddr2(phys_addr_t base, unsigned chip)
{
	const uint32_t mr = (((S5P_DRAM_WR) - 1) << 9)
			  | ((S5P_DRAM_CAS) << 4)
			  | (S5P_DRAM_BURST);
	/* TODO this sequence is untested */
	dcmd(NOP);
	dcmd(MRS | ADDR(mr));
	do {
		dcmd(MRR);
	} while (readl(base + S5P_DMC_MRSTATUS) & 0x01); /* poll DAI */
}

static void __bare_init s5p_dram_init_seq_ddr2(phys_addr_t base, unsigned chip)
{
	const uint32_t emr = 0x400; /* DQS disable */
	const uint32_t mr = (((S5P_DRAM_WR) - 1) << 9)
			  | ((S5P_DRAM_CAS) << 4)
			  | (S5P_DRAM_BURST);
	dcmd(NOP);
	/* FIXME wait here? JEDEC recommends but nobody does */
	dcmd(PALL); dcmd(EMRS2); dcmd(EMRS3);
	dcmd(EMRS1 | ADDR(emr));         /* DQS disable */
	dcmd(MRS   | ADDR(mr | 0x100));  /* DLL reset */
	dcmd(PALL); dcmd(REFA); dcmd(REFA);
	dcmd(MRS   | ADDR(mr));          /* DLL no reset */
	dcmd(EMRS1 | ADDR(emr | 0x380)); /* OCD defaults */
	dcmd(EMRS1 | ADDR(emr));         /* OCD exit */
}

#undef dcmd

static inline void __bare_init s5p_dram_start_dll(phys_addr_t base, uint32_t phycon1)
{
	uint32_t pc0 = 0x00101000; /* the only legal initial value */
	uint32_t lv;

	/* Init DLL */
	writel(pc0, base + S5P_DMC_PHYCONTROL0);
	writel(phycon1, base + S5P_DMC_PHYCONTROL1);

	/* DLL on */
	pc0 |= 0x2;
	writel(pc0, base + S5P_DMC_PHYCONTROL0);

	/* DLL start */
	pc0 |= 0x1;
	writel(pc0, base + S5P_DMC_PHYCONTROL0);

	/* Find lock val */
	do {
		lv = readl(base + S5P_DMC_PHYSTATUS);
	} while ((lv & 0x7) != 0x7);

	lv >>= 6;
	lv &= 0xff; /* ctrl_lock_value[9:2] - coarse */
	pc0 |= (lv << 24); /* ctrl_force */
	writel(pc0, base + S5P_DMC_PHYCONTROL0); /* force value locking */
}

static inline void __bare_init s5p_dram_setup(phys_addr_t base, uint32_t mc0, uint32_t mc1,
				       int bus16, uint32_t mcon)
{
	mcon |= (S5P_DRAM_BURST) << 20;
	/* 16 or 32-bit bus ? */
	mcon |= bus16 ? 0x1000 : 0x2000;
	if (mc1)
		mcon |= 0x10000; /* two chips */

	writel(mcon, base + S5P_DMC_MEMCONTROL);

	/* Set up memory layout */
	writel(mc0, base + S5P_DMC_MEMCONFIG0);
	if (mc1)
		writel(mc1, base + S5P_DMC_MEMCONFIG1);

	/* Open page precharge policy - reasonable defaults */
	writel(0xFF000000, base + S5P_DMC_PRECHCONFIG);

	/* Set up timings */
	writel(DMC_TIMING_AREF, base + S5P_DMC_TIMINGAREF);
	writel(DMC_TIMING_ROW,  base + S5P_DMC_TIMINGROW);
	writel(DMC_TIMING_DATA, base + S5P_DMC_TIMINGDATA);
	writel(DMC_TIMING_PWR,  base + S5P_DMC_TIMINGPOWER);
}

static inline void __bare_init s5p_dram_start(phys_addr_t base)
{
	/* Reasonable defaults and auto-refresh on */
	writel(0x0FFF1070, base + S5P_DMC_CONCONTROL);
	/* Reasonable defaults */
	writel(0xFFFF00FF, base + S5P_DMC_PWRDNCONFIG);
}

/*
 * Initialize LPDDR memory bank
 * TODO: this function is untested, see also init_seq function
 */
void __bare_init s5p_init_dram_bank_lpddr(phys_addr_t base, uint32_t mc0, uint32_t mc1, int bus16)
{
	/* refcount 8, 90 deg. shift */
	s5p_dram_start_dll(base, 0x00000085);
	/* LPDDR type */
	s5p_dram_setup(base, mc0, mc1, bus16, 0x100);

	/* Start-Up Commands */
	s5p_dram_init_seq_lpddr(base, 0);
	if (mc1)
		s5p_dram_init_seq_lpddr(base, 1);

	s5p_dram_start(base);
}

/*
 * Initialize LPDDR2 memory bank
 * TODO: this function is untested, see also init_seq function
 */
void __bare_init s5p_init_dram_bank_lpddr2(phys_addr_t base, uint32_t mc0, uint32_t mc1, int bus16)
{
	/* refcount 8, 90 deg. shift */
	s5p_dram_start_dll(base, 0x00000085);
	/* LPDDR2 type */
	s5p_dram_setup(base, mc0, mc1, bus16, 0x200);

	/* Start-Up Commands */
	s5p_dram_init_seq_lpddr2(base, 0);
	if (mc1)
		s5p_dram_init_seq_lpddr2(base, 1);

	s5p_dram_start(base);
}

/*
 * Initialize DDR2 memory bank
 */
void __bare_init s5p_init_dram_bank_ddr2(phys_addr_t base, uint32_t mc0, uint32_t mc1, int bus16)
{
	/* refcount 8, 180 deg. shift */
	s5p_dram_start_dll(base, 0x00000086);
	/* DDR2 type */
	s5p_dram_setup(base, mc0, mc1, bus16, 0x400);

	/* Start-Up Commands */
	s5p_dram_init_seq_ddr2(base, 0);
	if (mc1)
		s5p_dram_init_seq_ddr2(base, 1);

	s5p_dram_start(base);
}


#define BANK_ENABLED(base) (readl((base) + S5P_DMC_PHYCONTROL0) & 1)
#define NUM_EXTRA_CHIPS(base) ((readl((base) + S5P_DMC_MEMCONTROL) >> 16) & 0xF)

#define BANK_START(x) ((x) & 0xFF000000)
#define BANK_END(x) (BANK_START(x) | ~(((x) & 0x00FF0000) << 8))
#define BANK_LEN(x) (BANK_END(x) - BANK_START(x) + 1)

static inline void sortswap(uint32_t *x, uint32_t *y)
{
	if (*y < *x) {
		*x ^= *y;
		*y ^= *x;
		*x ^= *y;
	}
}

uint32_t s5p_get_memory_size(void)
{
	int i;
	uint32_t len;
	uint32_t mc[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	/* Read MEMCONFIG registers */
	if (BANK_ENABLED(S5P_DMC0_BASE)) {
		mc[0] = readl(S5P_DMC0_BASE + S5P_DMC_MEMCONFIG0);
		if (NUM_EXTRA_CHIPS(S5P_DMC0_BASE) > 0)
			mc[1] = readl(S5P_DMC0_BASE + S5P_DMC_MEMCONFIG1);
	}
	if (BANK_ENABLED(S5P_DMC1_BASE)) {
		mc[2] = readl(S5P_DMC1_BASE + S5P_DMC_MEMCONFIG0);
		if (NUM_EXTRA_CHIPS(S5P_DMC1_BASE) > 0)
			mc[3] = readl(S5P_DMC1_BASE + S5P_DMC_MEMCONFIG1);
	}
	/* Sort using a sorting network */
	sortswap(mc + 0, mc + 2);
	sortswap(mc + 1, mc + 3);
	sortswap(mc + 0, mc + 1);
	sortswap(mc + 2, mc + 3);
	sortswap(mc + 1, mc + 2);
	/* Is at least one chip enabled? */
	if (mc[0] == 0xFFFFFFFF)
		return 0;
	/* Determine maximum continuous region at start */
	len = BANK_LEN(mc[0]);
	for (i = 1; i < 4; ++i) {
		if (BANK_START(mc[i]) == BANK_END(mc[i - 1]) + 1)
			len += BANK_LEN(mc[i]);
		else
			break;
	}
	return len;
}
