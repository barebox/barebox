/*
 * For clock initialization, see chapter 3 of the "MCIMX27 Multimedia
 * Applications Processor Reference Manual, Rev. 0.2".
 *
 */

#include <config.h>
#include <mach/imx-regs.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];
#define IIM_BASE_ADDR           0x53FF0000
#define SDRAM_BASE_ADDR             CSD0_BASE_ADDR
#define CSD0_BASE_ADDR          0x80000000
#define IIM_SREV_OFF            0x24
#define AIPS1_CTRL_BASE_ADDR    AIPS1_BASE_ADDR
#define AIPS1_BASE_ADDR         0x43F00000
#define AIPS2_CTRL_BASE_ADDR    AIPS2_BASE_ADDR
#define AIPS2_BASE_ADDR         0x53F00000
#define MAX_BASE_ADDR           0x43F04000
#define CLKCTL_BASE_ADDR        0x43F0C000
#define ESDCTL_BASE             0xB8001000
#define M3IF_BASE               0xB8003000
#define IOMUXC_BASE_ADDR        0x43FAC000
#define MPCTL_PARAM_399     (((1-1) << 26) + ((16-1) << 16) + (8  << 10) + (5 << 0))
#define MPCTL_PARAM_532     ((1 << 31) + ((1-1) << 26) + ((12-1) << 16) + (11  << 10) + (1 << 0))
#define PPCTL_PARAM_300     (((1-1) << 26) + ((4-1) << 16) + (6  << 10) + (1  << 0))
#define SDRAM_SIZE                  0x08000000
#define CCM_BASE_ADDR           0x53F80000
#define IPU_CTRL_BASE_ADDR      0x53FC0000
#define WEIM_CTRL_CS5           (WEIM_BASE_ADDR + 0x50)
#define WEIM_BASE_ADDR          0xB8002000
#define WEIM_CTRL_CS0           WEIM_BASE_ADDR

ARM_PPMRR:              .word   0x40000015
L2CACHE_PARAM:          .word   0x00030024
IIM_SREV_REG_VAL:       .word   IIM_BASE_ADDR + IIM_SREV_OFF
AIPS1_CTRL_BASE_ADDR_W: .word   AIPS1_CTRL_BASE_ADDR
AIPS2_CTRL_BASE_ADDR_W: .word   AIPS2_CTRL_BASE_ADDR
AIPS1_PARAM_W:          .word   0x77777777
MAX_BASE_ADDR_W:        .word   MAX_BASE_ADDR
MAX_PARAM1:             .word   0x00302154
CLKCTL_BASE_ADDR_W:     .word   CLKCTL_BASE_ADDR
ESDCTL_BASE_W:          .word   ESDCTL_BASE
M3IF_BASE_W:            .word   M3IF_BASE
RAM_PARAM1_MDDR:	.word	0x00000400
RAM_PARAM2_MDDR:	.word	0x00000333
RAM_PARAM3_MDDR:	.word	0x02000400
			.word	0x02000000
RAM_PARAM4_MDDR:	.word	0x04000000
RAM_PARAM5_MDDR:	.word	0x06000000
RAM_PARAM6_MDDR:	.word	0x00000233
			.word	0x00000033
RAM_PARAM7_MDDR:	.word	0x02000780
ESDCTL_0x92220000:      .word   0x92220000
ESDCTL_0xA2220000:      .word   0xA2220000
ESDCTL_0xB2220000:      .word   0xB2220000
ESDCTL_0x82226080:      .word   0x82226080
ESDCTL_CONFIG:       	.word   0x007FFC3F	//DDR2
			.word 	0x00295729	//MDDR
ESDCTL_DELAY5:		.word   0x00F49F00
IOMUXC_BASE_ADDR_W:     .word   IOMUXC_BASE_ADDR
CCM_CCMR_W:             .word   0x003F4208
//CCM_PDR0_W:             .word   0x00801000
CCM_PDR0_W:             .word   0x00801c00
MPCTL_PARAM_399_W:      .word   MPCTL_PARAM_399
MPCTL_PARAM_532_W:      .word   MPCTL_PARAM_532
PPCTL_PARAM_W:    	.word   PPCTL_PARAM_300
MXC_REDBOOT_ROM_START:  .word   SDRAM_BASE_ADDR + SDRAM_SIZE - 0x100000
CONST_0x0FFF:           .word   0x0FFF
CCM_BASE_ADDR_W:        .word   CCM_BASE_ADDR
IPU_CTRL_BASE_ADDR_W:   .word   IPU_CTRL_BASE_ADDR
WEIM_CTRL_CS5_W:    .word   WEIM_CTRL_CS5
WEIM_CTRL_CS0_W:    .word   WEIM_CTRL_CS0
CS0_CSCRU_0x0000CC03:   .word   0x0000DCF6
CS0_CSCRL_0xA0330D01:   .word   0x444A4541
CS0_CSCRA_0x00220800:   .word   0x44443302
CS5_CSCRU_0x0000D843:   .word   0x0000D843
CS5_CSCRL_0x22252521:   .word   0x22252521
CS5_CSCRA_0x22220A00:   .word   0x22220A00

