#ifndef STM32_LTDC_H__
#define STM32_LTDC_H__

/* LTDC main registers */
#define LTDC_IDR	0x00	/* IDentification */
#define LTDC_LCR	0x04	/* Layer Count */
#define LTDC_SSCR	0x08	/* Synchronization Size Configuration */
#define LTDC_BPCR	0x0C	/* Back Porch Configuration */
#define LTDC_AWCR	0x10	/* Active Width Configuration */
#define LTDC_TWCR	0x14	/* Total Width Configuration */
#define LTDC_GCR	0x18	/* Global Control */
#define LTDC_GC1R	0x1C	/* Global Configuration 1 */
#define LTDC_GC2R	0x20	/* Global Configuration 2 */
#define LTDC_SRCR	0x24	/* Shadow Reload Configuration */
#define LTDC_GACR	0x28	/* GAmma Correction */
#define LTDC_BCCR	0x2C	/* Background Color Configuration */
#define LTDC_IER	0x34	/* Interrupt Enable */
#define LTDC_ISR	0x38	/* Interrupt Status */
#define LTDC_ICR	0x3C	/* Interrupt Clear */
#define LTDC_LIPCR	0x40	/* Line Interrupt Position Conf. */
#define LTDC_CPSR	0x44	/* Current Position Status */
#define LTDC_CDSR	0x48	/* Current Display Status */

/* LTDC layer 1 registers */
#define LTDC_L1LC1R	0x80	/* L1 Layer Configuration 1 */
#define LTDC_L1LC2R	0x84	/* L1 Layer Configuration 2 */
#define LTDC_L1CR	0x84	/* L1 Control */
#define LTDC_L1WHPCR	0x88	/* L1 Window Hor Position Config */
#define LTDC_L1WVPCR	0x8C	/* L1 Window Vert Position Config */
#define LTDC_L1CKCR	0x90	/* L1 Color Keying Configuration */
#define LTDC_L1PFCR	0x94	/* L1 Pixel Format Configuration */
#define LTDC_L1CACR	0x98	/* L1 Constant Alpha Config */
#define LTDC_L1DCCR	0x9C	/* L1 Default Color Configuration */
#define LTDC_L1BFCR	0xA0	/* L1 Blend Factors Configuration */
#define LTDC_L1FBBCR	0xA4	/* L1 FrameBuffer Bus Control */
#define LTDC_L1AFBCR	0xA8	/* L1 AuxFB Control */
#define LTDC_L1CFBAR	0xAC	/* L1 Color FrameBuffer Address */
#define LTDC_L1CFBLR	0xB0	/* L1 Color FrameBuffer Length */
#define LTDC_L1CFBLNR	0xB4	/* L1 Color FrameBuffer Line Nb */
#define LTDC_L1AFBAR	0xB8	/* L1 AuxFB Address */
#define LTDC_L1AFBLR	0xBC	/* L1 AuxFB Length */
#define LTDC_L1AFBLNR	0xC0	/* L1 AuxFB Line Number */
#define LTDC_L1CLUTWR	0xC4	/* L1 CLUT Write */

/* Bit definitions */
#define SSCR_VSH	GENMASK(10, 0)	/* Vertical Synchronization Height */
#define SSCR_HSW	GENMASK(27, 16)	/* Horizontal Synchronization Width */

#define BPCR_AVBP	GENMASK(10, 0)	/* Accumulated Vertical Back Porch */
#define BPCR_AHBP	GENMASK(27, 16)	/* Accumulated Horizontal Back Porch */

#define AWCR_AAH	GENMASK(10, 0)	/* Accumulated Active Height */
#define AWCR_AAW	GENMASK(27, 16)	/* Accumulated Active Width */

#define TWCR_TOTALH	GENMASK(10, 0)	/* TOTAL Height */
#define TWCR_TOTALW	GENMASK(27, 16)	/* TOTAL Width */

#define GCR_LTDCEN	BIT(0)		/* LTDC ENable */
#define GCR_DEN		BIT(16)		/* Dither ENable */
#define GCR_PCPOL	BIT(28)		/* Pixel Clock POLarity-Inverted */
#define GCR_DEPOL	BIT(29)		/* Data Enable POLarity-High */
#define GCR_VSPOL	BIT(30)		/* Vertical Synchro POLarity-High */
#define GCR_HSPOL	BIT(31)		/* Horizontal Synchro POLarity-High */

