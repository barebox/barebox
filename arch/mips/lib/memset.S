/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 1998, 1999, 2000 by Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2007 by Maciej W. Rozycki
 * Copyright (C) 2011, 2012 MIPS Technologies, Inc.
 *
 * Kernel-mode memset function without exceptions
 * by Aleksey Kuleshov (rndfax@yandex.ru), 2015
 */
#include <asm/asm.h>
#include <asm/asm-offsets.h>
#include <asm/regdef.h>

#if LONGSIZE == 4
#define LONG_S_L swl
#define LONG_S_R swr
#else
#define LONG_S_L sdl
#define LONG_S_R sdr
#endif

#define STORSIZE LONGSIZE
#define STORMASK LONGMASK
#define FILL64RG a1
#define FILLPTRG t0

/*
 * memset(void *s, int c, size_t n)
 *
 * a0: start of area to clear
 * a1: char to fill with
 * a2: size of area to clear
 */

LEAF(memset)
	beqz		a1, 1f
	move		v0, a0			/* result */

	andi		a1, 0xff		/* spread fillword */
	LONG_SLL		t1, a1, 8
	or		a1, t1
	LONG_SLL		t1, a1, 16
#if LONGSIZE == 8
	or		a1, t1
	LONG_SLL		t1, a1, 32
#endif
	or		a1, t1

	.macro	f_fill64 dst, offset, val
	LONG_S	\val, (\offset +  0 * STORSIZE)(\dst)
	LONG_S	\val, (\offset +  1 * STORSIZE)(\dst)
	LONG_S	\val, (\offset +  2 * STORSIZE)(\dst)
	LONG_S	\val, (\offset +  3 * STORSIZE)(\dst)
	LONG_S	\val, (\offset +  4 * STORSIZE)(\dst)
	LONG_S	\val, (\offset +  5 * STORSIZE)(\dst)
	LONG_S	\val, (\offset +  6 * STORSIZE)(\dst)
	LONG_S	\val, (\offset +  7 * STORSIZE)(\dst)
#if (LONGSIZE == 4)
	LONG_S	\val, (\offset +  8 * STORSIZE)(\dst)
	LONG_S	\val, (\offset +  9 * STORSIZE)(\dst)
	LONG_S	\val, (\offset + 10 * STORSIZE)(\dst)
	LONG_S	\val, (\offset + 11 * STORSIZE)(\dst)
	LONG_S	\val, (\offset + 12 * STORSIZE)(\dst)
	LONG_S	\val, (\offset + 13 * STORSIZE)(\dst)
	LONG_S	\val, (\offset + 14 * STORSIZE)(\dst)
	LONG_S	\val, (\offset + 15 * STORSIZE)(\dst)
#endif
	.endm

	.set	noreorder
	.align	5

1:
	sltiu		t0, a2, STORSIZE	/* very small region? */
	bnez		t0, .Lsmall_memset
	andi		t0, a0, STORMASK	/* aligned? */

	beqz		t0, 1f
	PTR_SUBU	t0, STORSIZE		/* alignment in bytes */

#ifdef __MIPSEB__
	LONG_S_L	a1, (a0)		/* make word/dword aligned */
#else
	LONG_S_R	a1, (a0)		/* make word/dword aligned */
#endif
	PTR_SUBU	a0, t0			/* long align ptr */
	PTR_ADDU	a2, t0			/* correct size */

1:	ori		t1, a2, 0x3f		/* # of full blocks */
	xori		t1, 0x3f
	beqz		t1, .Lmemset_partial	/* no block to fill */
	andi		t0, a2, 0x40-STORSIZE

	PTR_ADDU	t1, a0			/* end address */
	.set		reorder
1:	PTR_ADDIU	a0, 64
	f_fill64 a0, -64, FILL64RG
	bne		t1, a0, 1b
	.set		noreorder

.Lmemset_partial:
	PTR_LA		t1, 2f			/* where to start */
#if LONGSIZE == 4
	PTR_SUBU	t1, FILLPTRG
#else
	.set		noat
	LONG_SRL	AT, FILLPTRG, 1
	PTR_SUBU	t1, AT
	.set		at
#endif
	jr		t1
	PTR_ADDU	a0, t0			/* dest ptr */

	.set		push
	.set		noreorder
	.set		nomacro
	/* ... but first do longs ... */
	f_fill64 a0, -64, FILL64RG
2:	.set		pop
	andi		a2, STORMASK		/* At most one long to go */

	beqz		a2, 1f
	PTR_ADDU	a0, a2			/* What's left */
#ifdef __MIPSEB__
	LONG_S_R	a1, -1(a0)
#else
	LONG_S_L	a1, -1(a0)
#endif
1:	jr		ra
	move		a2, zero

.Lsmall_memset:
	beqz		a2, 2f
	PTR_ADDU	t1, a0, a2

1:	PTR_ADDIU	a0, 1			/* fill bytewise */
	bne		t1, a0, 1b
	sb		a1, -1(a0)

2:	jr		ra			/* done */
	move		a2, zero
	END(memset)