#define L2CC_BASE_ADDR          0x30000000
#define L2_CACHE_CTL_REG                0x100
#define L2_CACHE_AUX_CTL_REG            0x104
#define L2_CACHE_DBG_CTL_REG      0xF40
#define L2_CACHE_INV_WAY_REG            0x77C

/* Assuming 24MHz input clock */
/*                            PD             MFD              MFI          MFN */
#define MPCTL_PARAM_399     (((1-1) << 26) + ((16-1) << 16) + (8  << 10) + (5 << 0))
#define MPCTL_PARAM_532     ((1 << 31) + ((1-1) << 26) + ((12-1) << 16) + (11  << 10) + (1 << 0))
#define MPCTL_PARAM_665     (((1-1) << 26) + ((48-1) << 16) + (13  << 10) + (41 << 0))
#define PPCTL_PARAM_300     (((1-1) << 26) + ((4-1) << 16) + (6  << 10) + (1  << 0))

#define M3IF_BASE               0xB8003000

#define UNALIGNED_ACCESS_ENABLE
#define LOW_INT_LATENCY_ENABLE
#define BRANCH_PREDICTION_ENABLE

.globl board_init_lowlevel
board_init_lowlevel:
	mov     r10, lr

    mrc 15, 0, r1, c1, c0, 0
    bic r1, r1, #(0x3<<21)
    bic r1, r1, #(0x3<<11)
    bic r1, r1, #0x5

    bic r1, r1, #(1<<3)

#ifndef BRANCH_PREDICTION_ENABLE
    mrc 15, 0, r0, c1, c0, 1
    bic r0, r0, #7
    mcr 15, 0, r0, c1, c0, 1
#else
    mrc 15, 0, r0, c1, c0, 1
    orr r0, r0, #7
    mcr 15, 0, r0, c1, c0, 1
    orr r1, r1, #(1<<11)
#endif

#ifdef UNALIGNED_ACCESS_ENABLE
    orr r1, r1, #(1<<22)
#endif

#ifdef LOW_INT_LATENCY_ENABLE
    orr r1, r1, #(1<<21)
#endif
    mcr 15, 0, r1, c1, c0, 0

#ifdef BRANCH_PREDICTION_ENABLE
    mov r0, #0
    mcr 15, 0, r0, c15, c2, 4
