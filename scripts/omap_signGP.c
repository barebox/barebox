/**
 * signGP.c - Read the x-load.bin file and write out the x-load.bin.ift file
 *
 * The signed image is the original pre-pended with the size of the image
 * and the load address.  If not entered on command line, file name is
 * assumed to be x-load.bin in current directory and load address is
 * 0x40200800.
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <getopt.h>
#include <linux/types.h>

#undef CH_WITH_CHRAM
struct chsettings {
	__u32 section_key;
	__u8 valid;
	__u8 version;
	__u16 reserved;
	__u32 flags;
} __attribute__ ((__packed__));

/*    __u32  cm_clksel_core;
    __u32  reserved1;
    __u32  cm_autoidle_dpll_mpu;
    __u32  cm_clksel_dpll_mpu;
    __u32  cm_div_m2_dpll_mpu;
    __u32  cm_autoidle_dpll_core;
    __u32  cm_clksel_dpll_core;
    __u32  cm_div_m2_dpll_core;
    __u32  cm_div_m3_dpll_core;
    __u32  cm_div_m4_dpll_core;
    __u32  cm_div_m5_dpll_core;
    __u32  cm_div_m6_dpll_core;
    __u32  cm_div_m7_dpll_core;
    __u32  cm_autoidle_dpll_per;
    __u32  cm_clksel_dpll_per;
    __u32  cm_div_m2_dpll_per;
    __u32  cm_div_m3_dpll_per;
    __u32  cm_div_m4_dpll_per;
    __u32  cm_div_m5_dpll_per;
    __u32  cm_div_m6_dpll_per;
    __u32  cm_div_m7_dpll_per;
    __u32  cm_autoidle_dpll_usb;
    __u32  cm_clksel_dpll_usb;
    __u32  cm_div_m2_dpll_usb;
}*/

struct gp_header {
	__u32 size;
	__u32 load_addr;
} __attribute__ ((__packed__));

struct ch_toc {
	__u32 section_offset;
	__u32 section_size;
	__u8 unused[12];
	__u8 section_name[12];
} __attribute__ ((__packed__));

struct chram {
	/* CHRAM */
	__u32 section_key_chr;
	__u8 section_disable_chr;
	__u8 pad_chr[3];
	/* EMIF1 */
	__u32 config_emif1;
	__u32 refresh_emif1;
	__u32 tim1_emif1;
	__u32 tim2_emif1;
	__u32 tim3_emif1;
	__u32 pwrControl_emif1;
	__u32 phy_cntr1_emif1;
	__u32 phy_cntr2_emif1;
	__u8 modereg1_emif1;
	__u8 modereg2_emif1;
	__u8 modereg3_emif1;
	__u8 pad_emif1;
	/* EMIF2 */
	__u32 config_emif2;
	__u32 refresh_emif2;
	__u32 tim1_emif2;
	__u32 tim2_emif2;
	__u32 tim3_emif2;
	__u32 pwrControl_emif2;
	__u32 phy_cntr1_emif2;
	__u32 phy_cntr2_emif2;
	__u8 modereg1_emif2;
	__u8 modereg2_emif2;
	__u8 modereg3_emif2;
	__u8 pad_emif2;

	__u32 dmm_lisa_map;
	__u8 flags;
	__u8 pad[3];
} __attribute__ ((__packed__));


struct ch_chsettings_chram {
	struct ch_toc toc_chsettings;
	struct ch_toc toc_chram;
	struct ch_toc toc_terminator;
	struct chsettings section_chsettings;
	struct chram section_chram;
	__u8 padding1[512 -
		    (sizeof(struct ch_toc) * 3 +
		     sizeof(struct chsettings) + sizeof(struct chram))];
	/* struct gp_header gpheader; */
} __attribute__ ((__packed__));

struct ch_chsettings_nochram {
	struct ch_toc toc_chsettings;
	struct ch_toc toc_terminator;
	struct chsettings section_chsettings;
	__u8 padding1[512 -
		    (sizeof(struct ch_toc) * 2 +
		     sizeof(struct chsettings))];
	/* struct gp_header gpheader; */
} __attribute__ ((__packed__));


