/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 1998, 99, 2000, 01, 2002 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999, 2000, 01, 2002 Silicon Graphics, Inc.
 * Copyright (C) 2002 Broadcom, Inc.
 *   memcpy/copy_user author: Mark Vandevoorde
 * Copyright (C) 2007  Maciej W. Rozycki
 * Copyright (C) 2014 Imagination Technologies Ltd.
 *
 * Kernel-mode memcpy function without exceptions for _some_ MIPS CPUs
 * by Aleksey Kuleshov (rndfax@yandex.ru), 2015
 *
 */

#include <asm/asm.h>
#include <asm/asm-offsets.h>
#include <asm/regdef.h>

#define dst a0
#define src a1
#define len a2

#define LOADK lw /* No exception */
#define LOAD(reg, addr)		lw reg, addr
#define LOADL(reg, addr)	lwl reg, addr
#define LOADR(reg, addr)	lwr reg, addr
#define STOREL(reg, addr)	swl reg, addr
#define STORER(reg, addr)	swr reg, addr
#define STORE(reg, addr)	sw reg, addr
#define ADD    addu
#define SUB    subu
#define SRL    srl
#define SLL    sll
#define SRA    sra
#define SLLV   sllv
#define SRLV   srlv
#define NBYTES 4
#define LOG_NBYTES 2

#define LOADB(reg, addr)	lb reg, addr
#define STOREB(reg, addr)	sb reg, addr

#ifdef CONFIG_CPU_LITTLE_ENDIAN
#define LDFIRST LOADR
#define LDREST	LOADL
#define STFIRST STORER
#define STREST	STOREL
#define SHIFT_DISCARD SLLV
#else
#define LDFIRST LOADL
#define LDREST	LOADR
#define STFIRST STOREL
#define STREST	STORER
#define SHIFT_DISCARD SRLV
#endif

#define FIRST(unit) ((unit)*NBYTES)
#define REST(unit)  (FIRST(unit)+NBYTES-1)
#define UNIT(unit)  FIRST(unit)

#define ADDRMASK (NBYTES-1)

	.text
	.align	5
	.set	noreorder
LEAF(memcpy)					/* a0=dst a1=src a2=len */
	move	v0, dst				/* return value */

	/*
	 * Note: dst & src may be unaligned, len may be 0
	 * Temps
	 */
#define rem t8

	/*
	 * The "issue break"s below are very approximate.
	 * Issue delays for dcache fills will perturb the schedule, as will
	 * load queue full replay traps, etc.
	 *
	 * If len < NBYTES use byte operations.
	 */
	sltu	t2, len, NBYTES
	and	t1, dst, ADDRMASK
	bnez	t2, .Lcopy_bytes_checklen
	 and	t0, src, ADDRMASK
	bnez	t1, .Ldst_unaligned
	 nop
	bnez	t0, .Lsrc_unaligned_dst_aligned
	/*
	 * use delay slot for fall-through
	 * src and dst are aligned; need to compute rem
	 */
.Lboth_aligned:
	 SRL	t0, len, LOG_NBYTES+3	 # +3 for 8 units/iter
	beqz	t0, .Lcleanup_both_aligned # len < 8*NBYTES
	 and	rem, len, (8*NBYTES-1)	 # rem = len % (8*NBYTES)
	.align	4
1:
	LOAD(t0, UNIT(0)(src))
	LOAD(t1, UNIT(1)(src))
	LOAD(t2, UNIT(2)(src))
	LOAD(t3, UNIT(3)(src))
	SUB	len, len, 8*NBYTES
	LOAD(t4, UNIT(4)(src))
	LOAD(t7, UNIT(5)(src))
	STORE(t0, UNIT(0)(dst))
	STORE(t1, UNIT(1)(dst))
	LOAD(t0, UNIT(6)(src))
	LOAD(t1, UNIT(7)(src))
	ADD	src, src, 8*NBYTES
	ADD	dst, dst, 8*NBYTES
	STORE(t2, UNIT(-6)(dst))
	STORE(t3, UNIT(-5)(dst))
	STORE(t4, UNIT(-4)(dst))
	STORE(t7, UNIT(-3)(dst))
	STORE(t0, UNIT(-2)(dst))
	STORE(t1, UNIT(-1)(dst))
	bne	len, rem, 1b
	 nop

	/*
	 * len == rem == the number of bytes left to copy < 8*NBYTES
	 */
.Lcleanup_both_aligned:
	beqz	len, .Ldone
	 sltu	t0, len, 4*NBYTES
	bnez	t0, .Lless_than_4units
	 and	rem, len, (NBYTES-1)	# rem = len % NBYTES
	/*
	 * len >= 4*NBYTES
	 */
	LOAD( t0, UNIT(0)(src))
	LOAD( t1, UNIT(1)(src))
	LOAD( t2, UNIT(2)(src))
	LOAD( t3, UNIT(3)(src))
	SUB	len, len, 4*NBYTES
	ADD	src, src, 4*NBYTES
	STORE(t0, UNIT(0)(dst))
	STORE(t1, UNIT(1)(dst))
	STORE(t2, UNIT(2)(dst))
	STORE(t3, UNIT(3)(dst))
	.set	reorder				/* DADDI_WAR */
	ADD	dst, dst, 4*NBYTES
	beqz	len, .Ldone
	.set	noreorder
