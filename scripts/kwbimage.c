/*
 * Image manipulator for Marvell SoCs
 *  supports Kirkwood, Dove, Armada 370, and Armada XP
 *
 * (C) Copyright 2013 Thomas Petazzoni
 * <thomas.petazzoni@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This tool allows to extract and create bootable images for Marvell
 * Kirkwood, Dove, Armada 370, and Armada XP SoCs. It supports two
 * versions of the bootable image format: version 0 (used on Marvell
 * Kirkwood and Dove) and version 1 (used on Marvell Armada 370/XP).
 *
 * To extract an image, run:
 *  ./scripts/kwbimage -x -i <image-file> -o <some-directory>
 *
 * In <some-directory>, kwbimage will output 'kwbimage.cfg', the
 * configuration file that describes the image, 'payload', which is
 * the bootloader code itself, and it may output a 'binary.0' file
 * that corresponds to a binary blob (only possible in version 1
 * images).
 *
 * To create an image, run:
 *  ./scripts/kwbimage -c -i <image-cfg-file> -o <image-file>
 *
 * The given configuration file is in the format of the 'kwbimage.cfg'
 * file, and should reference the payload file (generally the
 * bootloader code) and optionally a binary blob.
 *
 * Not implemented: support for the register headers and secure
 * headers in v1 images
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#define ALIGN_SUP(x, a) (((x) + (a - 1)) & ~(a - 1))

/* Structure of the main header, version 0 (Kirkwood, Dove) */
struct main_hdr_v0 {
	uint8_t  blockid;		/*0     */
	uint8_t  nandeccmode;		/*1     */
	uint16_t nandpagesize;		/*2-3   */
	uint32_t blocksize;		/*4-7   */
	uint32_t rsvd1;			/*8-11  */
	uint32_t srcaddr;		/*12-15 */
	uint32_t destaddr;		/*16-19 */
	uint32_t execaddr;		/*20-23 */
	uint8_t  satapiomode;		/*24    */
	uint8_t  rsvd3;			/*25    */
	uint16_t ddrinitdelay;		/*26-27 */
	uint16_t rsvd2;			/*28-29 */
	uint8_t  ext;			/*30    */
	uint8_t  checksum;		/*31    */
};

struct ext_hdr_v0_reg {
	uint32_t raddr;
	uint32_t rdata;
};

#define EXT_HDR_V0_REG_COUNT ((0x1dc - 0x20)/sizeof(struct ext_hdr_v0_reg))

struct ext_hdr_v0 {
	uint32_t              offset;
	uint8_t               reserved[0x20 - sizeof(uint32_t)];
	struct ext_hdr_v0_reg rcfg[EXT_HDR_V0_REG_COUNT];
	uint8_t               reserved2[7];
	uint8_t               checksum;
};

/* Structure of the main header, version 1 (Armada 370, Armada XP) */
struct main_hdr_v1 {
	uint8_t  blockid;               /* 0 */
	uint8_t  reserved1;             /* 1 */
	uint16_t reserved2;             /* 2-3 */
	uint32_t blocksize;             /* 4-7 */
	uint8_t  version;               /* 8 */
	uint8_t  headersz_msb;          /* 9 */
	uint16_t headersz_lsb;          /* A-B */
	uint32_t srcaddr;               /* C-F */
	uint32_t destaddr;              /* 10-13 */
	uint32_t execaddr;              /* 14-17 */
	uint8_t  reserved3;             /* 18 */
	uint8_t  nandblocksize;         /* 19 */
	uint8_t  nandbadblklocation;    /* 1A */
	uint8_t  reserved4;             /* 1B */
	uint16_t reserved5;             /* 1C-1D */
	uint8_t  ext;                   /* 1E */
	uint8_t  checksum;              /* 1F */
};

/*
 * Header for the optional headers, version 1 (Armada 370, Armada XP)
 */
struct opt_hdr_v1 {
	uint8_t  headertype;
	uint8_t  headersz_msb;
	uint16_t headersz_lsb;
	char     data[0];
};

/*
 * Various values for the opt_hdr_v1->headertype field, describing the
 * different types of optional headers. The "secure" header contains
 * informations related to secure boot (encryption keys, etc.). The
 * "binary" header contains ARM binary code to be executed prior to
 * executing the main payload (usually the bootloader). This is
 * typically used to execute DDR3 training code. The "register" header
 * allows to describe a set of (address, value) tuples that are
 * generally used to configure the DRAM controller.
 */
#define OPT_HDR_V1_SECURE_TYPE   0x1
#define OPT_HDR_V1_BINARY_TYPE   0x2
#define OPT_HDR_V1_REGISTER_TYPE 0x3

#define KWBHEADER_V1_SIZE(hdr) \
	(((hdr)->headersz_msb << 16) | (hdr)->headersz_lsb)

struct boot_mode {
	unsigned int id;
	const char *name;
};

struct boot_mode boot_modes[] = {
	{ 0x4D, "i2c"  },
	{ 0x5A, "spi"  },
	{ 0x8B, "nand" },
	{ 0x78, "sata" },
	{ 0x9C, "pex"  },
	{ 0x69, "uart" },
	{},
};

struct nand_ecc_mode {
	unsigned int id;
	const char *name;
};

struct nand_ecc_mode nand_ecc_modes[] = {
	{ 0x00, "default" },
	{ 0x01, "hamming" },
	{ 0x02, "rs" },
	{ 0x03, "disabled" },
	{},
};

/* Used to identify an undefined execution or destination address */
#define ADDR_INVALID ((uint32_t)-1)

#define BINARY_MAX_ARGS 8

