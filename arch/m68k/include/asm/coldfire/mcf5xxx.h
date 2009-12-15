/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Definitions common across all ColdFire processors
 */
#ifndef __MCF5XXX__H
#define __MCF5XXX__H

/*
 * Common M68K & ColdFire definitions
 */
#define ADDRESS                  uint32_t
#define INSTRUCTION              uint16_t
#define ILLEGAL                  0x4AFC
#define CPU_WORD_SIZE            16

/*
 *  Definitions for CPU status register (SR)
 */
#define MCF5XXX_SR_T            (0x8000)
#define MCF5XXX_SR_S            (0x2000)
#define MCF5XXX_SR_M            (0x1000)
#define MCF5XXX_SR_IPL          (0x0700)
#define MCF5XXX_SR_IPL_0        (0x0000)
#define MCF5XXX_SR_IPL_1        (0x0100)
#define MCF5XXX_SR_IPL_2        (0x0200)
#define MCF5XXX_SR_IPL_3        (0x0300)
#define MCF5XXX_SR_IPL_4        (0x0400)
#define MCF5XXX_SR_IPL_5        (0x0500)
#define MCF5XXX_SR_IPL_6        (0x0600)
#define MCF5XXX_SR_IPL_7        (0x0700)
#define MCF5XXX_SR_X            (0x0010)
#define MCF5XXX_SR_N            (0x0008)
#define MCF5XXX_SR_Z            (0x0004)
#define MCF5XXX_SR_V            (0x0002)
#define MCF5XXX_SR_C            (0x0001)

/*
 *  Definitions for CPU cache control register
 */
#define MCF5XXX_CACR_CENB       (0x80000000)
#define MCF5XXX_CACR_DEC        (0x80000000)
#define MCF5XXX_CACR_DW         (0x40000000)
#define MCF5XXX_CACR_DESB       (0x20000000)
#define MCF5XXX_CACR_CPDI       (0x10000000)
#define MCF5XXX_CACR_DDPI       (0x10000000)
#define MCF5XXX_CACR_CPD        (0x10000000)
#define MCF5XXX_CACR_CFRZ       (0x08000000)
#define MCF5XXX_CACR_DHLCK      (0x08000000)
#define MCF5XXX_CACR_DDCM_WT    (0x00000000)
#define MCF5XXX_CACR_DDCM_CB    (0x02000000)
#define MCF5XXX_CACR_DDCM_IP    (0x04000000)
#define MCF5XXX_CACR_DDCM_II    (0x06000000)
#define MCF5XXX_CACR_CINV       (0x01000000)
#define MCF5XXX_CACR_DCINVA     (0x01000000)
#define MCF5XXX_CACR_DIDI       (0x00800000)
#define MCF5XXX_CACR_DDSP       (0x00800000)
#define MCF5XXX_CACR_DISD       (0x00400000)
#define MCF5XXX_CACR_INVI       (0x00200000)
#define MCF5XXX_CACR_INVD       (0x00100000)
#define MCF5XXX_CACR_BEC        (0x00080000)
#define MCF5XXX_CACR_BCINVA     (0x00040000)
#define MCF5XXX_CACR_IEC        (0x00008000)
#define MCF5XXX_CACR_DNFB       (0x00002000)
#define MCF5XXX_CACR_IDPI       (0x00001000)
#define MCF5XXX_CACR_IHLCK      (0x00000800)
#define MCF5XXX_CACR_CEIB       (0x00000400)
#define MCF5XXX_CACR_IDCM       (0x00000400)
#define MCF5XXX_CACR_DCM_WR     (0x00000000)
#define MCF5XXX_CACR_DCM_CB     (0x00000100)
#define MCF5XXX_CACR_DCM_IP     (0x00000200)
#define MCF5XXX_CACR_DCM        (0x00000200)
#define MCF5XXX_CACR_DCM_II     (0x00000300)
#define MCF5XXX_CACR_DBWE       (0x00000100)
#define MCF5XXX_CACR_ICINVA     (0x00000100)
#define MCF5XXX_CACR_IDSP       (0x00000080)
#define MCF5XXX_CACR_DWP        (0x00000020)
#define MCF5XXX_CACR_EUSP       (0x00000020)
#define MCF5XXX_CACR_EUST       (0x00000020)
#define MCF5XXX_CACR_DF         (0x00000010)
#define MCF5XXX_CACR_CLNF_00    (0x00000000)
#define MCF5XXX_CACR_CLNF_01    (0x00000002)
#define MCF5XXX_CACR_CLNF_10    (0x00000004)
#define MCF5XXX_CACR_CLNF_11    (0x00000006)

