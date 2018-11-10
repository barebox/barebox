/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <crc.h>
#include <ddr_spd.h>

uint32_t ddr2_spd_checksum_pass(const struct ddr2_spd_eeprom_s *spd)
{
	uint32_t i, cksum = 0;
	const uint8_t *buf = (const uint8_t *)spd;
	uint8_t rev, spd_cksum;

	rev = spd->spd_rev;
	spd_cksum = spd->cksum;

	/* Rev 1.X or less supported by this code */
	if (rev >= 0x20)
		goto error;

	/*
	 * The checksum is calculated on the first 64 bytes
	 * of the SPD as per JEDEC specification.
	 */
	for (i = 0; i < 63; i++)
		cksum += *buf++;
	cksum &= 0xFF;

	if (cksum != spd_cksum)
		goto error;

	return 0;
error:
	return 1;
}

uint32_t ddr3_spd_checksum_pass(const struct ddr3_spd_eeprom_s *spd)
{
	char crc_lsb, crc_msb;
	int csum16, len;

	/*
	 * SPD byte0[7] - CRC coverage
	 * 0 = CRC covers bytes 0~125
	 * 1 = CRC covers bytes 0~116
	 */

	len = !(spd->info_size_crc & 0x80) ? 126 : 117;
	csum16 = cyg_crc16((char *)spd, len);

	crc_lsb = (char) (csum16 & 0xff);
	crc_msb = (char) (csum16 >> 8);

	if (spd->crc[0] != crc_lsb || spd->crc[1] != crc_msb)
		return 1;

	return 0;
}

static char *heights[] = {
	"<25.4",
	"25.4",
	"25.4 - 30.0",
	"30.0",
	"30.5",
	"> 30.5",
};

static char *sdram_voltage_interface_level[] = {
	"TTL (5V tolerant)",
	"LVTTL (not 5V tolerant)",
	"HSTL 1.5V",
	"SSTL 3.3V",
	"SSTL 2.5V",
	"SSTL 1.8V",
};

static char *ddr2_module_types[] = {
	"RDIMM (133.35 mm)",
	"UDIMM (133.25 mm)",
	"SO-DIMM (67.6 mm)",
	"Micro-DIMM (45.5 mm)",
	"Mini-RDIMM (82.0 mm)",
	"Mini-UDIMM (82.0 mm)",
};

static char *refresh[] = {
	"15.625",
	"3.9",
	"7.8",
	"31.3",
	"62.5",
	"125",
};

static char *type_list[] = {
	"Reserved",
	"FPM DRAM",
	"EDO",
	"Pipelined Nibble",
	"SDR SDRAM",
	"Multiplexed ROM",
	"DDR SGRAM",
	"DDR SDRAM",
	[SPD_MEMTYPE_DDR2] = "DDR2 SDRAM",
	"FB-DIMM",
	"FB-DIMM Probe",
	[SPD_MEMTYPE_DDR3] = "DDR3 SDRAM",
};

static int funct(uint8_t addr)
{
	int t;

	t = ((addr >> 4) * 10 + (addr & 0xf));

	return t;
}

static int des(uint8_t byte)
{
	int k;

	k = (byte & 0x3) * 100 / 4;

	return k;
}

static int integ(uint8_t byte)
{
	int k;

	k = (byte >> 2);

	return k;
}

static int ddr2_sdram_ctime(uint8_t byte)
{
	int ctime;

	ctime = (byte >> 4) * 100;
	if ((byte & 0xf) <= 9)
		ctime += (byte & 0xf) * 10;
	else if ((byte & 0xf) == 10)
		ctime += 25;
	else if ((byte & 0xf) == 11)
		ctime += 33;
	else if ((byte & 0xf) == 12)
		ctime += 66;
	else if ((byte & 0xf) == 13)
		ctime += 75;

	return ctime;
}

/*
 * Based on
 * https://github.com/groeck/i2c-tools/blob/master/eeprom/decode-dimms
 */