/* In-memory representation of a line of the configuration file */
struct image_cfg_element {
	enum {
		IMAGE_CFG_VERSION = 0x1,
		IMAGE_CFG_BOOT_FROM,
		IMAGE_CFG_DEST_ADDR,
		IMAGE_CFG_EXEC_ADDR,
		IMAGE_CFG_NAND_BLKSZ,
		IMAGE_CFG_NAND_BADBLK_LOCATION,
		IMAGE_CFG_NAND_ECC_MODE,
		IMAGE_CFG_NAND_PAGESZ,
		IMAGE_CFG_BINARY,
		IMAGE_CFG_PAYLOAD,
		IMAGE_CFG_DATA,
	} type;
	union {
		unsigned int version;
		unsigned int bootfrom;
		struct {
			char *file;
			unsigned int args[BINARY_MAX_ARGS];
			unsigned int nargs;
		} binary;
		const char *payload;
		unsigned int dstaddr;
		unsigned int execaddr;
		unsigned int nandblksz;
		unsigned int nandbadblklocation;
		unsigned int nandeccmode;
		unsigned int nandpagesz;
		struct ext_hdr_v0_reg regdata;
	};
};

#define IMAGE_CFG_ELEMENT_MAX 256

/*
 * Byte 8 of the image header contains the version number. In the v0
 * header, byte 8 was reserved, and always set to 0. In the v1 header,
 * byte 8 has been changed to a proper field, set to 1.
 */
static unsigned int image_version(void *header)
{
	unsigned char *ptr = header;
	return ptr[8];
}

/*
 * Utility functions to manipulate boot mode and ecc modes (convert
 * them back and forth between description strings and the
 * corresponding numerical identifiers).
 */

static const char *image_boot_mode_name(unsigned int id)
{
	int i;
	for (i = 0; boot_modes[i].name; i++)
		if (boot_modes[i].id == id)
			return boot_modes[i].name;
	return NULL;
}

int image_boot_mode_id(const char *boot_mode_name)
{
	int i;
	for (i = 0; boot_modes[i].name; i++)
		if (!strcmp(boot_modes[i].name, boot_mode_name))
			return boot_modes[i].id;

	return -1;
}

static const char *image_nand_ecc_mode_name(unsigned int id)
{
	int i;
	for (i = 0; nand_ecc_modes[i].name; i++)
		if (nand_ecc_modes[i].id == id)
			return nand_ecc_modes[i].name;
	return NULL;
}

int image_nand_ecc_mode_id(const char *nand_ecc_mode_name)
{
	int i;
	for (i = 0; nand_ecc_modes[i].name; i++)
		if (!strcmp(nand_ecc_modes[i].name, nand_ecc_mode_name))
			return nand_ecc_modes[i].id;
	return -1;
}

static struct image_cfg_element *
image_find_option(struct image_cfg_element *image_cfg,
		  int cfgn, unsigned int optiontype)
{
	int i;

	for (i = 0; i < cfgn; i++) {
		if (image_cfg[i].type == optiontype)
			return &image_cfg[i];
	}

	return NULL;
}

static unsigned int
image_count_options(struct image_cfg_element *image_cfg,
		    int cfgn, unsigned int optiontype)
{
	int i;
	unsigned int count = 0;

	for (i = 0; i < cfgn; i++)
		if (image_cfg[i].type == optiontype)
			count++;

	return count;
}

/*
 * Compute a 8-bit checksum of a memory area. This algorithm follows
 * the requirements of the Marvell SoC BootROM specifications.
 */
static uint8_t image_checksum8(void *start, uint32_t len)
{
	uint8_t csum = 0;
	uint8_t *p = start;

	/* check len and return zero checksum if invalid */
	if (!len)
		return 0;

	do {
		csum += *p;
		p++;
	} while (--len);

	return csum;
}

static uint32_t image_checksum32 (void *start, uint32_t len)
{
	uint32_t csum = 0;
	uint32_t *p = start;

	/* check len and return zero checksum if invalid */
	if (!len)
		return 0;

	if (len % sizeof(uint32_t)) {
		fprintf (stderr, "Length %d is not in multiple of %zu\n",
			 len, sizeof(uint32_t));
		return 0;
	}

	do {
		csum += *p;
		p++;
		len -= sizeof(uint32_t);
	} while (len > 0);

	return csum;
}

static void usage(const char *prog)
{
	printf("Usage: %s [-c | -x] -i <input> -o <output>\n", prog);
	printf("   -c: create a new image\n");
	printf("   -x: extract an existing image\n");
	printf("   -i: input file\n");
	printf("       when used with -c, should point to a kwbimage.cfg file\n");
	printf("       when used with -x, should point to the image to be extracted\n");
	printf("   -o: output file/directory\n");
	printf("       when used with -c, should point to the image file to create\n");
	printf("       when used with -x, should point to a directory when the image will be extracted\n");
	printf("   -v: verbose\n");
	printf("   -h: this help text\n");
	printf(" Options specific to image creation:\n");
	printf("   -p: path to payload image. Overrides the PAYLOAD line from kwbimage.cfg\n");
	printf("   -b: path to binary image. Overrides the BINARY line from kwbimage.cfg\n");
	printf("   -m: boot media. Overrides the BOOT_FROM line from kwbimage.cfg\n");
	printf("   -d: load address. Overrides the DEST_ADDR line from kwbimage.cfg\n");
	printf("   -e: exec address. Overrides the EXEC_ADDR line from kwbimage.cfg\n");
}

static int image_extract_payload(void *payload, size_t sz, const char *output)
{
	char *imageoutname;
	FILE *imageout;
	int ret;

	ret = asprintf(&imageoutname, "%s/payload", output);
	if (ret < 0) {
		fprintf(stderr, "Cannot allocate memory\n");
		return -1;
	}

	imageout = fopen(imageoutname, "w+");
	if (!imageout) {
		fprintf(stderr, "Could not open output file %s\n",
			imageoutname);
		free(imageoutname);
		return -1;
	}

	ret = fwrite(payload, sz, 1, imageout);
	if (ret != 1) {
		fprintf(stderr, "Could not write to open file %s\n",
			imageoutname);
		fclose(imageout);
		free(imageoutname);
		return -1;
	}

	fclose(imageout);
	free(imageoutname);
	return 0;
}

