// SPDX-License-Identifier: GPL-2.0-only
/*
 * pxa-image - build a bootable PXA3xx NAND image
 *
 * The PXA3xx Boot ROM reads a Non-Trusted Image Module (NTIM) header from the
 * start of NAND, copies the OBM image listed in it into internal SRAM and
 * jumps there. It does not return: the OBM has to set up DRAM and load the
 * next image itself.
 *
 * This tool builds such an image out of the OBM (a barebox PBL built for
 * internal SRAM) and barebox proper:
 *
 *   offset 0                NTIM header
 *   offset <obm-offset>     OBM, loaded to internal SRAM by the Boot ROM
 *   offset <boot-offset>    barebox, loaded to DRAM by the OBM
 *
 * The default offsets keep the first two in the first erase block, which NAND
 * chips guarantee to be good as shipped, so that the Boot ROM never has to
 * read a block that might be bad. Only barebox lives beyond it, and that one
 * is read by the OBM, which is ours.
 *
 * The size of the barebox image is stored in its NTIM entry so that the OBM
 * knows how much to copy; see pxa_nand_load_image() in mach-pxa.
 */

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define NTIM_VERSION		0x00030102
#define NTIM_ID_TIMH		0x54494d48	/* 'TIMH' */
#define NTIM_ID_OBMI		0x4f424d49	/* 'OBMI' */
#define NTIM_ID_BOOT		0x424f4f54	/* 'BOOT' */
#define NTIM_ID_LAST		0xffffffff
#define NTIM_OEM_UNIQUE_ID	0xcafeaffe
#define NTIM_FLASH_INFO_NAND	0x4e414e06

/* Where the Boot ROM puts the header itself. Fixed, see the Boot ROM manual. */
#define NTIM_LOAD_ADDR		0x5c008000

/*
 * The vendor image declares 0xff for the header itself and rounds the OBM up
 * to a NAND page. Stay byte compatible with it: this is the one configuration
 * known to boot.
 */
#define NTIM_HEADER_CRC_SIZE	0xff
#define NAND_PAGE_SIZE		2048

struct ntim_header {
	uint32_t version;
	uint32_t identifier;
	uint32_t trusted;
	uint32_t issue_date;
	uint32_t oem_unique_id;
	uint32_t reserved[5];
	uint32_t flash_info;
	uint32_t num_images;
	uint32_t num_keys;
	uint32_t size_of_reserved;
};

struct ntim_image {
	uint32_t image_id;
	uint32_t next_image_id;
	uint32_t flash_entry_addr;
	uint32_t load_addr;
	uint32_t image_size;
	uint32_t reserved[10];
};