.Lless_than_4units:
	/*
	 * rem = len % NBYTES
	 */
	beq	rem, len, .Lcopy_bytes
	 nop
1:
	LOAD(t0, 0(src))
	ADD	src, src, NBYTES
	SUB	len, len, NBYTES
	STORE(t0, 0(dst))
	.set	reorder				/* DADDI_WAR */
	ADD	dst, dst, NBYTES
	bne	rem, len, 1b
	.set	noreorder

	/*
	 * src and dst are aligned, need to copy rem bytes (rem < NBYTES)
	 * A loop would do only a byte at a time with possible branch
	 * mispredicts.	 Can't do an explicit LOAD dst,mask,or,STORE
	 * because can't assume read-access to dst.  Instead, use
	 * STREST dst, which doesn't require read access to dst.
	 *
	 * This code should perform better than a simple loop on modern,
	 * wide-issue mips processors because the code has fewer branches and
	 * more instruction-level parallelism.
	 */
#define bits t2
	beqz	len, .Ldone
	 ADD	t1, dst, len	# t1 is just past last byte of dst
	li	bits, 8*NBYTES
	SLL	rem, len, 3	# rem = number of bits to keep
	LOAD(t0, 0(src))
	SUB	bits, bits, rem # bits = number of bits to discard
	SHIFT_DISCARD t0, t0, bits
	STREST(t0, -1(t1))
	jr	ra
	 move	len, zero
.Ldst_unaligned:
	/*
	 * dst is unaligned
	 * t0 = src & ADDRMASK
	 * t1 = dst & ADDRMASK; T1 > 0
	 * len >= NBYTES
	 *
	 * Copy enough bytes to align dst
	 * Set match = (src and dst have same alignment)
	 */
#define match rem
	LDFIRST(t3, FIRST(0)(src))
	ADD	t2, zero, NBYTES
	LDREST(t3, REST(0)(src))
	SUB	t2, t2, t1	# t2 = number of bytes copied
	xor	match, t0, t1
	STFIRST(t3, FIRST(0)(dst))
	beq	len, t2, .Ldone
	 SUB	len, len, t2
	ADD	dst, dst, t2
	beqz	match, .Lboth_aligned
	 ADD	src, src, t2

.Lsrc_unaligned_dst_aligned:
	SRL	t0, len, LOG_NBYTES+2	 # +2 for 4 units/iter
	beqz	t0, .Lcleanup_src_unaligned
	 and	rem, len, (4*NBYTES-1)	 # rem = len % 4*NBYTES
1:
/*
 * Avoid consecutive LD*'s to the same register since some mips
 * implementations can't issue them in the same cycle.
 * It's OK to load FIRST(N+1) before REST(N) because the two addresses
 * are to the same unit (unless src is aligned, but it's not).
 */
	LDFIRST(t0, FIRST(0)(src))
	LDFIRST(t1, FIRST(1)(src))
	SUB	len, len, 4*NBYTES
	LDREST(t0, REST(0)(src))
	LDREST(t1, REST(1)(src))
	LDFIRST(t2, FIRST(2)(src))
	LDFIRST(t3, FIRST(3)(src))
	LDREST(t2, REST(2)(src))
	LDREST(t3, REST(3)(src))
	ADD	src, src, 4*NBYTES
	STORE(t0, UNIT(0)(dst))
	STORE(t1, UNIT(1)(dst))
	STORE(t2, UNIT(2)(dst))
	STORE(t3, UNIT(3)(dst))
	.set	reorder				/* DADDI_WAR */
	ADD	dst, dst, 4*NBYTES
	bne	len, rem, 1b
	.set	noreorder

.Lcleanup_src_unaligned:
	beqz	len, .Ldone
	 and	rem, len, NBYTES-1  # rem = len % NBYTES
	beq	rem, len, .Lcopy_bytes
	 nop
1:
	LDFIRST(t0, FIRST(0)(src))
	LDREST(t0, REST(0)(src))
	ADD	src, src, NBYTES
	SUB	len, len, NBYTES
	STORE(t0, 0(dst))
	.set	reorder				/* DADDI_WAR */
	ADD	dst, dst, NBYTES
	bne	len, rem, 1b
	.set	noreorder

.Lcopy_bytes_checklen:
	beqz	len, .Ldone
	 nop
.Lcopy_bytes:
	/* 0 < len < NBYTES  */
#define COPY_BYTE(N)			\
	LOADB(t0, N(src));	\
	SUB	len, len, 1;		\
	beqz	len, .Ldone;		\
	STOREB(t0, N(dst))

	COPY_BYTE(0)
	COPY_BYTE(1)
	LOADB(t0, NBYTES-2(src))
	SUB	len, len, 1
	jr	ra
	STOREB(t0, NBYTES-2(dst))
.Ldone:
	jr	ra
	 nop
	END(memcpy)