static int image_extract_v0(void *fdimap, const char *output, FILE *focfg)
{
	struct main_hdr_v0 *main_hdr = fdimap;
	struct ext_hdr_v0 *ext_hdr;
	const char *boot_mode_name;
	uint32_t *img_checksum;
	size_t payloadsz;
	int cksum, i;

	/*
	 * Verify checksum. When calculating the header, discard the
	 * last byte of the header, which itself contains the
	 * checksum.
	 */
	cksum = image_checksum8(main_hdr, sizeof(struct main_hdr_v0)-1);
	if (cksum != main_hdr->checksum) {
		fprintf(stderr,
			"Invalid main header checksum: 0x%08x vs. 0x%08x\n",
			cksum, main_hdr->checksum);
		return -1;
	}

	boot_mode_name = image_boot_mode_name(main_hdr->blockid);
	if (!boot_mode_name) {
		fprintf(stderr, "Invalid boot ID: 0x%x\n",
			main_hdr->blockid);
		return -1;
	}

	fprintf(focfg, "VERSION 0\n");
	fprintf(focfg, "BOOT_FROM %s\n", boot_mode_name);
	fprintf(focfg, "DESTADDR %08x\n", main_hdr->destaddr);
	fprintf(focfg, "EXECADDR %08x\n", main_hdr->execaddr);

	if (!strcmp(boot_mode_name, "nand")) {
		const char *nand_ecc_mode =
			image_nand_ecc_mode_name(main_hdr->nandeccmode);
		fprintf(focfg, "NAND_ECCMODE %s\n",
			nand_ecc_mode);
		fprintf(focfg, "NAND_PAGESZ %08x\n",
			main_hdr->nandpagesize);
	}

	/* No extension header, we're done */
	if (!main_hdr->ext)
		return 0;

	ext_hdr = fdimap + sizeof(struct main_hdr_v0);

	for (i = 0; i < EXT_HDR_V0_REG_COUNT; i++) {
		if (ext_hdr->rcfg[i].raddr == 0 &&
		    ext_hdr->rcfg[i].rdata == 0)
			break;

		fprintf(focfg, "DATA %08x %08x\n",
			ext_hdr->rcfg[i].raddr,
			ext_hdr->rcfg[i].rdata);
	}

	/* The image is concatenated with a 32 bits checksum */
	payloadsz = main_hdr->blocksize - sizeof(uint32_t);
	img_checksum = (uint32_t *) (fdimap + main_hdr->srcaddr + payloadsz);

	if (*img_checksum != image_checksum32(fdimap + main_hdr->srcaddr,
					      payloadsz)) {
		fprintf(stderr, "The image checksum does not match\n");
		return -1;
	}

	/* Finally, handle the image itself */
	fprintf(focfg, "PAYLOAD %s/payload\n", output);
	return image_extract_payload(fdimap + main_hdr->srcaddr,
				     payloadsz, output);
}

static int image_extract_binary_hdr_v1(const void *binary, const char *output,
				       FILE *focfg, int hdrnum, size_t binsz)
{
	char *binaryoutname;
	FILE *binaryout;
	const unsigned int *args;
	unsigned int nargs;
	int ret, i;

	args = binary;
	nargs = args[0];
	args++;

	ret = asprintf(&binaryoutname, "%s/binary.%d", output,
		       hdrnum);
	if (ret < 0) {
		fprintf(stderr, "Couldn't not allocate memory\n");
		return ret;
	}

	binaryout = fopen(binaryoutname, "w+");
	if (!binaryout) {
		fprintf(stderr, "Couldn't open output file %s\n",
			binaryoutname);
		free(binaryoutname);
		return -1;
	}

	ret = fwrite(binary + (nargs + 1) * sizeof(unsigned int),
		     binsz - (nargs + 2) * sizeof(unsigned int), 1,
		     binaryout);
	if (ret != 1) {
		fprintf(stderr, "Could not write to output file %s\n",
			binaryoutname);
		fclose(binaryout);
		free(binaryoutname);
		return -1;
	}

	fclose(binaryout);

	fprintf(focfg, "BINARY %s", binaryoutname);
	for (i = 0; i < nargs; i++)
		fprintf(focfg, " %08x", args[i]);
	fprintf(focfg, "\n");

	free(binaryoutname);

	return 0;
}

