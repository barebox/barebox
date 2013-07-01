/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#ifndef _DDR_SPD_H_
#define _DDR_SPD_H_

/*
 * Format from "JEDEC Appendix X: Serial Presence Detects for DDR2 SDRAM",
 * SPD Revision 1.2
 */
struct ddr2_spd_eeprom_s {
	uint8_t info_size;   /*  0 # bytes written into serial memory */
	uint8_t chip_size;   /*  1 Total # bytes of SPD memory device */
	uint8_t mem_type;    /*  2 Fundamental memory type */
	uint8_t nrow_addr;   /*  3 # of Row Addresses on this assembly */
	uint8_t ncol_addr;   /*  4 # of Column Addrs on this assembly */
	uint8_t mod_ranks;   /*  5 Number of DIMM Ranks */
	uint8_t dataw;       /*  6 Module Data Width */
	uint8_t res_7;       /*  7 Reserved */
	uint8_t voltage;     /*  8 Voltage intf std of this assembly */
	uint8_t clk_cycle;   /*  9 SDRAM Cycle time @ CL=X */
	uint8_t clk_access;  /* 10 SDRAM Access from Clk @ CL=X (tAC) */
	uint8_t config;      /* 11 DIMM Configuration type */
	uint8_t refresh;     /* 12 Refresh Rate/Type */
	uint8_t primw;       /* 13 Primary SDRAM Width */
	uint8_t ecw;         /* 14 Error Checking SDRAM width */
	uint8_t res_15;      /* 15 Reserved */
	uint8_t burstl;      /* 16 Burst Lengths Supported */
	uint8_t nbanks;      /* 17 # of Banks on Each SDRAM Device */
	uint8_t cas_lat;     /* 18 CAS# Latencies Supported */
	uint8_t mech_char;   /* 19 DIMM Mechanical Characteristics */
	uint8_t dimm_type;   /* 20 DIMM type information */
	uint8_t mod_attr;    /* 21 SDRAM Module Attributes */
	uint8_t dev_attr;    /* 22 SDRAM Device Attributes */
	uint8_t clk_cycle2;  /* 23 Min SDRAM Cycle time @ CL=X-1 */
	uint8_t clk_access2; /* 24 SDRAM Access from Clk @ CL=X-1 (tAC) */
	uint8_t clk_cycle3;  /* 25 Min SDRAM Cycle time @ CL=X-2 */
	uint8_t clk_access3; /* 26 Max Access from Clk @ CL=X-2 (tAC) */
	uint8_t trp;         /* 27 Min Row Precharge Time (tRP)*/
	uint8_t trrd;        /* 28 Min Row Active to Row Active (tRRD) */
	uint8_t trcd;        /* 29 Min RAS to CAS Delay (tRCD) */
	uint8_t tras;        /* 30 Minimum RAS Pulse Width (tRAS) */
	uint8_t rank_dens;   /* 31 Density of each rank on module */
	uint8_t ca_setup;    /* 32 Addr+Cmd Setup Time Before Clk (tIS) */
	uint8_t ca_hold;     /* 33 Addr+Cmd Hold Time After Clk (tIH) */
	uint8_t data_setup;  /* 34 Data Input Setup Time Before Strobe
					(tDS) */
	uint8_t data_hold;   /* 35 Data Input Hold Time
					After Strobe (tDH) */
	uint8_t twr;         /* 36 Write Recovery time tWR */
	uint8_t twtr;        /* 37 Int write to read delay tWTR */
	uint8_t trtp;        /* 38 Int read to precharge delay tRTP */
	uint8_t mem_probe;   /* 39 Mem analysis probe characteristics */
	uint8_t trctrfc_ext; /* 40 Extensions to trc and trfc */
	uint8_t trc;         /* 41 Min Active to Auto refresh time tRC */
	uint8_t trfc;        /* 42 Min Auto to Active period tRFC */
	uint8_t tckmax;      /* 43 Max device cycle time tCKmax */
	uint8_t tdqsq;       /* 44 Max DQS to DQ skew (tDQSQ max) */
	uint8_t tqhs;        /* 45 Max Read DataHold skew (tQHS) */
	uint8_t pll_relock;  /* 46 PLL Relock time */
	uint8_t Tcasemax;    /* 47 Tcasemax */
	uint8_t psiTAdram;   /* 48 Thermal Resistance of DRAM Package
					from Top (Case) to Ambient
					(Psi T-A DRAM) */
	uint8_t dt0_mode;    /* 49 DRAM Case Temperature Rise from
					Ambient due to Activate-Precharge/Mode
					Bits (DT0/Mode Bits) */
	uint8_t dt2n_dt2q;   /* 50 DRAM Case Temperature Rise from
					Ambient due to Precharge/Quiet Standby
					(DT2N/DT2Q) */
	uint8_t dt2p;        /* 51 DRAM Case Temperature Rise from
					Ambient due to Precharge Power-Down
					(DT2P) */
	uint8_t dt3n;        /* 52 DRAM Case Temperature Rise from
					Ambient due to Active Standby (DT3N) */
	uint8_t dt3pfast;    /* 53 DRAM Case Temperature Rise from
					Ambient due to Active Power-Down with
					Fast PDN Exit (DT3Pfast) */
	uint8_t dt3pslow;    /* 54 DRAM Case Temperature Rise from
					Ambient due to Active Power-Down with
					Slow PDN Exit (DT3Pslow) */
	uint8_t dt4r_dt4r4w; /* 55 DRAM Case Temperature Rise from
					Ambient due to Page Open Burst
					Read/DT4R4W Mode Bit
					(DT4R/DT4R4W Mode Bit) */
	uint8_t dt5b;        /* 56 DRAM Case Temperature Rise from
					Ambient due to Burst Refresh (DT5B) */
	uint8_t dt7;         /* 57 DRAM Case Temperature Rise from
					Ambient due to Bank Interleave Reads
					with Auto-Precharge (DT7) */
	uint8_t psiTApll;    /* 58 Thermal Resistance of PLL Package
					form Top (Case) to Ambient
					(Psi T-A PLL) */
	uint8_t psiTAreg;    /* 59 Thermal Reisitance of Register
					Package from Top (Case) to Ambient
					(Psi T-A Register) */
	uint8_t dtpllactive; /* 60 PLL Case Temperature Rise from
					Ambient due to PLL Active
					(DT PLL Active) */
	uint8_t dtregact;    /* 61 Register Case Temperature Rise from
					Ambient due to Register Active/Mode
					Bit (DT Register Active/Mode Bit) */
	uint8_t spd_rev;     /* 62 SPD Data Revision Code */
	uint8_t cksum;       /* 63 Checksum for bytes 0-62 */
	uint8_t mid[8];      /* 64 Mfr's JEDEC ID code per JEP-106 */
	uint8_t mloc;        /* 72 Manufacturing Location */
	uint8_t mpart[18];   /* 73 Manufacturer's Part Number */
	uint8_t rev[2];      /* 91 Revision Code */
	uint8_t mdate[2];    /* 93 Manufacturing Date */
	uint8_t sernum[4];   /* 95 Assembly Serial Number */
	uint8_t mspec[27];   /* 99-127 Manufacturer Specific Data */

};

extern uint32_t ddr2_spd_checksum_pass(const struct ddr2_spd_eeprom_s *spd);

/* * Byte 2 Fundamental Memory Types. */
#define SPD_MEMTYPE_DDR2	(0x08)

/* DIMM Type for DDR2 SPD (according to v1.3) */
#define DDR2_SPD_DIMMTYPE_RDIMM		(0x01)
#define DDR2_SPD_DIMMTYPE_UDIMM		(0x02)
#define DDR2_SPD_DIMMTYPE_SO_DIMM	(0x04)
#define DDR2_SPD_DIMMTYPE_72B_SO_CDIMM	(0x06)
#define DDR2_SPD_DIMMTYPE_72B_SO_RDIMM	(0x07)
#define DDR2_SPD_DIMMTYPE_MICRO_DIMM	(0x08)
#define DDR2_SPD_DIMMTYPE_MINI_RDIMM	(0x10)
#define DDR2_SPD_DIMMTYPE_MINI_UDIMM	(0x20)

#endif /* _DDR_SPD_H_ */