void ddr_spd_print(uint8_t *record)
{
	int highestCAS = 0;
	int i, i_i, k, x, y;
	int ddrclk, tbits, pcclk;
	int trcd, trp, tras;
	int ctime;
	uint8_t parity;
	char *ref, *sum;
	struct ddr2_spd_eeprom_s *s = (struct ddr2_spd_eeprom_s *)record;

	if (s->mem_type != SPD_MEMTYPE_DDR2) {
		printf("Can't dump information for non-DDR2 memory\n");
		return;
	}

	ctime = ddr2_sdram_ctime(s->clk_cycle);
	ddrclk = 2 * (100000 / ctime);
	tbits = (s->res_7 << 8) + (s->dataw);
	if ((s->config & 0x03) == 1)
		tbits = tbits - 8;

	pcclk = ddrclk * tbits / 8;
	pcclk = pcclk - (pcclk % 100);
	i_i = (s->nrow_addr & 0x0f) + (s->ncol_addr & 0x0f) - 17;
	k = ((s->mod_ranks & 0x7) + 1) * s->nbanks;
	trcd = ((s->trcd >> 2) * 4 + (s->trcd & 3)) * 25 / ctime;
	trp = ((s->trp >> 2) * 4 + (s->trp & 3)) * 25 / ctime;
	tras = s->tras * 100 / ctime ;
	x = (int)(ctime / 100);
	y = (ctime - (int)((ctime / 100) * 100)) / 10;

	for (i_i = 2; i_i < 7; i_i++) {
		if (s->cas_lat & 1 << i_i) {
			highestCAS = i_i;
		}
	}

	if (ddr2_spd_checksum_pass(s))
		sum = "ERR";
	else
		sum = "OK";

	printf("---=== SPD EEPROM Information ===---\n");
	printf("%-48s %s (0x%02X)\n", "EEPROM Checksum of bytes 0-62",
		sum, s->cksum);
	printf("%-48s %d\n", "# of bytes written to SDRAM EEPROM",
		s->info_size);
	printf("%-48s %d\n", "Total number of bytes in EEPROM",
		1 << (s->chip_size));

	if (s->mem_type < ARRAY_SIZE(type_list))
		printf("%-48s %s\n", "Fundamental Memory type",
			type_list[s->mem_type]);
	else
		printf("%-48s (%02x)\n", "Warning: unknown memory type",
			s->mem_type);

	printf("%-48s %x.%x\n", "SPD Revision", s->spd_rev >> 4,
		s->spd_rev & 0x0f);

	printf("\n---=== Memory Characteristics ===---\n");
	printf("%-48s %d MHz (PC2-%d)\n", "Maximum module speed",
		ddrclk, pcclk);
	if (i_i > 0 && i_i <= 12 && k > 0)
		printf("%-48s %d MB\n", "Size", (1 << i_i) * k);
	else
		printf("%-48s INVALID: %02x %02x %02x %02x\n", "Size",
			s->nrow_addr, s->ncol_addr, s->mod_ranks, s->nbanks);

	printf("%-48s %d x %d x %d x %d\n", "Banks x Rows x Columns x Bits",
		s->nbanks, s->nrow_addr, s->ncol_addr, s->dataw);
	printf("%-48s %d\n", "Ranks", (s->mod_ranks & 0x7) + 1);
	printf("%-48s %d bits\n", "SDRAM Device Width", s->primw);

	if (s->mod_ranks >> 5 < ARRAY_SIZE(heights))
		printf("%-48s %s mm\n", "Module Height",
			heights[s->mod_ranks >> 5]);
	else
		printf("Error height\n");

	if ((fls(s->dimm_type) - 1) < ARRAY_SIZE(ddr2_module_types))
		printf("%-48s %s\n", "Module Type",
			ddr2_module_types[fls(s->dimm_type) - 1]);
	else
		printf("Error module type\n");

	printf("%-48s ", "DRAM Package ");
	if ((s->mod_ranks & 0x10) == 1)
		printf("Stack\n");
	else
		printf("Planar\n");
	if (s->voltage < ARRAY_SIZE(sdram_voltage_interface_level))
		printf("%-48s %s\n", "Voltage Interface Level",
			sdram_voltage_interface_level[s->voltage]);
	else
		printf("Error Voltage Interface Level\n");

	printf("%-48s ", "Module Configuration Type ");

	parity = s->config & 0x07;
	if (parity == 0)
		printf("No Parity\n");

	if ((parity & 0x03) == 0x01)
		printf("Data Parity\n");
	if (parity & 0x02)
		printf("Data ECC\n");

	if (parity & 0x04)
		printf("Address/Command Parity\n");

	if ((s->refresh >> 7) == 1)
		ref = "- Self Refresh";
	else
		ref = " ";

	printf("%-48s Reduced (%s us) %s\n", "Refresh Rate",
		refresh[s->refresh & 0x7f], ref);
	printf("%-48s %d, %d\n", "Supported Burst Lengths",
		s->burstl & 4, s->burstl & 8);

	printf("%-48s %dT\n", "Supported CAS Latencies (tCL)", highestCAS);
	printf("%-48s %d-%d-%d-%d as DDR2-%d\n", "tCL-tRCD-tRP-tRAS",
		highestCAS, trcd, trp, tras, ddrclk);
	printf("%-48s %d.%d ns at CAS %d\n", "Minimum Cycle Time", x, y,
		highestCAS);
	printf("%-48s 0.%d%d ns at CAS %d\n", "Maximum Access Time",
		(s->clk_access >> 4), (s->clk_access & 0xf), highestCAS);
	printf("%-48s %d ns\n", "Maximum Cycle Time (tCK max)",
		(s->tckmax >> 4) + (s->tckmax & 0x0f));

	printf("\n---=== Timing Parameters ===---\n");
	printf("%-48s 0.%d ns\n",
		"Address/Command Setup Time Before Clock (tIS)",
		funct(s->ca_setup));
	printf("%-48s 0.%d ns\n", "Address/Command Hold Time After Clock (tIH)",
		funct(s->ca_hold));
	printf("%-48s 0.%d%d ns\n", "Data Input Setup Time Before Strobe (tDS)",
		s->data_setup >> 4, s->data_setup & 0xf);
	printf("%-48s 0.%d%d ns\n", "Data Input Hold Time After Strobe (tDH)",
		s->data_hold >> 4, s->data_hold & 0xf);

	printf("%-48s %d.%02d ns\n", "Minimum Row Precharge Delay (tRP)",
		integ(s->trp), des(s->trp));
	printf("%-48s %d.%02d ns\n",
		"Minimum Row Active to Row Active Delay (tRRD)",
		integ(s->trrd), des(s->trrd));
	printf("%-48s %d.%02d ns\n", "Minimum RAS# to CAS# Delay (tRCD)",
		integ(s->trcd), des(s->trcd));
	printf("%-48s %d.00 ns\n", "Minimum RAS# Pulse Width (tRAS)",
		((s->tras & 0xfc) + (s->tras & 0x3)));
	printf("%-48s %d.%02d ns\n", "Write Recovery Time (tWR)",
		integ(s->twr), des(s->twr));
	printf("%-48s %d.%02d ns\n", "Minimum Write to Read CMD Delay (tWTR)",
		integ(s->twtr), des(s->twtr));
	printf("%-48s %d.%02d ns\n",
		"Minimum Read to Pre-charge CMD Delay (tRTP)",
		integ(s->trtp), des(s->trtp));
	printf("%-48s %d.00 ns\n", "Minimum Active to Auto-refresh Delay (tRC)",
		s->trc);
	printf("%-48s %d ns\n", "Minimum Recovery Delay (tRFC)", s->trfc);
	printf("%-48s 0.%d ns\n", "Maximum DQS to DQ Skew (tDQSQ)", s->tdqsq);
	printf("%-48s 0.%d ns\n", "Maximum Read Data Hold Skew (tQHS)",
		s->tqhs);

	printf("\n---=== Manufacturing Information ===---\n");

	printf("%-48s", "Manufacturer JEDEC ID");
	for (i = 64; i < 72; i++)
		printf(" %02x", record[i]);

	printf("\n");
	if (s->mloc)
		printf("%-48s 0x%02x\n", "Manufacturing Location Code",
			s->mloc);

	printf("%-48s ", "Part Number");
	for (i = 73; i < 91; i++) {
		if (record[i] >= 32 && record[i] < 127)
			printf("%c", record[i]);
		else
			printf("%d", record[i]);
	}
	printf("\n");
	printf("%-48s 20%d-W%d\n", "Manufacturing Date", record[93],
		record[94]);
	printf("%-48s 0x", "Assembly Serial Number");
	for (i = 95; i < 99; i++)
		printf("%02X", record[i]);
	printf("\n");
}