static int image_extract_v1(void *fdimap, const char *output, FILE *focfg)
{
	struct main_hdr_v1 *main_hdr = fdimap;
	struct opt_hdr_v1 *opt_hdr;
	const char *boot_mode_name;
	int headersz = KWBHEADER_V1_SIZE(main_hdr);
	int hasheaders;
	uint8_t cksum;
	int opthdrid;

	/*
	 * Verify the checksum. We have to subtract the checksum
	 * itself, because when the checksum is calculated, the
	 * checksum field is 0.
	 */
	cksum = image_checksum8(main_hdr, headersz);
	cksum -= main_hdr->checksum;

	if (cksum != main_hdr->checksum) {
		fprintf(stderr,
			"Invalid main header checksum: 0x%08x vs. 0x%08x\n",
			cksum, main_hdr->checksum);
		return -1;
	}

	/* First, take care of the main header */
	boot_mode_name = image_boot_mode_name(main_hdr->blockid);
	if (!boot_mode_name) {
		fprintf(stderr, "Invalid boot ID: 0x%x\n",
			main_hdr->blockid);
		return -1;
	}

	fprintf(focfg, "VERSION 1\n");
	fprintf(focfg, "BOOT_FROM %s\n", boot_mode_name);
	fprintf(focfg, "DESTADDR %08x\n", main_hdr->destaddr);
	fprintf(focfg, "EXECADDR %08x\n", main_hdr->execaddr);
	fprintf(focfg, "NAND_BLKSZ %08x\n",
		main_hdr->nandblocksize * 64 * 1024);
	fprintf(focfg, "NAND_BADBLK_LOCATION %02x\n",
		main_hdr->nandbadblklocation);

	hasheaders = main_hdr->ext;
	opt_hdr = fdimap + sizeof(struct main_hdr_v1);
	opthdrid = 0;

	/* Then, go through all the extension headers */
	while (hasheaders) {
		int opthdrsz = KWBHEADER_V1_SIZE(opt_hdr);

		switch (opt_hdr->headertype) {
		case OPT_HDR_V1_BINARY_TYPE:
			image_extract_binary_hdr_v1(opt_hdr->data, output,
						    focfg, opthdrid,
						    opthdrsz -
						    sizeof(struct opt_hdr_v1));
			break;
		case OPT_HDR_V1_SECURE_TYPE:
		case OPT_HDR_V1_REGISTER_TYPE:
			fprintf(stderr,
				"Support for header type 0x%x not implemented\n",
				opt_hdr->headertype);
			exit(1);
			break;
		default:
			fprintf(stderr, "Invalid header type 0x%x\n",
				opt_hdr->headertype);
			exit(1);
		}

		/*
		 * The first byte of the last double word of the
		 * current header indicates whether there is a next
		 * header or not.
		 */
		hasheaders = ((char *)opt_hdr)[opthdrsz - 4];

		/* Move to the next header */
		opt_hdr = ((void *)opt_hdr) + opthdrsz;
		opthdrid++;
	}

	/* Finally, handle the image itself */
	fprintf(focfg, "PAYLOAD %s/payload\n", output);
	return image_extract_payload(fdimap + main_hdr->srcaddr,
				     main_hdr->blocksize - 4,
				     output);
}

static int image_extract(const char *input, const char *output)
{
	int fdi, ret;
	struct stat fdistat, fdostat;
	void *fdimap;
	char *focfgname;
	FILE *focfg;

	fdi = open(input, O_RDONLY);
	if (fdi < 0) {
		fprintf(stderr, "Cannot open input file %s: %m\n",
			input);
		return -1;
	}

	ret = fstat(fdi, &fdistat);
	if (ret < 0) {
		fprintf(stderr, "Cannot stat input file %s: %m\n",
			input);
		close(fdi);
		return -1;
	}

	fdimap = mmap(NULL, fdistat.st_size, PROT_READ, MAP_PRIVATE, fdi, 0);
	if (fdimap == MAP_FAILED) {
		fprintf(stderr, "Cannot map input file %s: %m\n",
			input);
		close(fdi);
		return -1;
	}

	close(fdi);

	ret = stat(output, &fdostat);
	if (ret < 0) {
		fprintf(stderr, "Cannot stat output directory %s: %m\n",
			output);
		munmap(fdimap, fdistat.st_size);
		return -1;
	}

	if (!S_ISDIR(fdostat.st_mode)) {
		fprintf(stderr, "Output %s should be a directory\n",
			output);
		munmap(fdimap, fdistat.st_size);
		return -1;
	}

	ret = asprintf(&focfgname, "%s/kwbimage.cfg", output);
	if (ret < 0) {
		fprintf(stderr, "Failed to allocate memory\n");
		munmap(fdimap, fdistat.st_size);
		return -1;
	}

	focfg = fopen(focfgname, "w+");
	if (!focfg) {
		fprintf(stderr, "Output file %s could not be created\n",
			focfgname);
		free(focfgname);
		munmap(fdimap, fdistat.st_size);
		return -1;
	}

	free(focfgname);

	if (image_version(fdimap) == 0)
		ret = image_extract_v0(fdimap, output, focfg);
	else if (image_version(fdimap) == 1)
		ret = image_extract_v1(fdimap, output, focfg);
	else {
		fprintf(stderr, "Invalid image version %d\n",
			image_version(fdimap));
		ret = -1;
	}

	fclose(focfg);
	munmap(fdimap, fdistat.st_size);
	return ret;
}

static int image_create_payload(void *payload_start, size_t payloadsz,
				const char *payload_filename)
{
	FILE *payload;
	struct stat s;
	uint32_t *payload_checksum =
		(uint32_t *) (payload_start + payloadsz);
	int ret;

	payload = fopen(payload_filename, "r");
	if (!payload) {
		fprintf(stderr, "Cannot open payload file %s\n",
			payload_filename);
		return -1;
	}

	ret = stat(payload_filename, &s);
	if (ret < 0) {
		fprintf(stderr, "Cannot stat payload file %s\n",
			payload_filename);
		fclose(payload);
		return ret;
	}

	ret = fread(payload_start, s.st_size, 1, payload);
	fclose(payload);
	if (ret != 1) {
		fprintf(stderr, "Cannot read payload file %s\n",
			payload_filename);
		return -1;
	}

	*payload_checksum = image_checksum32(payload_start, payloadsz);
	return 0;
}

