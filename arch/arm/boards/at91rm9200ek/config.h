#ifndef __CONFIG_H
#define __CONFIG_H

#define MASTER_PLL_MUL		39
#define MASTER_PLL_DIV		4

/* clocks */
#define CONFIG_SYS_MOR_VAL						\
		(AT91_PMC_MOSCEN |					\
		 (255 << 8))		/* Main Oscillator Start-up Time */
#define CONFIG_SYS_PLLAR_VAL						\
		(AT91_PMC_PLLA_WR_ERRATA | /* Bit 29 must be 1 when prog */ \
		 (0x3e << 8) |		/* PLL Counter */		\
		 (0 << 14) |		/* Divider A */			\
		 ((MASTER_PLL_MUL - 1) << 16) | (MASTER_PLL_DIV))

#define CONFIG_SYS_PLLBR_VAL	0x10483E0E /* 48.054857 MHz (divider by 2 for USB) */
/* PCK/2 = MCK Master Clock from SLOW */
#define	CONFIG_SYS_MCKR2_VAL1		\
		(AT91_PMC_CSS_SLOW |	\
		 AT91RM9200_PMC_MDIV_2) \

/* PCK/3 = MCK Master Clock = 59.904000MHz from PLLA */
#define	CONFIG_SYS_MCKR2_VAL2		\
		(AT91_PMC_CSS_PLLA |	\
		 AT91_PMC_PRES_1 |	\
		 AT91RM9200_PMC_MDIV_3 |\
		 AT91_PMC_PDIV_1)

/* flash */
#define CONFIG_SYS_EBI_CFGR_VAL	0x00000000
#define CONFIG_SYS_SMC_CSR0_VAL							\
		(AT91RM9200_SMC_NWS_(4) |	/* Number of Wait States */		\
		 AT91RM9200_SMC_WSEN |	/* Wait State Enable */			\
		 AT91RM9200_SMC_TDF_(2) |	/* Data Float Time */			\
		 AT91RM9200_SMC_BAT |		/* Byte Access Type */			\
		 AT91RM9200_SMC_DBW_16)	/* Data Bus Width */

/* sdram */
#define CONFIG_SYS_PIOC_ASR_VAL	0xFFFF0000 /* Configure PIOC as peripheral (D16/D31) */
#define CONFIG_SYS_PIOC_BSR_VAL	0x00000000
#define CONFIG_SYS_PIOC_PDR_VAL	0xFFFF0000
#define CONFIG_SYS_EBI_CSA_VAL							\
		(AT91RM9200_EBI_CS0A_SMC |					\
		 AT91RM9200_EBI_CS1A_SDRAMC |					\
		 AT91RM9200_EBI_CS3A_SMC |					\
		 AT91RM9200_EBI_CS4A_SMC)					\

/* SDRAM */
/* SDRAMC_MR Mode register */
/* SDRAMC_CR - Configuration register*/
#define CONFIG_SYS_SDRC_CR_VAL							\
		(AT91RM9200_SDRAMC_NC_9 |					\
		 AT91RM9200_SDRAMC_NR_12 |					\
		 AT91RM9200_SDRAMC_NB_4 |					\
		 AT91RM9200_SDRAMC_CAS_2 |					\
		 (1 <<  8) |		/* Write Recovery Delay */		\
		 (12 << 12) |		/* Row Cycle Delay */			\
		 (8 << 16) |		/* Row Precharge Delay */		\
		 (8 << 20) |		/* Row to Column Delay */		\
		 (1 << 24) |		/* Active to Precharge Delay */		\
		 (2 << 28))		/* Exit Self Refresh to Active Delay */

#define CONFIG_SYS_SDRC_TR_VAL	0x000002E0 /* Write refresh rate */

#endif	/* __CONFIG_H */
