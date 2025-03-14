// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <crc.h>
#include <ddr_spd.h>
#include <pbl/i2c.h>

/* used for ddr1 and ddr2 spd */
static int spd_check(const u8 *buf, u8 spd_rev, u8 spd_cksum)
{
	unsigned int cksum = 0;
	unsigned int i;

	/*
	 * Check SPD revision supported
	 * Rev 1.X or less supported by this code
	 */
	if (spd_rev >= 0x20) {
		printf("SPD revision %02X not supported by this code\n",
		       spd_rev);
		return 1;
	}
	if (spd_rev > 0x13) {
		printf("SPD revision %02X not verified by this code\n",
		       spd_rev);
	}

	/*
	 * Calculate checksum
	 */
	for (i = 0; i < 63; i++)
		cksum += *buf++;

	cksum &= 0xFF;

	if (cksum != spd_cksum) {
		printf("SPD checksum unexpected. "
			"Checksum in SPD = %02X, computed SPD = %02X\n",
			spd_cksum, cksum);
		return -EBADMSG;
	}

	return 0;
}

int ddr1_spd_check(const struct ddr1_spd_eeprom *spd)
{
	const u8 *p = (const u8 *)spd;

	return spd_check(p, spd->spd_rev, spd->cksum);
}

int ddr2_spd_check(const struct ddr2_spd_eeprom *spd)
{
	const u8 *p = (const u8 *)spd;

	return spd_check(p, spd->spd_rev, spd->cksum);
}

int ddr3_spd_check(const struct ddr3_spd_eeprom *spd)
{
	char *p = (char *)spd;
	int csum16;
	int len;
	unsigned char crc_lsb;	/* byte 126 */
	unsigned char crc_msb;	/* byte 127 */

	/*
	 * SPD byte0[7] - CRC coverage
	 * 0 = CRC covers bytes 0~125
	 * 1 = CRC covers bytes 0~116
	 */

	len = !(spd->info_size_crc & 0x80) ? 126 : 117;
	csum16 = crc_itu_t(0, p, len);

	crc_lsb = csum16 & 0xff;
	crc_msb = csum16 >> 8;

	if (spd->crc[0] == crc_lsb && spd->crc[1] == crc_msb) {
		return 0;
	} else {
		printf("SPD checksum unexpected.\n"
			"Checksum lsb in SPD = %02X, computed SPD = %02X\n"
			"Checksum msb in SPD = %02X, computed SPD = %02X\n",
			spd->crc[0], crc_lsb, spd->crc[1], crc_msb);
		return -EBADMSG;
	}
}