static void *image_create_v0(struct image_cfg_element *image_cfg,
			     int cfgn, const char *output, size_t *imagesz)
{
	struct image_cfg_element *e, *payloade;
	size_t headersz, payloadsz, totalsz;
	struct main_hdr_v0 *main_hdr;
	struct ext_hdr_v0 *ext_hdr;
	void *image;
	int has_ext = 0;
	int ret;

	/* Calculate the size of the header and the size of the
	 * payload */
	headersz  = sizeof(struct main_hdr_v0);
	payloadsz = 0;

	if (image_count_options(image_cfg, cfgn, IMAGE_CFG_DATA) > 0) {
		has_ext = 1;
		headersz += sizeof(struct ext_hdr_v0);
	}

	if (image_count_options(image_cfg, cfgn, IMAGE_CFG_PAYLOAD) > 1) {
		fprintf(stderr, "More than one payload, not possible\n");
		return NULL;
	}

	payloade = image_find_option(image_cfg, cfgn, IMAGE_CFG_PAYLOAD);
	if (payloade) {
		struct stat s;
		int ret;

		ret = stat(payloade->payload, &s);
		if (ret < 0) {
			fprintf(stderr, "Cannot stat payload file %s\n",
				payloade->payload);
			return NULL;
		}

		/* payload size must be multiple of 32b */
		payloadsz = 4 * ((s.st_size + 3)/4);
	}

	/* Headers, payload and 32-bits checksum */
	totalsz = headersz + payloadsz + sizeof(uint32_t);

	image = malloc(totalsz);
	if (!image) {
		fprintf(stderr, "Cannot allocate memory for image\n");
		return NULL;
	}

	memset(image, 0, totalsz);

	main_hdr = image;

	/* Fill in the main header */
	main_hdr->blocksize = payloadsz + sizeof(uint32_t);
	main_hdr->srcaddr   = headersz;
	main_hdr->ext       = has_ext;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_BOOT_FROM);
	if (e)
		main_hdr->blockid = e->bootfrom;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_DEST_ADDR);
	if (e)
		main_hdr->destaddr = e->dstaddr;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_EXEC_ADDR);
	if (e)
		main_hdr->execaddr = e->execaddr;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_NAND_ECC_MODE);
	if (e)
		main_hdr->nandeccmode = e->nandeccmode;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_NAND_PAGESZ);
	if (e)
		main_hdr->nandpagesize = e->nandpagesz;
	main_hdr->checksum = image_checksum8(image,
					     sizeof(struct main_hdr_v0));

	/* Generate the ext header */
	if (has_ext) {
		int cfgi, datai;

		ext_hdr = image + sizeof(struct main_hdr_v0);
		ext_hdr->offset = 0x40;

		for (cfgi = 0, datai = 0; cfgi < cfgn; cfgi++) {
			e = &image_cfg[cfgi];

			if (e->type != IMAGE_CFG_DATA)
				continue;

			ext_hdr->rcfg[datai].raddr = e->regdata.raddr;
			ext_hdr->rcfg[datai].rdata = e->regdata.rdata;
			datai++;
		}

		ext_hdr->checksum = image_checksum8(ext_hdr,
						    sizeof(struct ext_hdr_v0));
	}

	if (payloade) {
		ret = image_create_payload(image + headersz, payloadsz,
					   payloade->payload);
		if (ret < 0)
			return NULL;
	}

	*imagesz = totalsz;
	return image;
}

static void *image_create_v1(struct image_cfg_element *image_cfg,
			     int cfgn, const char *output, size_t *imagesz)
{
	struct image_cfg_element *e, *payloade, *binarye;
	struct main_hdr_v1 *main_hdr;
	size_t headersz, payloadsz, totalsz;
	void *image, *cur;
	int hasext = 0;
	int ret;

	/* Calculate the size of the header and the size of the
	 * payload */
	headersz = sizeof(struct main_hdr_v1);
	payloadsz = 0;

	if (image_count_options(image_cfg, cfgn, IMAGE_CFG_BINARY) > 1) {
		fprintf(stderr, "More than one binary blob, not supported\n");
		return NULL;
	}

	if (image_count_options(image_cfg, cfgn, IMAGE_CFG_PAYLOAD) > 1) {
		fprintf(stderr, "More than one payload, not possible\n");
		return NULL;
	}

	binarye = image_find_option(image_cfg, cfgn, IMAGE_CFG_BINARY);
	if (binarye) {
		struct stat s;

		ret = stat(binarye->binary.file, &s);
		if (ret < 0) {
			char *cwd = get_current_dir_name();
			fprintf(stderr,
				"Didn't find the file '%s' in '%s' which is mandatory to generate the image\n"
				"This file generally contains the DDR3 training code, and should be extracted from an existing bootable\n"
				"image for your board. See 'kwbimage -x' to extract it from an existing image.\n",
				binarye->binary.file, cwd);
			free(cwd);
			return NULL;
		}

		headersz += ALIGN_SUP(s.st_size, 4) +
			12 + binarye->binary.nargs * sizeof(unsigned int);
		hasext = 1;
	}

	payloade = image_find_option(image_cfg, cfgn, IMAGE_CFG_PAYLOAD);
	if (payloade) {
		struct stat s;

		ret = stat(payloade->payload, &s);
		if (ret < 0) {
			fprintf(stderr, "Cannot stat payload file %s\n",
				payloade->payload);
			return NULL;
		}

		/* payload size must be multiple of 32b */
		payloadsz = ALIGN_SUP(s.st_size, 4);
	}

	/* The payload should be aligned on some reasonable
	 * boundary */
	headersz = ALIGN_SUP(headersz, 4096);

	/* The total size includes the headers, the payload, and the
	 * 32 bits checksum at the end of the payload */
	totalsz = headersz + payloadsz + sizeof(uint32_t);

	image = malloc(totalsz);
	if (!image) {
		fprintf(stderr, "Cannot allocate memory for image\n");
		return NULL;
	}

	memset(image, 0, totalsz);

	cur = main_hdr = image;
	cur += sizeof(struct main_hdr_v1);

	/* Fill the main header */
	main_hdr->blocksize    = payloadsz + sizeof(uint32_t);
	main_hdr->headersz_lsb = headersz & 0xFFFF;
	main_hdr->headersz_msb = (headersz & 0xFFFF0000) >> 16;
	main_hdr->srcaddr      = headersz;
	main_hdr->ext          = hasext;
	main_hdr->version      = 1;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_BOOT_FROM);
	if (e)
		main_hdr->blockid = e->bootfrom;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_DEST_ADDR);
	if (e)
		main_hdr->destaddr = e->dstaddr;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_EXEC_ADDR);
	if (e)
		main_hdr->execaddr = e->execaddr;
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_NAND_BLKSZ);
	if (e)
		main_hdr->nandblocksize = e->nandblksz / (64 * 1024);
	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_NAND_BADBLK_LOCATION);
	if (e)
		main_hdr->nandbadblklocation = e->nandbadblklocation;

	if (binarye) {
		struct opt_hdr_v1 *hdr = cur;
		unsigned int *args;
		size_t binhdrsz;
		struct stat s;
		int argi;
		FILE *bin;

		hdr->headertype = OPT_HDR_V1_BINARY_TYPE;

		bin = fopen(binarye->binary.file, "r");
		if (!bin) {
			fprintf(stderr, "Cannot open binary file %s\n",
				binarye->binary.file);
			return NULL;
		}

		fstat(fileno(bin), &s);

		binhdrsz = sizeof(struct opt_hdr_v1) +
			(binarye->binary.nargs + 2) * sizeof(unsigned int) +
			ALIGN_SUP(s.st_size, 4);
		hdr->headersz_lsb = binhdrsz & 0xFFFF;
		hdr->headersz_msb = (binhdrsz & 0xFFFF0000) >> 16;

		cur += sizeof(struct opt_hdr_v1);

		args = cur;
		*args = binarye->binary.nargs;
		args++;
		for (argi = 0; argi < binarye->binary.nargs; argi++)
			args[argi] = binarye->binary.args[argi];

		cur += (binarye->binary.nargs + 1) * sizeof(unsigned int);

		if (s.st_size)
			ret = fread(cur, s.st_size, 1, bin);
		else
			ret = 1;

		if (ret != 1) {
			fprintf(stderr,
				"Could not read binary image %s\n",
				binarye->binary.file);
			return NULL;
		}

		fclose(bin);

		cur += ALIGN_SUP(s.st_size, 4);

		/*
		 * For now, we don't support more than one binary
		 * header, and no other header types are
		 * supported. So, the binary header is necessarily the
		 * last one
		 */
		*((unsigned char *) cur) = 0;

		cur += sizeof(uint32_t);
	}

	/* Calculate and set the header checksum */
	main_hdr->checksum = image_checksum8(main_hdr, headersz);

	if (payloade) {
		ret = image_create_payload(image + headersz, payloadsz,
					   payloade->payload);
		if (ret < 0)
			return NULL;
	}

	*imagesz = totalsz;
	return image;
}