#ifdef CH_WITH_CHRAM
static const struct ch_chsettings_chram config_header = {
	/* CHSETTINGS TOC */
	{sizeof(struct ch_toc) * 4,
	 sizeof(struct chsettings),
	 "",
	 {"CHSETTINGS"}
	 },
	/* CHRAM TOC */
	{sizeof(struct ch_toc) * 4 + sizeof(struct chsettings),
	 sizeof(struct chram),
	 "",
	 {"CHRAM"}
	 },
	/* toc terminator */
	{0xFFFFFFFF,
	 0xFFFFFFFF,
	 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF},
	 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF}
	 },
	/* CHSETTINGS section */
	{
	 0xC0C0C0C1,
	 0,
	 1,
	 0,
	 0},
	/* CHRAM section */
	{
	 0xc0c0c0c2,
	 0x01,
	 {0x00, 0x00, 0x00},

	 /* EMIF1 */
	 0x80800eb2,
	 0x00000010,
	 0x110d1624,
	 0x3058161b,
	 0x030060b2,
	 0x00000200,
	 0x901ff416,
	 0x00000000,
	 0x23,
	 0x01,
	 0x02,
	 0x00,

	 /* EMIF2 */
	 0x80800eb2,
	 0x000002ba,
	 0x110d1624,
	 0x3058161b,
	 0x03006542,
	 0x00000200,
	 0x901ff416,
	 0x00000000,
	 0x23,
	 0x01,
	 0x02,
	 0x00,

	 /* LISA map */
	 0x80700100,
	 0x05,
	 {0x00, 0x00, 0x00},
	 },
	""
};
#else
static struct ch_chsettings_nochram config_header
	__attribute__((section(".config_header"))) = {
	/* CHSETTINGS TOC */
	{(sizeof(struct ch_toc)) * 2,
	 sizeof(struct chsettings),
	 "",
	 {"CHSETTINGS"}
	 },
	/* toc terminator */
	{0xFFFFFFFF,
	 0xFFFFFFFF,
	 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF},
	 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF}
	 },
	/* CHSETTINGS section */
	{
	 0xC0C0C0C1,
	 0,
	 1,
	 0,
	 0},
	""
};
#endif

static void usage(const char *prgname)
{
	fprintf(stderr,
"usage: %s [OPTIONS] <infile>\n"
"\n"
"Options:\n"
"-o <outfile>     output to <outfile>\n"
"-l <loadaddr>    specify load address\n"
"-c               Add config header\n"
"-h               This help\n"
	, prgname);
}

#define err(...) do { int save_errno = errno; \
                      fprintf(stderr, __VA_ARGS__); \
                      errno = save_errno; \
                    } while (0);
#define pdie(func, ...) do { perror(func); exit(1); } while (0);

int main(int argc, char *argv[])
{
	int	i;
	char	*ifname, *ofname = NULL, ch;
	FILE	*ifile, *ofile;
	unsigned long	loadaddr = ~0, len;
	struct stat	sinfo;
	int ch_add = 0;
	int opt;

	while ((opt = getopt(argc, argv, "o:hl:c")) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'o':
			ofname = optarg;
			break;
		case 'l':
			loadaddr = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			ch_add = 1;
			break;
		default:
			exit(1);
		}
	}

	if (loadaddr == ~0) {
		fprintf(stderr, "no loadaddr given\n");
		exit(1);
	}

	if (optind == argc || !ofname) {
		usage(argv[0]);
		exit(1);
	}

	ifname = argv[optind];

	/* Open the input file. */
	ifile = fopen(ifname, "rb");
	if (ifile == NULL) {
		err("Cannot open %s\n", ifname);
		pdie("fopen");
	}

	/* Get file length. */
	stat(ifname, &sinfo);
	len = sinfo.st_size;

	/* Open the output file and write it. */
	ofile = fopen(ofname, "wb");
	if (ofile == NULL) {
		fclose(ifile);
		err("Cannot open %s\n", ofname);
		pdie("fopen");
	}

	if (ch_add)
		if (fwrite(&config_header, 1, 512, ofile) <= 0)
			pdie("fwrite");

	if (fwrite(&len, 1, 4, ofile) <= 0)
		pdie("fwrite");
	if (fwrite(&loadaddr, 1, 4, ofile) <= 0)
		pdie("fwrite");
	for (i = 0; i < len; i++) {
		if (fread(&ch, 1, 1, ifile) <= 0)
		    pdie("fread");
		if (fwrite(&ch, 1, 1, ofile) <= 0)
		    pdie("fwrite");
	}

	if (fclose(ifile))
		perror("warning: fclose");
	if (fclose(ofile))
		perror("warning: fclose");

	return 0;
}