int ddr4_spd_check(const struct ddr4_spd_eeprom *spd)
{
	char *p = (char *)spd;
	int csum16;
	int len;
	unsigned char crc_lsb;	/* byte 126 */
	unsigned char crc_msb;	/* byte 127 */

	len = 126;
	csum16 = crc_itu_t(0, p, len);

	crc_lsb = csum16 & 0xff;
	crc_msb = csum16 >> 8;

	if (spd->crc[0] != crc_lsb || spd->crc[1] != crc_msb) {
		printf("SPD checksum unexpected.\n"
			"Checksum lsb in SPD = %02X, computed SPD = %02X\n"
			"Checksum msb in SPD = %02X, computed SPD = %02X\n",
			spd->crc[0], crc_lsb, spd->crc[1], crc_msb);
		return -EBADMSG;
	}

	p = (char *)((ulong)spd + 128);
	len = 126;
	csum16 = crc_itu_t(0, p, len);

	crc_lsb = csum16 & 0xff;
	crc_msb = csum16 >> 8;

	if (spd->mod_section.uc[126] != crc_lsb ||
	    spd->mod_section.uc[127] != crc_msb) {
		printf("SPD checksum unexpected.\n"
			"Checksum lsb in SPD = %02X, computed SPD = %02X\n"
			"Checksum msb in SPD = %02X, computed SPD = %02X\n",
			spd->mod_section.uc[126],
			crc_lsb, spd->mod_section.uc[127],
			crc_msb);
		return -EBADMSG;
	}

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

static void spd_print_manufacturing_date(uint8_t year, uint8_t week)
{
	/*
	 * According to JEDEC Standard the year/week bytes must be in BCD
	 * format. However, that is not always true for actual DIMMs out
	 * there, so fall back to binary format if it makes more sense.
	 */

	printf("%-48s ", "Manufacturing Date");
	if ((year & 0xf0) <= 0x90 && (year & 0xf) <= 0x9
	    && (week & 0xf0) <= 0x90 && (week & 0xf) <= 0x9) {
		printf("20%02X-W%02X", year, week);
	} else if (year <= 99 && week >= 1 && week <= 53) {
		printf("20%02d-W%02d", year, week);
	} else {
		printf("0x%02X%02X", year, week);
	}
	printf("\n");
}

/*
 * Based on
 * https://github.com/groeck/i2c-tools/blob/master/eeprom/decode-dimms
 */
static void ddr2_spd_print(uint8_t *record)
{
	int highestCAS = 0;
	int i, i_i, k, x, y;
	int ddrclk, tbits, pcclk;
	int trcd, trp, tras;
	int ctime;
	uint8_t parity;
	char *ref, *sum;
	struct ddr2_spd_eeprom *s = (struct ddr2_spd_eeprom *)record;

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

	if (ddr2_spd_check(s))
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
	if ((s->mod_ranks & 0x10) != 0)
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
	spd_print_manufacturing_date(record[93], record[94]);
	printf("%-48s 0x", "Assembly Serial Number");
	for (i = 95; i < 99; i++)
		printf("%02X", record[i]);
	printf("\n");
}

static const char * const ddr3_spd_moduletypes[] = {
	"Undefined",
	"RDIMM",
	"UDIMM",
	"SO-DIMM",
	"Micro-DIMM",
	"Mini-RDIMM",
	"Mini-UDIMM",
	"Mini-CDIMM",
	"72b-SO-UDIMM",
	"72b-SO-RDIMM",
	"72b-SO-CDIMM",
	"LRDIMM",
	"16b-SO-DIMM",
	"32b-SO-DIMM"
};

static const char * const ddr3_spd_moduletypes_width[] = {
	"Unknown",
	"133.35 mm",
	"133.35 mm",
	"67.6 mm",
	"TBD",
	"82.0 mm",
	"82.0 mm",
	"67.6 mm",
	"67.6 mm",
	"67.6 mm",
	"67.6 mm",
	"133.35 mm",
	"67.6 mm",
	"67.6 mm"
};

#define DDR3_UNBUFFERED		1
#define DDR3_REGISTERED		2
#define DDR3_CLOCKED		3
#define DDR3_LOAD_REDUCED	4

static const uint8_t ddr3_spd_moduletypes_family[] = {
	0,
	DDR3_REGISTERED,
	DDR3_UNBUFFERED,
	DDR3_UNBUFFERED,
	DDR3_UNBUFFERED,
	DDR3_REGISTERED,
	DDR3_UNBUFFERED,
	DDR3_CLOCKED,
	DDR3_UNBUFFERED,
	DDR3_REGISTERED,
	DDR3_CLOCKED,
	DDR3_LOAD_REDUCED,
	DDR3_UNBUFFERED,
	DDR3_UNBUFFERED
};

static const int ddr3_std_speeds[] = {
	1000 * 7.5 / 8,
	1000 * 7.5 / 7,
	1000 * 1.25,
	1000 * 1.5,
	1000 * 1.875,
	1000 * 2.5
};

static const char * const ddr3_maximum_activated_counts[] = {
	"Untested",
	"700 K",
	"600 K",
	"500 K",
	"400 K",
	"300 K",
	"200 K",
	"Reserved",
	"Unlimited",
};

static int ddr3_timing_from_mtb_ftb(uint16_t txx, int8_t fine_txx,
				    uint8_t mtb_dividend, uint8_t mtb_divisor,
				    uint8_t ftb_div)
{
	/*
	 * Given mtb in ns and ftb in ps, return the result in ps,
	 * carefully rounding to the nearest picosecond.
	 */
	int result = txx * 10000 * mtb_dividend / mtb_divisor
		     + fine_txx * (ftb_div >> 4) / (ftb_div & 0xf);
	return DIV_ROUND_CLOSEST(result, 10);
}

static int ddr3_adjust_ctime(int ctime, uint8_t ftb_div)
{
	uint8_t ftb_divisor = ftb_div >> 4;
	uint8_t ftb_dividend = ftb_div & 0xf;
	int i;

	/*
	 * Starting with DDR3-1866, vendors may start approximating the
	 * minimum cycle time. Try to guess what they really meant so
	 * that the reported speed matches the standard.
	 */
	for (i = 7; i < 15; i++) {
		int test = ctime * ftb_divisor - (int)(1000 * 7.5) * ftb_divisor / i;

		if (test > -(int)ftb_dividend && test < ftb_dividend)
			return (int)(1000 * 7.5) / i;
	}

	return ctime;
}

static void ddr3_print_reference_card(uint8_t ref_raw_card, uint8_t mod_height)
{
	const char alphabet[] = "ABCDEFGHJKLMNPRTUVWY";
	uint8_t ref = ref_raw_card & 0x1f;
	uint8_t revision = mod_height >> 5;
	char ref_card[3] = { 0 };

	if (ref == 0x1f) {
		printf("%-48s %s\n", "Module Reference Card", "ZZ");
		return;
	}

	if (ref_raw_card & 0x80)
		ref += 0x1f;
	if (!revision)
		revision = ((ref_raw_card >> 5) & 0x3);

	if (ref < ARRAY_SIZE(alphabet) - 1) {
		/* One letter reference card */
		ref_card[0] = alphabet[ref];
	} else {
		/* Two letter reference card */
		uint8_t ref1 = ref / (ARRAY_SIZE(alphabet) - 1);

		ref -= (ARRAY_SIZE(alphabet) - 1) * ref1;
		ref_card[0] = alphabet[ref1];
		ref_card[1] = alphabet[ref];
	}

	printf("%-48s %s revision %u\n", "Module Reference Card", ref_card, revision);
}

static void ddr3_print_revision_number(const char *name, uint8_t rev)
{
	uint8_t h = rev >> 4;
	uint8_t l = rev & 0xf;

	/* Decode as suggested by JEDEC Standard 21-C */
	if (!h)
		printf("%-48s %d\n", name, l);
	if (h < 0xa)
		printf("%-48s %d.%d\n", name, h, l);
	else
		printf("%-48s %c%d\n", name, 'A' + h - 0xa, l);
}

static void ddr3_spd_print(uint8_t *record)
{
	struct ddr3_spd_eeprom *s = (struct ddr3_spd_eeprom *)record;
	const char *sum;
	uint8_t spd_len;
	int size, bytes_written;
	int ctime, ddrclk;
	uint8_t tbits;
	int pcclk;
	uint8_t cap, k;
	int taa, trcd, trp, tras;
	uint16_t cas_sup;
	int twr, trrd, trc, trfc, twtr, trtp, tfaw;
	uint8_t die_count, loading;
	uint8_t mac;
	uint8_t family;
	int i;

	sum = ddr3_spd_check(s) ? "ERR" : "OK";

	printf("\n---=== SPD EEPROM Information ===---\n");

	printf("EEPROM CRC of bytes 0-%3d %22s %s (0x%02X%02X)\n",
	       (s->info_size_crc & 0x80) ? 116 : 125, "", sum, s->crc[1], s->crc[0]);

	spd_len = (s->info_size_crc >> 4) & 0x7;
	size = 64 << (s->info_size_crc & 0xf);
	if (spd_len == 0) {
		bytes_written = 128;
	} else if (spd_len == 1) {
		bytes_written = 176;
	} else if (spd_len == 2) {
		bytes_written = 256;
	} else {
		size = 64;
		bytes_written = 64;
	}
	printf("%-48s %d\n", "# of bytes written to SDRAM EEPROM", bytes_written);
	printf("%-48s %d\n", "Total number of bytes in EEPROM", size);
	printf("%-48s %s\n", "Fundamental Memory type", type_list[s->mem_type]);

	if (s->spd_rev != 0xff)
		printf("%-48s %x.%x\n", "SPD Revision", s->spd_rev >> 4,
		       s->spd_rev & 0xf);

	printf("%-48s ", "Module Type");
	if (s->module_type <= ARRAY_SIZE(ddr3_spd_moduletypes))
		printf("%s\n", ddr3_spd_moduletypes[s->module_type]);
	else
		printf("Reserved (0x%02x)\n", s->module_type);

	if ((s->ftb_div & 0x0f) == 0 || s->mtb_divisor == 0) {
		printf("Invalid time base divisor, can't decode\n");
		return;
	}

	printf("\n---=== Memory Characteristics ===---\n");

	ctime = ddr3_timing_from_mtb_ftb(s->tck_min, s->fine_tck_min,
					 s->mtb_dividend, s->mtb_divisor, s->ftb_div);
	ctime = ddr3_adjust_ctime(ctime, s->ftb_div);

	ddrclk = 2 * 1000 * 1000 / ctime;
	tbits = 1 << ((s->bus_width & 0x7) + 3);
	pcclk = ddrclk * tbits / 8;
	pcclk = pcclk - (pcclk % 100);	/* Round down to comply with JEDEC */
	printf("%-48s %d MT/s (PC3-%d)\n", "Maximum module speed", ddrclk, pcclk);

	cap = (s->density_banks & 0xf) + 28 + (s->bus_width & 0x7) + 3
	      - ((s->organization & 0x7) + 2) - (20 + 3);
	k = (s->organization >> 3) + 1;
	printf("%-48s %d MB\n", "Size", (1 << cap) * k);

	printf("%-48s %d x %d x %d x %d\n", "Banks x Rows x Columns x Bits",
	       1 << (((s->density_banks >> 4) & 0x7) + 3),
	       (((s->addressing >> 3) & 0x1f) + 12),
	       ((s->addressing & 7) + 9),
	       (1 << ((s->bus_width & 0x7) + 3)));

	printf("%-48s %d\n", "Ranks", k);

	printf("%-48s %d bits\n", "SDRAM Device Width",
	       (1 << ((s->organization & 0x7) + 2)));

	printf("%-48s %d bits\n", "Primary Bus Width",
	       (8 << (s->bus_width & 0x7)));
	if (s->bus_width & 0x18)
		printf("%-48s %d bits\n", "Bus Width Extension", s->bus_width & 0x18);

	taa = ddr3_timing_from_mtb_ftb(s->taa_min, s->fine_taa_min,
				       s->mtb_dividend, s->mtb_divisor, s->ftb_div);
	trcd = ddr3_timing_from_mtb_ftb(s->trcd_min, s->fine_trcd_min,
					s->mtb_dividend, s->mtb_divisor, s->ftb_div);
	trp = ddr3_timing_from_mtb_ftb(s->trp_min, s->fine_trp_min,
				       s->mtb_dividend, s->mtb_divisor, s->ftb_div);
	tras = (((s->tras_trc_ext & 0xf) << 8) + s->tras_min_lsb)
	       * 1000 * s->mtb_dividend / s->mtb_divisor;

	printf("%-48s %d-%d-%d-%d\n", "tCL-tRCD-tRP-tRAS",
	       DIV_ROUND_UP(taa, ctime), DIV_ROUND_UP(trcd, ctime),
	       DIV_ROUND_UP(trp, ctime), DIV_ROUND_UP(tras, ctime));

	printf("%-48s", "Supported CAS Latencies (tCL)");
	cas_sup = (s->caslat_msb << 8) + s->caslat_lsb;
	for (i = 14; i >= 0; i--) {
		if (cas_sup & (1 << i))
			printf(" %dT", i + 4);
	}
	printf("\n");

	printf("\n---=== Timings at Standard Speeds ===---\n");

	for (i = 0; i < ARRAY_SIZE(ddr3_std_speeds); i++) {
		int ctime_at_speed = ddr3_std_speeds[i];
		int best_cas = 0;
		int j;

		/* Find min CAS latency at this speed */
		for (j = 14; j >= 0; j--) {
			if (!(cas_sup & (1 << j)))
				continue;
			if (DIV_ROUND_UP(taa, ctime_at_speed) <= j + 4) {
				best_cas = j + 4;
			}
		}

		if (!best_cas || ctime_at_speed < ctime)
			continue;

		printf("tCL-tRCD-tRP-tRAS as DDR3-%-4d %17s %d-%d-%d-%d\n",
		       2000 * 1000 / ctime_at_speed, "", best_cas,
		       DIV_ROUND_UP(trcd, ctime_at_speed),
		       DIV_ROUND_UP(trp, ctime_at_speed),
		       DIV_ROUND_UP(tras, ctime_at_speed));
	}

	printf("\n---=== Timing Parameters ===---\n");

	printf("%-48s %d.%03d ns\n", "Minimum Cycle Time (tCK)",
	       ctime / 1000, ctime % 1000);
	printf("%-48s %d.%03d ns\n", "Minimum CAS Latency Time (tAA)",
	       taa / 1000, taa % 1000);
	twr = s->twr_min * 1000 * s->mtb_dividend / s->mtb_divisor;
	printf("%-48s %d.%03d ns\n", "Minimum Write Recovery time (tWR)",
	       twr / 1000, twr % 1000);
	printf("%-48s %d.%03d ns\n", "Minimum RAS# to CAS# Delay (tRCD)",
	       trcd / 1000, trcd % 1000);
	trrd = s->trrd_min * 1000 * s->mtb_dividend / s->mtb_divisor;
	printf("%-48s %d.%03d ns\n", "Minimum Row Active to Row Active Delay (tRRD)",
	       trrd / 1000, trrd % 1000);
	printf("%-48s %d.%03d ns\n", "Minimum Row Precharge Delay (tRP)",
	       trp / 1000, trp % 1000);
	printf("%-48s %d.%03d ns\n", "Minimum Active to Precharge Delay (tRAS)",
	       tras / 1000, tras % 1000);
	trc = ddr3_timing_from_mtb_ftb(((s->tras_trc_ext & 0xf0) << 4) + s->trc_min_lsb,
				       s->fine_trc_min, s->mtb_dividend, s->mtb_divisor,
				       s->ftb_div);
	printf("%-48s %d.%03d ns\n", "Minimum Active to Auto-Refresh Delay (tRC)",
	       trc / 1000, trc % 1000);
	trfc = ((s->trfc_min_msb << 8) + s->trfc_min_lsb) * 1000
	       * s->mtb_dividend / s->mtb_divisor;
	printf("%-48s %d.%03d ns\n", "Minimum Recovery Delay (tRFC)",
	       trfc / 1000, trfc % 1000);
	twtr = s->twtr_min * 1000 * s->mtb_dividend / s->mtb_divisor;
	printf("%-48s %d.%03d ns\n", "Minimum Write to Read CMD Delay (tWTR)",
	       twtr / 1000, twtr % 1000);
	trtp = s->trtp_min * 1000 * s->mtb_dividend / s->mtb_divisor;
	printf("%-48s %d.%03d ns\n", "Minimum Read to Pre-charge CMD Delay (tRTP)",
	       trtp / 1000, trtp % 1000);
	tfaw = (((s->tfaw_msb & 0xf) << 8) + s->tfaw_min) * 1000
	       * s->mtb_dividend / s->mtb_divisor;
	printf("%-48s %d.%03d ns\n", "Minimum Four Activate Window Delay (tFAW)",
	       tfaw / 1000, tfaw % 1000);

	printf("\n---=== Optional Features ===---\n");

	printf("%-48s 1.5V%s%s%s\n", "Operable voltages",
	       (s->module_vdd & 0x1) ? " tolerant" : "",
	       (s->module_vdd & 0x2) ? ", 1.35V" : "",
	       (s->module_vdd & 0x4) ? ", 1.2X V" : "");
	printf("%-48s %s\n", "RZQ/6 supported?",
	       (s->opt_features & 1) ? "Yes" : "No");
	printf("%-48s %s\n", "RZQ/7 supported?",
	       (s->opt_features & 2) ? "Yes" : "No");
	printf("%-48s %s\n", "DLL-Off Mode supported?",
	       (s->opt_features & 0x80) ? "Yes" : "No");
	printf("%-48s 0-%d degrees C\n", "Operating temperature range",
	       (s->therm_ref_opt & 0x1) ? 95 : 85);
	if (s->therm_ref_opt & 0x1)
		printf("%-48s %s\n", "Refresh Rate in extended temp range",
		       (s->therm_ref_opt & 0x2) ? "1X" : "2X");
	printf("%-48s %s\n", "Auto Self-Refresh?",
	       (s->therm_ref_opt & 0x4) ? "Yes" : "No");
	printf("%-48s %s\n", "On-Die Thermal Sensor readout?",
	       (s->therm_ref_opt & 0x8) ? "Yes" : "No");
	printf("%-48s %s\n", "Partial Array Self-Refresh?",
	       (s->therm_ref_opt & 0x80) ? "Yes" : "No");
	printf("%-48s %s\n", "Module Thermal Sensor",
	       (s->therm_sensor & 0x80) ? "Yes" : "No");

	printf("%-48s %s\n", "SDRAM Device Type",
	       (s->device_type & 0x80) ? "Non-Standard" : "Standard Monolithic");

	die_count = (s->device_type >> 4) & 0x7;
	if (die_count == 1)
		printf("%-48s Single die\n", "");
	else if (die_count == 2)
		printf("%-48s 2 die\n", "");
	else if (die_count == 3)
		printf("%-48s 4 die\n", "");
	else if (die_count == 4)
		printf("%-48s 8 die\n", "");

	loading = (s->device_type >> 2) & 0x3;
	if (loading == 1)
		printf("%-48s Multi load stack\n", "");
	else if (loading == 2)
		printf("%-48s Single load stack\n", "");

	mac = s->res_39_59[2] & 0xf;
	if (mac < ARRAY_SIZE(ddr3_maximum_activated_counts))
		printf("%-48s %s\n", "Maximum Activate Count (MAC)",
		       ddr3_maximum_activated_counts[mac]);

	/*
	 * The following bytes are type-specific, so don't continue if the type
	 * is unknown.
	 */
	if (!s->module_type || s->module_type > ARRAY_SIZE(ddr3_spd_moduletypes))
		return;

	family = ddr3_spd_moduletypes_family[s->module_type];
	if (family == DDR3_UNBUFFERED || family == DDR3_REGISTERED
	    || family == DDR3_CLOCKED || family == DDR3_LOAD_REDUCED) {
		printf("\n---=== Physical Characteristics ===---\n");
		printf("%-48s %d mm\n", "Module Height",
		       ((s->mod_section.unbuffered.mod_height & 0x1f) + 15));
		printf("%-48s %d mm front, %d mm back\n", "Module Thickness",
		       (s->mod_section.unbuffered.mod_thickness & 0xf) + 1,
		       ((s->mod_section.unbuffered.mod_thickness >> 4) & 0xf) + 1);
		printf("%-48s %s\n", "Module Width",
		       ddr3_spd_moduletypes_width[s->module_type]);
		ddr3_print_reference_card(s->mod_section.unbuffered.ref_raw_card,
					  s->mod_section.unbuffered.mod_height);
	}

	if (family == DDR3_UNBUFFERED)
		printf("%-48s %s\n", "Rank 1 Mapping",
		       (s->mod_section.unbuffered.addr_mapping) ? "Mirrored" : "Standard");

	if (family == DDR3_REGISTERED) {
		uint8_t rows = (s->mod_section.registered.modu_attr >> 2) & 0x3;
		uint8_t registers = s->mod_section.registered.modu_attr & 0x3;

		printf("\n---=== Registered DIMM ===---\n");

		if (!rows)
			printf("%-48s Undefined\n", "# DRAM Rows");
		else
			printf("%-48s %u\n", "# DRAM Rows", 1 << (rows - 1));

		if (!registers)
			printf("%-48s Undefined\n", "# Registers");
		else
			printf("%-48s %u\n", "# Registers", 1 << (registers - 1));
		printf("%-48s 0x%02x%02x\n", "Register manufacturer ID",
		       s->mod_section.registered.reg_id_hi,
		       s->mod_section.registered.reg_id_lo);
		printf("%-48s %s\n", "Register device type",
		       (!s->mod_section.registered.reg_type) ? "SSTE32882" : "Undefined");
		if (s->mod_section.registered.reg_rev != 0xff)
			ddr3_print_revision_number("Register revision",
						   s->mod_section.registered.reg_rev);
		printf("%-48s %s\n", "Heat spreader",
		       (s->mod_section.registered.thermal & 0x80) ? "Yes" : "No");
	}

	if (family == DDR3_LOAD_REDUCED) {
		uint8_t modu_attr = s->mod_section.loadreduced.modu_attr;
		uint8_t rows = (modu_attr >> 2) & 0x3;
		static const char *mirroring[] = {
			"None", "Odd ranks", "Reserved", "Reserved"
		};

		printf("\n---=== Load Reduced DIMM ===---\n");

		if (!rows)
			printf("%-48s Undefined\n", "# DRAM Rows");
		else if (rows == 0x3)
			printf("%-48s Reserved\n", "# DRAM Rows");
		else
			printf("%-48s %u\n", "# DRAM Rows", 1 << (rows - 1));

		printf("%-48s %s\n", "Mirroring",
		       mirroring[modu_attr & 0x3]);

		printf("%-48s %s\n", "Rank Numbering",
		       (modu_attr & 0x20) ? "Even only" : "Contiguous");
		printf("%-48s %s\n", "Buffer Orientation",
		       (modu_attr & 0x10) ? "Horizontal" : "Vertical");
		printf("%-48s 0x%02x%02x\n", "Register Manufacturer ID",
			s->mod_section.loadreduced.buf_id_hi,
			s->mod_section.loadreduced.buf_id_lo);
		if (s->mod_section.loadreduced.buf_rev_id != 0xff)
			ddr3_print_revision_number("Buffer Revision",
						   s->mod_section.loadreduced.buf_rev_id);
		printf("%-48s %s\n", "Heat spreader",
		       (s->mod_section.loadreduced.modu_attr & 0x80) ? "Yes" : "No");
	}

	printf("\n---=== Manufacturer Data ===---\n");

	printf("%-48s 0x%02x%02x\n", "Module Manufacturer ID",
	       s->mmid_msb, s->mmid_lsb);

	if (!((s->dmid_lsb == 0xff) && (s->dmid_msb == 0xff))
	    && !((s->dmid_lsb == 0x0) && (s->dmid_msb == 0x0)))
		printf("%-48s 0x%02x%02x\n", "DRAM Manufacturer ID",
		       s->dmid_msb, s->dmid_lsb);

	if (!(s->mloc == 0xff) && !(s->mloc == 0x0))
		printf("%-48s 0x%02x\n", "Manufacturing Location Code",
		       s->mloc);

	if (!((s->mdate[0] == 0xff) && (s->mdate[1] == 0xff))
	    && !((s->mdate[0] == 0x0) && (s->mdate[1] == 0x0)))
		spd_print_manufacturing_date(s->mdate[0], s->mdate[1]);

	if (memcmp(s->sernum, "\xFF\xFF\xFF\xFF", 4) &&
	    memcmp(s->sernum, "\x00\x00\x00\x00", 4))
		printf("%-48s 0x%02X%02X%02X%02X\n", "Assembly Serial Number",
		       s->sernum[0], s->sernum[1], s->sernum[2], s->sernum[3]);

	printf("%-48s ", "Part Number");
	if (!(s->mpart[0] >= 32 && s->mpart[0] < 127)) {
		printf("Undefined\n");
	} else {
		for (i = 0; i < ARRAY_SIZE(s->mpart); i++) {
			if (s->mpart[i] >= 32 && s->mpart[i] < 127)
				printf("%c", s->mpart[i]);
			else
				break;
		}
		printf("\n");
	}

	if (!((s->mrev[0] == 0xff) && (s->mrev[1] == 0xff))
	    && !((s->mrev[0] == 0x0) && (s->mrev[1] == 0x0)))
		printf("%-48s 0x%02X%02X\n", "Revision Code", s->mrev[0], s->mrev[1]);
}

void ddr_spd_print(uint8_t *record)
{
	uint8_t mem_type = record[2];

	switch (mem_type) {
	case SPD_MEMTYPE_DDR2:
		ddr2_spd_print(record);
		break;
	case SPD_MEMTYPE_DDR3:
		ddr3_spd_print(record);
		break;
	default:
		printf("Can only dump SPD information for DDR2 and DDR3 memory types\n");
	}
}

#define SPD_SPA0_ADDRESS        0x36
#define SPD_SPA1_ADDRESS        0x37

static int select_page(struct pbl_i2c *i2c, uint8_t addr)
{
	struct i2c_msg msg = {
		.addr = addr,
		.len = 0,
	};
	int ret;

	ret = pbl_i2c_xfer(i2c, &msg, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static int read_buf(struct pbl_i2c *i2c,
		    uint8_t addr, int page, void *buf)
{
	uint8_t pos = 0;
	int ret;
	struct i2c_msg msg[2] = {
		{
			.addr = addr,
			.len = 1,
			.buf = &pos,
		}, {
			.addr = addr,
			.len = 256,
			.flags = I2C_M_RD,
			.buf = buf,
		}
	};

	ret = select_page(i2c, page);
	if (ret < 0)
		return ret;

	ret = pbl_i2c_xfer(i2c, msg, 2);
	if (ret < 0)
		return ret;

	return 0;
}

/**
 * spd_read_eeprom - Read contents of a SPD EEPROM
 * @i2c: I2C controller handle
 * @addr: I2C bus address for the EEPROM
 * @buf: buffer to read the SPD data to
 * @memtype: Memory type, such as SPD_MEMTYPE_DDR4
 *
 * This function takes a I2C message transfer function and reads the contents
 * from a SPD EEPROM to the buffer provided at @buf. The buffer should at least
 * have a size of 512 bytes. Returns 0 for success or a negative error code
 * otherwise.
 */
int spd_read_eeprom(struct pbl_i2c *i2c,
		    uint8_t addr, void *buf,
		    int memtype)
{
	int ret;

	if (memtype == SPD_MEMTYPE_DDR4) {
		ret = read_buf(i2c, addr, SPD_SPA0_ADDRESS, buf);
		if (ret < 0)
			return ret;

		ret = read_buf(i2c, addr, SPD_SPA1_ADDRESS, buf + 256);
		if (ret < 0)
			return ret;
	} else {
		ret = read_buf(i2c, addr, 0, buf);
		if (ret < 0)
			return ret;
	}

	return 0;
}