static int image_create_config_parse_oneline(char *line,
					     struct image_cfg_element *el,
					     char *configpath)
{
	char *keyword, *saveptr;

	keyword = strtok_r(line, " ", &saveptr);
	if (!strcmp(keyword, "VERSION")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_VERSION;
		el->version = atoi(value);
	} else if (!strcmp(keyword, "BOOT_FROM")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_BOOT_FROM;
		el->bootfrom = image_boot_mode_id(value);
		if (el->bootfrom < 0) {
			fprintf(stderr,
				"Invalid boot media '%s'\n", value);
			return -1;
		}
	} else if (!strcmp(keyword, "DESTADDR")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_DEST_ADDR;
		el->dstaddr = strtoul(value, NULL, 16);
	} else if (!strcmp(keyword, "EXECADDR")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_EXEC_ADDR;
		el->execaddr = strtoul(value, NULL, 16);
	} else if (!strcmp(keyword, "NAND_BLKSZ")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_NAND_BLKSZ;
		el->nandblksz = strtoul(value, NULL, 16);
	} else if (!strcmp(keyword, "NAND_BADBLK_LOCATION")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_NAND_BADBLK_LOCATION;
		el->nandbadblklocation =
			strtoul(value, NULL, 16);
	} else if (!strcmp(keyword, "NAND_ECCMODE")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_NAND_ECC_MODE;
		el->nandeccmode = image_nand_ecc_mode_id(value);
		if (el->nandeccmode < 0) {
			fprintf(stderr,
				"Invalid NAND ECC mode '%s'\n", value);
			return -1;
		}
	} else if (!strcmp(keyword, "NAND_PAGESZ")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_NAND_PAGESZ;
		el->nandpagesz = strtoul(value, NULL, 16);
	} else if (!strcmp(keyword, "BINARY")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		int argi = 0;

		el->type = IMAGE_CFG_BINARY;
		if (*value == '/')
			el->binary.file = strdup(value);
		else
			asprintf(&el->binary.file, "%s/%s", configpath, value);
		while (1) {
			value = strtok_r(NULL, " ", &saveptr);
			if (!value)
				break;
			el->binary.args[argi] = strtoul(value, NULL, 16);
			argi++;
			if (argi >= BINARY_MAX_ARGS) {
				fprintf(stderr,
					"Too many argument for binary\n");
				return -1;
			}
		}
		el->binary.nargs = argi;
	} else if (!strcmp(keyword, "DATA")) {
		char *value1 = strtok_r(NULL, " ", &saveptr);
		char *value2 = strtok_r(NULL, " ", &saveptr);

		if (!value1 || !value2) {
			fprintf(stderr, "Invalid number of arguments for DATA\n");
			return -1;
		}

		el->type = IMAGE_CFG_DATA;
		el->regdata.raddr = strtoul(value1, NULL, 16);
		el->regdata.rdata = strtoul(value2, NULL, 16);
	} else if (!strcmp(keyword, "PAYLOAD")) {
		char *value = strtok_r(NULL, " ", &saveptr);
		el->type = IMAGE_CFG_PAYLOAD;
		el->payload = strdup(value);
	} else {
		fprintf(stderr, "Ignoring unknown line '%s'\n", line);
	}

	return 0;
}

/*
 * Parse the configuration file 'fcfg' into the array of configuration
 * elements 'image_cfg', and return the number of configuration
 * elements in 'cfgn'.
 */