/*
 *  Definition for CPU access control register
 */
#define MCF5XXX_ACR_AB(a)       ((a)&0xFF000000)
#define MCF5XXX_ACR_AM(a)       (((a)&0xFF000000) >> 8)
#define MCF5XXX_ACR_EN          (0x00008000)
#define MCF5XXX_ACR_SM_USER     (0x00000000)
#define MCF5XXX_ACR_SM_SUPER    (0x00002000)
#define MCF5XXX_ACR_SM_IGNORE   (0x00006000)
#define MCF5XXX_ACR_ENIB        (0x00000080)
#define MCF5XXX_ACR_CM          (0x00000040)
#define MCF5XXX_ACR_DCM_WR      (0x00000000)
#define MCF5XXX_ACR_DCM_CB      (0x00000020)
#define MCF5XXX_ACR_DCM_IP      (0x00000040)
#define MCF5XXX_ACR_DCM_II      (0x00000060)
#define MCF5XXX_ACR_CM          (0x00000040)
#define MCF5XXX_ACR_BWE         (0x00000020)
#define MCF5XXX_ACR_WP          (0x00000004)

/*
 *  Definitions for CPU core sram control registers
 */
#define MCF5XXX_RAMBAR_BA(a)    ((a)&0xFFFFC000)
#define MCF5XXX_RAMBAR_PRI_00   (0x00000000)
#define MCF5XXX_RAMBAR_PRI_01   (0x00004000)
#define MCF5XXX_RAMBAR_PRI_10   (0x00008000)
#define MCF5XXX_RAMBAR_PRI_11   (0x0000C000)
#define MCF5XXX_RAMBAR_WP       (0x00000100)
#define MCF5XXX_RAMBAR_CI       (0x00000020)
#define MCF5XXX_RAMBAR_SC       (0x00000010)
#define MCF5XXX_RAMBAR_SD       (0x00000008)
#define MCF5XXX_RAMBAR_UC       (0x00000004)
#define MCF5XXX_RAMBAR_UD       (0x00000002)
#define MCF5XXX_RAMBAR_V        (0x00000001)


#ifndef __ASSEMBLY__

extern char __MBAR[];


/*
 * Extention to thhe basic POSIX data types
 */
typedef volatile uint8_t        vuint8_t;  /*  8 bits */
typedef volatile uint16_t       vuint16_t; /* 16 bits */
typedef volatile uint32_t       vuint32_t; /* 32 bits */

/*
 * Routines and macros for accessing Input/Output devices
 */

#define mcf_iord_8(ADDR)        *((vuint8_t *)(ADDR))
#define mcf_iord_16(ADDR)       *((vuint16_t *)(ADDR))
#define mcf_iord_32(ADDR)       *((vuint32_t *)(ADDR))

#define mcf_iowr_8(ADDR,DATA)   *((vuint8_t *)(ADDR)) = (DATA)
#define mcf_iowr_16(ADDR,DATA)  *((vuint16_t *)(ADDR)) = (DATA)
#define mcf_iowr_32(ADDR,DATA)  *((vuint32_t *)(ADDR)) = (DATA)

/*
 * The ColdFire family of processors has a simplified exception stack
 * frame that looks like the following:
 *
 *              3322222222221111 111111
 *              1098765432109876 5432109876543210
 *           8 +----------------+----------------+
 *             |         Program Counter         |
 *           4 +----------------+----------------+
 *             |FS/Fmt/Vector/FS|      SR        |
 *   SP -->  0 +----------------+----------------+
 *
 * The stack self-aligns to a 4-byte boundary at an exception, with
 * the FS/Fmt/Vector/FS field indicating the size of the adjustment
 * (SP += 0,1,2,3 bytes).
 */
#define MCF5XXX_RD_SF_FORMAT(PTR)   \
    ((*((uint16_t *)(PTR)) >> 12) & 0x00FF)

#define MCF5XXX_RD_SF_VECTOR(PTR)   \
    ((*((uint16_t *)(PTR)) >>  2) & 0x00FF)

#define MCF5XXX_RD_SF_FS(PTR)       \
    ( ((*((uint16_t *)(PTR)) & 0x0C00) >> 8) | (*((uint16_t *)(PTR)) & 0x0003) )

#define MCF5XXX_SF_SR(PTR)  *((uint16_t *)(PTR)+1)
#define MCF5XXX_SF_PC(PTR)  *((uint32_t *)(PTR)+1)