#define GC1R_WBCH	GENMASK(3, 0)	/* Width of Blue CHannel output */
#define GC1R_WGCH	GENMASK(7, 4)	/* Width of Green Channel output */
#define GC1R_WRCH	GENMASK(11, 8)	/* Width of Red Channel output */
#define GC1R_PBEN	BIT(12)		/* Precise Blending ENable */
#define GC1R_DT		GENMASK(15, 14)	/* Dithering Technique */
#define GC1R_GCT	GENMASK(19, 17)	/* Gamma Correction Technique */
#define GC1R_SHREN	BIT(21)		/* SHadow Registers ENabled */
#define GC1R_BCP	BIT(22)		/* Background Colour Programmable */
#define GC1R_BBEN	BIT(23)		/* Background Blending ENabled */
#define GC1R_LNIP	BIT(24)		/* Line Number IRQ Position */
#define GC1R_TP		BIT(25)		/* Timing Programmable */
#define GC1R_IPP	BIT(26)		/* IRQ Polarity Programmable */
#define GC1R_SPP	BIT(27)		/* Sync Polarity Programmable */
#define GC1R_DWP	BIT(28)		/* Dither Width Programmable */
#define GC1R_STREN	BIT(29)		/* STatus Registers ENabled */
#define GC1R_BMEN	BIT(31)		/* Blind Mode ENabled */

#define GC2R_EDCA	BIT(0)		/* External Display Control Ability  */
#define GC2R_STSAEN	BIT(1)		/* Slave Timing Sync Ability ENabled */
#define GC2R_DVAEN	BIT(2)		/* Dual-View Ability ENabled */
#define GC2R_DPAEN	BIT(3)		/* Dual-Port Ability ENabled */
#define GC2R_BW		GENMASK(6, 4)	/* Bus Width (log2 of nb of bytes) */
#define GC2R_EDCEN	BIT(7)		/* External Display Control ENabled */

#define SRCR_IMR	BIT(0)		/* IMmediate Reload */
#define SRCR_VBR	BIT(1)		/* Vertical Blanking Reload */

#define LXCR_LEN	BIT(0)		/* Layer ENable */
#define LXCR_COLKEN	BIT(1)		/* Color Keying Enable */
#define LXCR_CLUTEN	BIT(4)		/* Color Look-Up Table ENable */

#define LXWHPCR_WHSTPOS	GENMASK(11, 0)	/* Window Horizontal StarT POSition */
#define LXWHPCR_WHSPPOS	GENMASK(27, 16)	/* Window Horizontal StoP POSition */

#define LXWVPCR_WVSTPOS	GENMASK(10, 0)	/* Window Vertical StarT POSition */
#define LXWVPCR_WVSPPOS	GENMASK(26, 16)	/* Window Vertical StoP POSition */

#define LXPFCR_PF	GENMASK(2, 0)	/* Pixel Format */

#define LXCACR_CONSTA	GENMASK(7, 0)	/* CONSTant Alpha */

#define LXBFCR_BF2	GENMASK(2, 0)	/* Blending Factor 2 */
#define LXBFCR_BF1	GENMASK(10, 8)	/* Blending Factor 1 */

#define LXCFBLR_CFBLL	GENMASK(12, 0)	/* Color Frame Buffer Line Length */
#define LXCFBLR_CFBP	GENMASK(28, 16)	/* Color Frame Buffer Pitch in bytes */

#define LXCFBLNR_CFBLN	GENMASK(10, 0)	/* Color Frame Buffer Line Number */

#define BF1_PAXCA	0x600		/* Pixel Alpha x Constant Alpha */
#define BF1_CA		0x400		/* Constant Alpha */
#define BF2_1PAXCA	0x007		/* 1 - (Pixel Alpha x Constant Alpha) */
#define BF2_1CA		0x005		/* 1 - Constant Alpha */

enum stm32_ltdc_pixfmt {
	PF_ARGB8888 = 0,
	PF_RGB888,
	PF_RGB565,
	PF_ARGB1555,
	PF_ARGB4444,
	PF_L8,
	PF_AL44,
	PF_AL88
};

#endif