static int image_create_config_parse(const char *input,
				     struct image_cfg_element *image_cfg,
				     int *cfgn)
{
	int ret;
	int cfgi = 0;
	FILE *fcfg;
	char *configpath = dirname(strdup(input));

	fcfg = fopen(input, "r");
	if (!fcfg) {
		fprintf(stderr, "Could not open input file %s\n",
			input);
		free(configpath);
		return -1;
	}

	/* Parse the configuration file */
	while (!feof(fcfg)) {
		char *line;
		char buf[256];

		/* Read the current line */
		memset(buf, 0, sizeof(buf));
		line = fgets(buf, sizeof(buf), fcfg);
		if (!line)
			break;

		/* Ignore useless lines */
		if (line[0] == '\n' || line[0] == '#')
			continue;

		/* Strip final newline */
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = 0;

		/* Parse the current line */
		ret = image_create_config_parse_oneline(line,
							&image_cfg[cfgi],
							configpath);
		if (ret)
			goto out;

		cfgi++;

		if (cfgi >= IMAGE_CFG_ELEMENT_MAX) {
			fprintf(stderr, "Too many configuration elements in .cfg file\n");
			ret = -1;
			goto out;
		}
	}

	ret = 0;
	*cfgn = cfgi;
out:
	fclose(fcfg);
	free(configpath);
	return ret;
}

static int image_override_payload(struct image_cfg_element *image_cfg,
				 int *cfgn, const char *payload)
{
	struct image_cfg_element *e;
	int cfgi = *cfgn;

	if (!payload)
		return 0;

	e = image_find_option(image_cfg, *cfgn, IMAGE_CFG_PAYLOAD);
	if (e) {
		e->payload = payload;
		return 0;
	}

	image_cfg[cfgi].type = IMAGE_CFG_PAYLOAD;
	image_cfg[cfgi].payload = payload;
	cfgi++;

	*cfgn = cfgi;
	return 0;
}

static int image_override_binary(struct image_cfg_element *image_cfg,
				 int *cfgn, char *binary)
{
	struct image_cfg_element *e;
	int cfgi = *cfgn;

	if (!binary)
		return 0;

	e = image_find_option(image_cfg, *cfgn, IMAGE_CFG_BINARY);
	if (e) {
		e->binary.file = binary;
		return 0;
	}

	image_cfg[cfgi].type = IMAGE_CFG_BINARY;
	image_cfg[cfgi].binary.file = binary;
	image_cfg[cfgi].binary.nargs = 0;
	cfgi++;

	*cfgn = cfgi;
	return 0;
}

static int image_override_bootmedia(struct image_cfg_element *image_cfg,
				    int *cfgn, const char *bootmedia)
{
	struct image_cfg_element *e;
	int bootfrom;
	int cfgi = *cfgn;

	if (!bootmedia)
		return 0;

	bootfrom = image_boot_mode_id(bootmedia);
	if (!bootfrom) {
		fprintf(stderr,
			"Invalid boot media '%s'\n", bootmedia);
		return -1;
	}

	e = image_find_option(image_cfg, *cfgn, IMAGE_CFG_BOOT_FROM);
	if (e) {
		e->bootfrom = bootfrom;
		return 0;
	}

	image_cfg[cfgi].type = IMAGE_CFG_BOOT_FROM;
	image_cfg[cfgi].bootfrom = bootfrom;
	cfgi++;

	*cfgn = cfgi;
	return 0;
}

static int image_override_dstaddr(struct image_cfg_element *image_cfg,
				   int *cfgn, uint32_t dstaddr)
{
	struct image_cfg_element *e;
	int cfgi = *cfgn;

	if (dstaddr == ADDR_INVALID)
		return 0;

	e = image_find_option(image_cfg, *cfgn, IMAGE_CFG_DEST_ADDR);
	if (e) {
		e->dstaddr = dstaddr;
		return 0;
	}

	image_cfg[cfgi].type = IMAGE_CFG_DEST_ADDR;
	image_cfg[cfgi].dstaddr = dstaddr;
	cfgi++;

	*cfgn = cfgi;
	return 0;
}

static int image_override_execaddr(struct image_cfg_element *image_cfg,
				   int *cfgn, uint32_t execaddr)
{
	struct image_cfg_element *e;
	int cfgi = *cfgn;

	if (execaddr == ADDR_INVALID)
		return 0;

	e = image_find_option(image_cfg, *cfgn, IMAGE_CFG_EXEC_ADDR);
	if (e) {
		e->execaddr = execaddr;
		return 0;
	}

	image_cfg[cfgi].type = IMAGE_CFG_EXEC_ADDR;
	image_cfg[cfgi].execaddr = execaddr;
	cfgi++;

	*cfgn = cfgi;
	return 0;
}

static int image_get_version(struct image_cfg_element *image_cfg,
			     int cfgn)
{
	struct image_cfg_element *e;

	e = image_find_option(image_cfg, cfgn, IMAGE_CFG_VERSION);
	if (!e)
		return -1;

	return e->version;
}

static void image_dump_config(struct image_cfg_element *image_cfg,
			      int cfgn)
{
	int cfgi;

	printf("== configuration dump\n");

	for (cfgi = 0; cfgi < cfgn; cfgi++) {
		struct image_cfg_element *e = &image_cfg[cfgi];
		switch (e->type) {
		case IMAGE_CFG_VERSION:
			printf("VERSION %u\n", e->version);
			break;
		case IMAGE_CFG_BOOT_FROM:
			printf("BOOTFROM %s\n",
			       image_boot_mode_name(e->bootfrom));
			break;
		case IMAGE_CFG_DEST_ADDR:
			printf("DESTADDR 0x%x\n", e->dstaddr);
			break;
		case IMAGE_CFG_EXEC_ADDR:
			printf("EXECADDR 0x%x\n", e->execaddr);
			break;
		case IMAGE_CFG_NAND_BLKSZ:
			printf("NANDBLKSZ 0x%x\n", e->nandblksz);
			break;
		case IMAGE_CFG_NAND_BADBLK_LOCATION:
			printf("NANDBADBLK 0x%x\n", e->nandbadblklocation);
			break;
		case IMAGE_CFG_NAND_ECC_MODE:
			printf("NAND_ECCMODE 0x%x\n", e->nandeccmode);
			break;
		case IMAGE_CFG_NAND_PAGESZ:
			printf("NAND_PAGESZ 0x%x\n", e->nandpagesz);
			break;
		case IMAGE_CFG_BINARY:
			printf("BINARY %s (%d args)\n", e->binary.file,
			       e->binary.nargs);
			break;
		case IMAGE_CFG_PAYLOAD:
			printf("PAYLOAD %s\n", e->payload);
			break;
		case IMAGE_CFG_DATA:
			printf("DATA 0x%x 0x%x\n",
			       e->regdata.raddr,
			       e->regdata.rdata);
			break;
		default:
			printf("Error, unknown type\n");
			break;
		}
	}

	printf("== end configuration dump\n");
}