/*
 * Functions provided as inline code to access supervisor mode
 * registers from C.
 *
 * Note: Most registers are write-only. So you must use shadow registers in
 *       RAM to track the state of each register!
 */
static __inline__ uint16_t  mcf5xxx_rd_sr(void)            { uint16_t rc; __asm__ __volatile__( "move.w %%sr,%0\n" : "=r" (rc) ); return rc; }
static __inline__ void mcf5xxx_wr_sr(uint16_t value)       { __asm__ __volatile__( "move.w %0,%%sr\n" : : "r" (value) ); }

static __inline__ int asm_set_ipl(uint32_t value)
{
	uint32_t oldipl,newipl;
	value = (value & 0x7) << 8U;
	oldipl = mcf5xxx_rd_sr();
	newipl = oldipl & ~0x0700U;
	newipl |= value;
	mcf5xxx_wr_sr(newipl);
	oldipl = (oldipl & 0x0700U) >> 8U;
	return oldipl;
}

static __inline__ void mcf5xxx_cpushl_bc(uint32_t* value)   { __asm__ __volatile__( " move.l %0,%%a0 \n .word 0xF4E8\n nop\n" : : "a" (value) : "a0"); }
                                                                                                     // cpushl bc,%%a0@ ???

static __inline__ void mcf5xxx_wr_cacr(uint32_t value)     { __asm__ __volatile__( "movec %0,%%cacr\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_asid(uint32_t value)     { __asm__ __volatile__( "movec %0,%%asid\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_acr0(uint32_t value)     { __asm__ __volatile__( "movec %0,#4\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_acr1(uint32_t value)     { __asm__ __volatile__( "movec %0,#5\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_acr2(uint32_t value)     { __asm__ __volatile__( "movec %0,#6\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_acr3(uint32_t value)     { __asm__ __volatile__( "movec %0,#7\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_mmubar(uint32_t value)   { __asm__ __volatile__( "movec %0,%%mmubar\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_other_a7(uint32_t value) { __asm__ __volatile__( "movec %0,%%other_sp\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_vbr(uint32_t value)      { __asm__ __volatile__( "movec %0,%%vbr\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_macsr(uint32_t value)    { __asm__ __volatile__( "movec %0,%%macsr\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_mask(uint32_t value)     { __asm__ __volatile__( "movec %0,%%mask\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_acc0(uint32_t value)     { __asm__ __volatile__( "movec %0,%%acc0\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_accext01(uint32_t value) { __asm__ __volatile__( "movec %0,%%accext01\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_accext23(uint32_t value) { __asm__ __volatile__( "movec %0,%%accext23\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_acc1(uint32_t value)     { __asm__ __volatile__( "movec %0,%%acc1\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_acc2(uint32_t value)     { __asm__ __volatile__( "movec %0,%%acc2\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_acc3(uint32_t value)     { __asm__ __volatile__( "movec %0,%%acc3\n nop\n" : : "r" (value) ); }
//static __inline__ void mcf5xxx_wr_sr(uint32_t value)       { __asm__ __volatile__( "movec %0,%%sr\n nop\n" : : "r" (value) ); }
//static __inline__ void mcf5xxx_wr_pc(uint32_t value)       { __asm__ __volatile__( "movec %0,#0x080F\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_rombar0(uint32_t value)  { __asm__ __volatile__( "movec %0,%%rombar0\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_rombar1(uint32_t value)  { __asm__ __volatile__( "movec %0,%%rombar1\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_rambar0(uint32_t value)  { __asm__ __volatile__( "movec %0,%%rambar0\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_rambar1(uint32_t value)  { __asm__ __volatile__( "movec %0,%%rambar1\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_mpcr(uint32_t value)     { __asm__ __volatile__( "movec %0,%%mpcr\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_secmbar(uint32_t value)  { __asm__ __volatile__( "movec %0,%%mbar1\n nop\n" : : "r" (value) ); }
static __inline__ void mcf5xxx_wr_mbar(uint32_t value)     { __asm__ __volatile__( "movec %0,%%mbar0\n nop\n" : : "r" (value) ); }

#endif

/*
 * Now do specific ColdFire processor
 */

#if   (defined(CONFIG_ARCH_MCF54xx))
#include "asm/coldfire/mcf548x.h"

#else
#error "Error: Yet unsupported ColdFire processor."
#endif


#endif  /* __MCF5XXX__H */