static void put32(void *buf, uint32_t v)
{
	unsigned char *p = buf;

	p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

static void *read_file(const char *name, size_t *size)
{
	struct stat st;
	void *buf;
	FILE *f;

	f = fopen(name, "rb");
	if (!f) {
		fprintf(stderr, "cannot open %s: %s\n", name, strerror(errno));
		return NULL;
	}
	if (fstat(fileno(f), &st) < 0) {
		fprintf(stderr, "cannot stat %s: %s\n", name, strerror(errno));
		fclose(f);
		return NULL;
	}

	buf = malloc(st.st_size);
	if (!buf) {
		fclose(f);
		return NULL;
	}

	if (fread(buf, 1, st.st_size, f) != (size_t)st.st_size) {
		fprintf(stderr, "short read on %s\n", name);
		free(buf);
		fclose(f);
		return NULL;
	}

	fclose(f);
	*size = st.st_size;

	return buf;
}

/*
 * The header and both images are written at their flash offsets, the gaps are
 * filled with 0xff so that the result can be written to erased NAND as is.
 */
static int pad_to(FILE *out, size_t *pos, size_t target, const char *what)
{
	if (*pos > target) {
		fprintf(stderr, "%s does not fit below 0x%zx (ends at 0x%zx)\n",
			what, target, *pos);
		return -1;
	}

	while (*pos < target) {
		if (fputc(0xff, out) == EOF)
			return -1;
		(*pos)++;
	}

	return 0;
}

static void usage(const char *argv0)
{
	fprintf(stderr,
"usage: %s -o OUT -b OBM -f BAREBOX [options]\n"
"  -o FILE     output image\n"
"  -b FILE     OBM image (barebox PBL for internal SRAM)\n"
"  -f FILE     barebox image\n"
"  -O OFFSET   flash offset of the OBM (default 0x800)\n"
"  -L ADDR     load address of the OBM (default 0x5c020000)\n"
"  -F OFFSET   flash offset of barebox (default 0x20000)\n"
"  -A ADDR     load address of barebox (default 0x81000000)\n"
"  -M SIZE     maximum total image size, 0 to disable (default 0x200000)\n",
		argv0);
}

int main(int argc, char *argv[])
{
	const char *outfile = NULL, *obmfile = NULL, *bootfile = NULL;
	unsigned long obm_offset = 0x800, obm_load = 0x5c020000;
	unsigned long boot_offset = 0x20000, boot_load = 0x81000000;
	unsigned long max_size = 0x200000;
	size_t obm_size, boot_size, pos = 0;
	void *obm, *boot;
	struct ntim_header hdr;
	struct ntim_image img[3];
	FILE *out;
	int opt;

	while ((opt = getopt(argc, argv, "o:b:f:O:L:F:A:M:h")) != -1) {
		switch (opt) {
		case 'o': outfile = optarg; break;
		case 'b': obmfile = optarg; break;
		case 'f': bootfile = optarg; break;
		case 'O': obm_offset = strtoul(optarg, NULL, 0); break;
		case 'L': obm_load = strtoul(optarg, NULL, 0); break;
		case 'F': boot_offset = strtoul(optarg, NULL, 0); break;
		case 'A': boot_load = strtoul(optarg, NULL, 0); break;
		case 'M': max_size = strtoul(optarg, NULL, 0); break;
		default:
			usage(argv[0]);
			return opt == 'h' ? 0 : 1;
		}
	}

	if (!outfile || !obmfile || !bootfile) {
		usage(argv[0]);
		return 1;
	}

	obm = read_file(obmfile, &obm_size);
	if (!obm)
		return 1;
	boot = read_file(bootfile, &boot_size);
	if (!boot)
		return 1;

	memset(&hdr, 0, sizeof(hdr));
	put32(&hdr.version, NTIM_VERSION);
	put32(&hdr.identifier, NTIM_ID_TIMH);
	put32(&hdr.trusted, 0);
	put32(&hdr.issue_date, 0);
	put32(&hdr.oem_unique_id, NTIM_OEM_UNIQUE_ID);
	memset(hdr.reserved, 0xff, sizeof(hdr.reserved));
	put32(&hdr.flash_info, NTIM_FLASH_INFO_NAND);
	put32(&hdr.num_images, 3);
	put32(&hdr.num_keys, 0);
	put32(&hdr.size_of_reserved, 0);

	memset(img, 0, sizeof(img));

	/* The header describes itself first. */
	put32(&img[0].image_id, NTIM_ID_TIMH);
	put32(&img[0].next_image_id, NTIM_ID_OBMI);
	put32(&img[0].flash_entry_addr, 0);
	put32(&img[0].load_addr, NTIM_LOAD_ADDR);
	put32(&img[0].image_size, NTIM_HEADER_CRC_SIZE);

	put32(&img[1].image_id, NTIM_ID_OBMI);
	put32(&img[1].next_image_id, NTIM_ID_BOOT);
	put32(&img[1].flash_entry_addr, obm_offset);
	put32(&img[1].load_addr, obm_load);
	put32(&img[1].image_size,
	      (obm_size + NAND_PAGE_SIZE - 1) & ~(NAND_PAGE_SIZE - 1));

	/*
	 * image_size is what the OBM copies out of NAND, so it has to be the
	 * real size of the barebox image rather than the CRC'd part.
	 */
	put32(&img[2].image_id, NTIM_ID_BOOT);
	put32(&img[2].next_image_id, NTIM_ID_LAST);
	put32(&img[2].flash_entry_addr, boot_offset);
	put32(&img[2].load_addr, boot_load);
	put32(&img[2].image_size, boot_size);

	out = fopen(outfile, "wb");
	if (!out) {
		fprintf(stderr, "cannot create %s: %s\n", outfile,
			strerror(errno));
		return 1;
	}

	if (fwrite(&hdr, 1, sizeof(hdr), out) != sizeof(hdr))
		goto write_error;
	pos += sizeof(hdr);
	if (fwrite(img, 1, sizeof(img), out) != sizeof(img))
		goto write_error;
	pos += sizeof(img);

	if (pad_to(out, &pos, obm_offset, "NTIM header"))
		goto error;
	if (fwrite(obm, 1, obm_size, out) != obm_size)
		goto write_error;
	pos += obm_size;

	if (pad_to(out, &pos, boot_offset, "OBM"))
		goto error;
	if (fwrite(boot, 1, boot_size, out) != boot_size)
		goto write_error;
	pos += boot_size;

	if (max_size && pos > max_size) {
		fprintf(stderr,
			"image is 0x%zx bytes, exceeds the maximum of 0x%lx\n",
			pos, max_size);
		goto error;
	}

	fclose(out);

	printf("pxa-image: NTIM + OBM (0x%zx @ 0x%lx) + barebox (0x%zx @ 0x%lx)"
	       " = 0x%zx bytes\n", obm_size, obm_offset, boot_size, boot_offset,
	       pos);

	return 0;

write_error:
	fprintf(stderr, "write error on %s: %s\n", outfile, strerror(errno));
error:
	fclose(out);
	remove(outfile);

	return 1;
}