static int image_create(const char *input, const char *output,
			const char *payload, char *binary,
			const char *bootmedia, uint32_t dstaddr,
			uint32_t execaddr, int verbose)
{
	struct image_cfg_element *image_cfg;
	FILE *outputimg;
	void *image = NULL;
	int version;
	size_t imagesz;
	int cfgn;
	int ret;

	image_cfg = malloc(IMAGE_CFG_ELEMENT_MAX *
			   sizeof(struct image_cfg_element));
	if (!image_cfg) {
		fprintf(stderr, "Cannot allocate memory\n");
		return -1;
	}

	memset(image_cfg, 0,
	       IMAGE_CFG_ELEMENT_MAX * sizeof(struct image_cfg_element));

	ret = image_create_config_parse(input, image_cfg, &cfgn);
	if (ret) {
		free(image_cfg);
		return -1;
	}

	image_override_payload(image_cfg, &cfgn, payload);
	image_override_binary(image_cfg, &cfgn, binary);
	image_override_bootmedia(image_cfg, &cfgn, bootmedia);
	image_override_dstaddr(image_cfg, &cfgn, dstaddr);
	image_override_execaddr(image_cfg, &cfgn, execaddr);

	if (!image_find_option(image_cfg, cfgn, IMAGE_CFG_BOOT_FROM) ||
	    !image_find_option(image_cfg, cfgn, IMAGE_CFG_DEST_ADDR) ||
	    !image_find_option(image_cfg, cfgn, IMAGE_CFG_EXEC_ADDR)) {
		fprintf(stderr,
			"Missing information (either boot media, exec addr or dest addr)\n");
		free(image_cfg);
		return -1;
	}

	if (verbose)
		image_dump_config(image_cfg, cfgn);

	version = image_get_version(image_cfg, cfgn);

	if (version == 0)
		image = image_create_v0(image_cfg, cfgn, output, &imagesz);
	else if (version == 1)
		image = image_create_v1(image_cfg, cfgn, output, &imagesz);
	else if (version == -1) {
		fprintf(stderr, "File %s does not have the VERSION field\n",
			input);
		free(image_cfg);
		return -1;
	}

	if (!image) {
		fprintf(stderr, "Could not create image\n");
		free(image_cfg);
		return -1;
	}

	free(image_cfg);

	outputimg = fopen(output, "w");
	if (!outputimg) {
		fprintf(stderr, "Cannot open output image %s for writing\n",
			output);
		free(image);
		return -1;
	}

	ret = fwrite(image, imagesz, 1, outputimg);
	if (ret != 1) {
		fprintf(stderr, "Cannot write to output image %s\n",
			output);
		fclose(outputimg);
		free(image);
		return -1;
	}

	fclose(outputimg);
	free(image);

	return 0;
}

enum {
	ACTION_CREATE,
	ACTION_EXTRACT,
	ACTION_DUMP,
	ACTION_HELP,
};

int main(int argc, char *argv[])
{
	int action = -1, opt, verbose = 0;
	const char *input = NULL, *output = NULL,
		*payload = NULL, *bootmedia = NULL;
	char *binary = NULL;
	uint32_t execaddr = ADDR_INVALID, dstaddr = ADDR_INVALID;

	while ((opt = getopt(argc, argv, "hxci:o:p:b:m:e:d:v")) != -1) {
		switch (opt) {
		case 'x':
			action = ACTION_EXTRACT;
			break;
		case 'c':
			action = ACTION_CREATE;
			break;
		case 'i':
			input = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		case 'p':
			payload = optarg;
			break;
		case 'b':
			binary = optarg;
			break;
		case 'm':
			bootmedia = optarg;
			break;
		case 'e':
			execaddr = strtol(optarg, NULL, 0);
			break;
		case 'd':
			dstaddr = strtol(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			action = ACTION_HELP;
			break;
		}
	}

	/* We should have consumed all arguments */
	if (optind != argc) {
		usage(argv[0]);
		exit(1);
	}

	if (action != ACTION_HELP && !input) {
		fprintf(stderr, "Missing input file\n");
		usage(argv[0]);
		exit(1);
	}

	if ((action == ACTION_EXTRACT || action == ACTION_CREATE) &&
	    !output) {
		fprintf(stderr, "Missing output file\n");
		usage(argv[0]);
		exit(1);
	}

	if (action == ACTION_EXTRACT &&
	    (bootmedia || payload ||
	     (execaddr != ADDR_INVALID) || (dstaddr != ADDR_INVALID))) {
		fprintf(stderr,
			"-m, -p, -e or -d do not make sense when extracting an image");
		usage(argv[0]);
		exit(1);
	}

	switch (action) {
	case ACTION_EXTRACT:
		return image_extract(input, output);
	case ACTION_CREATE:
		return image_create(input, output, payload, binary,
				    bootmedia, dstaddr, execaddr,
				    verbose);
	case ACTION_HELP:
		usage(argv[0]);
		return 0;
	default:
		fprintf(stderr, "No action specified\n");
		usage(argv[0]);
		exit(1);
	}

	return 0;
}