#endif

    mov r0, #0
    mcr 15, 0, r0, c7, c7, 0        /* invalidate I cache and D cache */
    mcr 15, 0, r0, c8, c7, 0        /* invalidate TLBs */
    mcr 15, 0, r0, c7, c10, 4       /* Drain the write buffer */

    /* Also setup the Peripheral Port Remap register inside the core */
    ldr r0, ARM_PPMRR        /* start from AIPS 2GB region */
    mcr p15, 0, r0, c15, c2, 4

    /*** L2 Cache setup/invalidation/disable ***/
    /* Disable L2 cache first */
    mov r0, #L2CC_BASE_ADDR
    ldr r2, [r0, #L2_CACHE_CTL_REG]
    bic r2, r2, #0x1
    str r2, [r0, #L2_CACHE_CTL_REG]
    /*
     * Configure L2 Cache:
     * - 128k size(16k way)
     * - 8-way associativity
     * - 0 ws TAG/VALID/DIRTY
     * - 4 ws DATA R/W
     */
    ldr r1, [r0, #L2_CACHE_AUX_CTL_REG]
    and r1, r1, #0xFE000000
    ldr r2, L2CACHE_PARAM
    orr r1, r1, r2
    str r1, [r0, #L2_CACHE_AUX_CTL_REG]
/* Workaournd for DDR issue:WT*/
    ldr r1, [r0, #L2_CACHE_DBG_CTL_REG]
    orr r1, r1, #2
    str r1, [r0, #L2_CACHE_DBG_CTL_REG]

    /* Invalidate L2 */
    mov r1, #0x000000FF
    str r1, [r0, #L2_CACHE_INV_WAY_REG]
L2_loop:
    /* Poll Invalidate By Way register */
    ldr r2, [r0, #L2_CACHE_INV_WAY_REG]
    cmp r2, #0
    bne L2_loop
    /*** End of L2 operations ***/

/*
 * End of ARM1136 init
 */

        /*
         * Set all MPROTx to be non-bufferable, trusted for R/W,
         * not forced to user-mode.
         */
        ldr r0, AIPS1_CTRL_BASE_ADDR_W
        ldr r1, AIPS1_PARAM_W
        str r1, [r0, #0x00]
        str r1, [r0, #0x04]
        ldr r0, AIPS2_CTRL_BASE_ADDR_W
        str r1, [r0, #0x00]
        str r1, [r0, #0x04]

        /*
         * Clear the on and off peripheral modules Supervisor Protect bit
         * for SDMA to access them. Did not change the AIPS control registers
         * (offset 0x20) access type
         */
        ldr r0, AIPS1_CTRL_BASE_ADDR_W
        ldr r1, =0x0
        str r1, [r0, #0x40]
        str r1, [r0, #0x44]
        str r1, [r0, #0x48]
        str r1, [r0, #0x4C]
        ldr r1, [r0, #0x50]
        and r1, r1, #0x00FFFFFF
        str r1, [r0, #0x50]

        ldr r0, AIPS2_CTRL_BASE_ADDR_W
        ldr r1, =0x0
        str r1, [r0, #0x40]
        str r1, [r0, #0x44]
        str r1, [r0, #0x48]
        str r1, [r0, #0x4C]
        ldr r1, [r0, #0x50]
        and r1, r1, #0x00FFFFFF
        str r1, [r0, #0x50]

        ldr r0, MAX_BASE_ADDR_W
        /* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
        ldr r1, MAX_PARAM1
        str r1, [r0, #0x000]        /* for S0 */
        str r1, [r0, #0x100]        /* for S1 */
        str r1, [r0, #0x200]        /* for S2 */
        str r1, [r0, #0x300]        /* for S3 */
        str r1, [r0, #0x400]        /* for S4 */
        /* SGPCR - always park on last master */
        ldr r1, =0x10
        str r1, [r0, #0x010]        /* for S0 */
        str r1, [r0, #0x110]        /* for S1 */
        str r1, [r0, #0x210]        /* for S2 */
        str r1, [r0, #0x310]        /* for S3 */
        str r1, [r0, #0x410]        /* for S4 */
        /* MGPCR - restore default values */
        ldr r1, =0x0
        str r1, [r0, #0x800]        /* for M0 */
        str r1, [r0, #0x900]        /* for M1 */
        str r1, [r0, #0xA00]        /* for M2 */
        str r1, [r0, #0xB00]        /* for M3 */
        str r1, [r0, #0xC00]        /* for M4 */
        str r1, [r0, #0xD00]        /* for M5 */

	ldr r1, M3IF_BASE_W
	/*
	 * M3IF Control Register (M3IFCTL)
	 * MRRP[0] = L2CC0 not on priority list (0 << 0)	= 0x00000000
	 * MRRP[1] = MAX1 not on priority list (0 << 0)		= 0x00000000
	 * MRRP[2] = L2CC1 not on priority list (0 << 0)	= 0x00000000
	 * MRRP[3] = USB  not on priority list (0 << 0)		= 0x00000000
	 * MRRP[4] = SDMA not on priority list (0 << 0)		= 0x00000000
	 * MRRP[5] = GPU not on priority list (0 << 0)		= 0x00000000
	 * MRRP[6] = IPU1 on priority list (1 << 6)		= 0x00000040
	 * MRRP[7] = IPU2 not on priority list (0 << 0)		= 0x00000000
	 *                                                       ------------
	 *                                                        0x00000040
	 */
	ldr r0, =0x00000040
	str r0, [r1]  /* M3IF control reg */

#if 1
	ldr r0, CCM_BASE_ADDR_W

	/* default CLKO to 1/32 of the ARM core*/
	ldr r1, [r0, #CCM_COSR]
	bic r1, r1, #0x00000FF00
	bic r1, r1, #0x0000000FF
	mov r2, #0x00006C00
	add r2, r2, #0x67
	orr r1, r1, r2
	str r1, [r0, #CCM_COSR]

	ldr r2, CCM_CCMR_W
	str r2, [r0, #CCM_CCMR]

	/* check clock path */
	ldr r2, [r0, #CCM_PDR0]
	tst r2, #0x1
	ldrne r3, MPCTL_PARAM_532_W  /* consumer path*/
	ldreq r3, MPCTL_PARAM_399_W  /* auto path*/

	/*Set MPLL , arm clock and ahb clock*/
	str r3, [r0, #CCM_MPCTL]

	ldr r1, PPCTL_PARAM_W
	str r1, [r0, #CCM_PPCTL]

	ldr r1, [r0, #CCM_PDR0]
	orr r1, r1, #0x800000
	str r1, [r0, #CCM_PDR0]

	ldr r1, CCM_PDR0_W
	str r1, [r0, #CCM_PDR0]

	ldr r1, [r0, #CCM_CGR0]
	orr r1, r1, #0x00300000
	str r1, [r0, #CCM_CGR0]

	ldr r1, [r0, #CCM_CGR1]
	orr r1, r1, #0x00000C00
	orr r1, r1, #0x00000003
	str r1, [r0, #CCM_CGR1]
#endif

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #0x80000000
	bls	1f
	cmp	pc, #0x90000000
	bhi	1f

	mov	pc, lr

1:
	ldr r0, ESDCTL_BASE_W
	mov r3, #0x2000
	str r3, [r0, #0x0]
	str r3, [r0, #0x8]

	mov r12, #0x00
	mov r2, #0x00
	mov r1, #IMX_SDRAM_CS0

	ldr r0, ESDCTL_BASE_W
	mov r3, #0x2000
	str r3, [r0, #0x0]
	str r3, [r0, #0x8]

	mov r12, #0x00
	mov r2, #0x00
	mov r1, #IMX_SDRAM_CS0
	bl setup_sdram_bank
	cmp r3, #0x0
	orreq r12, r12, #1
	eorne r2, r2, #0x1
	blne setup_sdram_bank

	cmp r12, #0
	movne r3, #L2CC_BASE_ADDR
	ldrne r4, [r3, #L2_CACHE_AUX_CTL_REG]
	orrne r4, r4, #0x1000
	strne r4, [r3, #L2_CACHE_AUX_CTL_REG]

	ldr r3, ESDCTL_DELAY5
	str r3, [r0, #0x30]
ret:
	mov	pc,r10

/*
 * r0: control base, r1: ram bank base
 * r2: ddr type(0:DDR2, 1:MDDR) r3, r4: working
 */
setup_sdram_bank:

	mov r3, #0xE /*0xA + 0x4*/
	tst r2, #0x1
	orreq r3, r3, #0x300 /*DDR2*/
	str r3, [r0, #0x10]
	bic r3, r3, #0x00A
	str r3, [r0, #0x10]
	beq 2f

	mov r3, #0x20000
1:	subs r3, r3, #1
	bne 1b

2:      adr r4, ESDCTL_CONFIG
	tst r2, #0x1
	ldreq r3, [r4, #0x0]
	ldrne r3, [r4, #0x4]
	cmp r1, #IMX_SDRAM_CS1
        strlo r3, [r0, #0x4]
        strhs r3, [r0, #0xC]

        ldr r3, ESDCTL_0x92220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
	mov r3, #0xDA
        ldr r4, RAM_PARAM1_MDDR
        strb r3, [r1, r4]

	tst r2, #0x1
	bne skip_set_mode

	cmp r1, #IMX_SDRAM_CS1
	ldr r3, ESDCTL_0xB2220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
	mov r3, #0xDA
	ldr r4, RAM_PARAM4_MDDR
        strb r3, [r1, r4]
        ldr r4, RAM_PARAM5_MDDR
        strb r3, [r1, r4]
        ldr r4, RAM_PARAM3_MDDR
        strb r3, [r1, r4]
        ldr r4, RAM_PARAM2_MDDR
        strb r3, [r1, r4]

        ldr r3, ESDCTL_0x92220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
	mov r3, #0xDA
        ldr r4, RAM_PARAM1_MDDR
        strb r3, [r1, r4]

skip_set_mode:
	cmp r1, #IMX_SDRAM_CS1
        ldr r3, ESDCTL_0xA2220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
        mov r3, #0xDA
        strb r3, [r1]
        strb r3, [r1]

        ldr r3, ESDCTL_0xB2220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
	adr r4, RAM_PARAM6_MDDR
	tst r2, #0x1
	ldreq r4, [r4, #0x0]
	ldrne r4, [r4, #0x4]
        mov r3, #0xDA
        strb r3, [r1, r4]
        ldreq r4, RAM_PARAM7_MDDR
        streqb r3, [r1, r4]
	adr r4, RAM_PARAM3_MDDR
	ldreq r4, [r4, #0x0]
	ldrne r4, [r4, #0x4]
        strb r3, [r1, r4]

	cmp r1, #IMX_SDRAM_CS1
        ldr r3, ESDCTL_0x82226080
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]

	tst r2, #0x1
	moveq r4, #0x20000
	movne r4, #0x200
1:	subs r4, r4, #1
	bne 1b

	str r3, [r1, #0x100]
	ldr r4, [r1, #0x100]
	cmp r3, r4
	movne r3, #1
	moveq r3, #0
	mov pc, lr

