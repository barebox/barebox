/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 *
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * derived from u-boot's mkimage utility
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdbool.h>
#include <inttypes.h>
#include <linux/kernel.h>

#include "../include/soc/imx9/flash_header.h"
#include "compiler.h"

typedef enum option_type {
	NO_IMG = 0,
	DCD,
	SCFW,
	SECO,
	M4,
	AP,
	OUTPUT,
	SCD,
	CSF,
	FLAG,
	DEVICE,
	NEW_CONTAINER,
	APPEND,
	DATA,
	PARTITION,
	FILEOFF,
	MSG_BLOCK,
	DUMMY_V2X,
	SENTINEL,
	UPOWER,
	FCB
} option_type_t;

typedef struct {
	option_type_t option;
	char* filename;
	uint64_t src;
	uint64_t dst;
	uint64_t entry;/* image entry address or general purpose num */
	uint64_t ext;
	uint64_t mu;
	uint64_t part;
} image_t;

typedef enum REVISION_TYPE {
	NO_REV = 0,
	A0,
	B0
} rev_type_t;

typedef enum SOC_TYPE {
	NONE = 0,
	QX,
	QM,
	DXL,
	ULP,
	IMX9,
} soc_type_t;

#define CORE_SC         0x1
#define CORE_CM4_0      0x2
#define CORE_CM4_1      0x3
#define CORE_CA53       0x4
#define CORE_CA35       0x4
#define CORE_CA72       0x5
#define CORE_SECO       0x6
#define CORE_V2X_P      0x9
#define CORE_V2X_S      0xA

#define CORE_ULP_CM33		0x1
#define CORE_ULP_CA35		0x2
#define CORE_ULP_UPOWER 	0x4
#define CORE_ULP_SENTINEL 	0x6

#define SC_R_OTP		357U
#define SC_R_DEBUG		354U
#define SC_R_ROM_0		236U
#define SC_R_PWM_0		191U
#define SC_R_SNVS		356U
#define SC_R_DC_0		32U

#define IMG_TYPE_CSF		0x01   /* CSF image type */
#define IMG_TYPE_SCD		0x02   /* SCD image type */
#define IMG_TYPE_EXEC		0x03   /* Executable image type */
#define IMG_TYPE_DATA		0x04   /* Data image type */
#define IMG_TYPE_DCD_DDR	0x05   /* DCD/DDR image type */
#define IMG_TYPE_SECO		0x06   /* SECO image type */
#define IMG_TYPE_SENTINEL	0x06   /* SENTINEL image type */
#define IMG_TYPE_PROV		0x07   /* Provisioning image type */
#define IMG_TYPE_DEK		0x08   /* DEK validation type */
#define IMG_TYPE_FCB_CHK	0x08   /* The FCB copy image */
#define IMG_TYPE_PRIM_V2X	0x0B   /* Primary V2X FW image */
#define IMG_TYPE_SEC_V2X	0x0C   /* Secondary V2X FW image*/
#define IMG_TYPE_V2X_ROM	0x0D   /* V2X ROM Patch image */
#define IMG_TYPE_V2X_DUMMY	0x0E   /* V2X Dummy image */

#define IMG_TYPE_SHIFT   0
#define IMG_TYPE_MASK    0x1f
#define IMG_TYPE(x)      (((x) & IMG_TYPE_MASK) >> IMG_TYPE_SHIFT)

#define BOOT_IMG_FLAGS_CORE_MASK		0xF
#define BOOT_IMG_FLAGS_CORE_SHIFT		0x04
#define BOOT_IMG_FLAGS_CPU_RID_MASK		0x3FF0
#define BOOT_IMG_FLAGS_CPU_RID_SHIFT		4
#define BOOT_IMG_FLAGS_MU_RID_MASK		0xFFC000
#define BOOT_IMG_FLAGS_MU_RID_SHIFT		14
#define BOOT_IMG_FLAGS_PARTITION_ID_MASK	0x1F000000
#define BOOT_IMG_FLAGS_PARTITION_ID_SHIFT	24

#define SC_R_A35_0      508
#define SC_R_A53_0      1
#define SC_R_A72_0      6
#define SC_R_MU_0A      213
#define SC_R_MU_3A      216
#define SC_R_M4_0_PID0  278
#define SC_R_M4_0_MU_1A 297
#define SC_R_M4_1_PID0  298
#define SC_R_M4_1_MU_1A 317
#define PARTITION_ID_M4 0
#define PARTITION_ID_AP 1
#define PARTITION_ID_AP2 3

/* Command tags and parameters */
#define HAB_DATA_WIDTH_BYTE 1 /* 8-bit value */
#define HAB_DATA_WIDTH_HALF 2 /* 16-bit value */
#define HAB_DATA_WIDTH_WORD 4 /* 32-bit value */
#define HAB_CMD_WRT_DAT_MSK 1 /* mask/value flag */
#define HAB_CMD_WRT_DAT_SET 2 /* set/clear flag */
#define HAB_CMD_CHK_DAT_SET 2 /* set/clear flag */
#define HAB_CMD_CHK_DAT_ANY 4 /* any/all flag */
#define HAB_CMD_WRT_DAT_FLAGS_WIDTH   5 /* flags field width */
#define HAB_CMD_WRT_DAT_FLAGS_SHIFT   3 /* flags field offset */
#define HAB_CMD_WRT_DAT_BYTES_WIDTH   3 /* bytes field width */
#define HAB_CMD_WRT_DAT_BYTES_SHIFT   0 /* bytes field offset */

#define IVT_VER                         0x01
#define IVT_VERSION                     0x43
#define DCD_HEADER_TAG                  0xD2
#define DCD_VERSION                     0x43
#define DCD_WRITE_DATA_COMMAND_TAG      0xCC
#define DCD_WRITE_DATA_PARAM            (HAB_DATA_WIDTH_WORD << HAB_CMD_WRT_DAT_BYTES_SHIFT) /* 0x4 */
#define DCD_WRITE_CLR_BIT_PARAM         ((HAB_CMD_WRT_DAT_MSK << HAB_CMD_WRT_DAT_FLAGS_SHIFT) | (HAB_DATA_WIDTH_WORD << HAB_CMD_WRT_DAT_BYTES_SHIFT)) /* 0xC */
#define DCD_WRITE_SET_BIT_PARAM         ((HAB_CMD_WRT_DAT_MSK << HAB_CMD_WRT_DAT_FLAGS_SHIFT) | (HAB_CMD_WRT_DAT_SET << HAB_CMD_WRT_DAT_FLAGS_SHIFT) | (HAB_DATA_WIDTH_WORD << HAB_CMD_WRT_DAT_BYTES_SHIFT)) /* 0x1C */
#define DCD_CHECK_DATA_COMMAND_TAG      0xCF
#define DCD_CHECK_BITS_CLR_PARAM        (HAB_DATA_WIDTH_WORD << HAB_CMD_WRT_DAT_BYTES_SHIFT) /* 0x04 */
#define DCD_CHECK_BITS_SET_PARAM        ((HAB_CMD_CHK_DAT_SET << HAB_CMD_WRT_DAT_FLAGS_SHIFT) | (HAB_DATA_WIDTH_WORD << HAB_CMD_WRT_DAT_BYTES_SHIFT)) /* 0x14 */
#define DCD_CHECK_ANY_BIT_CLR_PARAM     ((HAB_CMD_CHK_DAT_ANY << HAB_CMD_WRT_DAT_FLAGS_SHIFT) | (HAB_DATA_WIDTH_WORD << HAB_CMD_WRT_DAT_BYTES_SHIFT)) /* 0x24 */
#define DCD_CHECK_ANY_BIT_SET_PARAM     ((HAB_CMD_CHK_DAT_ANY << HAB_CMD_WRT_DAT_FLAGS_SHIFT) | (HAB_CMD_CHK_DAT_SET << HAB_CMD_WRT_DAT_FLAGS_SHIFT) | (HAB_DATA_WIDTH_WORD << HAB_CMD_WRT_DAT_BYTES_SHIFT)) /* 0x34 */

#define IVT_OFFSET_NAND         (0x400)
#define IVT_OFFSET_I2C          (0x400)
#define IVT_OFFSET_FLEXSPI      (0x1000)
#define IVT_OFFSET_SD           (0x400)
#define IVT_OFFSET_SATA         (0x400)
#define IVT_OFFSET_EMMC         (0x400)

#define CSF_DATA_SIZE       (0x4000)
#define INITIAL_LOAD_ADDR_SCU_ROM 0x2000e000
#define INITIAL_LOAD_ADDR_AP_ROM 0x00110000
#define INITIAL_LOAD_ADDR_FLEXSPI 0x08000000
#define IMG_AUTO_ALIGN 0x10

#define UNDEFINED 0xFFFFFFFF

#define OCRAM_START						0x00100000
#define OCRAM_END						0x00400000

#define MAX_NUM_SRK_RECORDS		4

#define IVT_HEADER_TAG_B0		0x87
#define IVT_VERSION_B0			0x00

#define IMG_FLAG_HASH_SHA256		0x000
#define IMG_FLAG_HASH_SHA384		0x100
#define IMG_FLAG_HASH_SHA512		0x200
#define IMG_FLAG_HASH_SM3		0x300

#define IMG_FLAG_ENCRYPTED_MASK		0x400
#define IMG_FLAG_ENCRYPTED_SHIFT	0x0A

#define IMG_FLAG_BOOTFLAGS_MASK		0xFFFF0000
#define IMG_FLAG_BOOTFLAGS_SHIFT	0x10

#define IMG_ARRAY_ENTRY_SIZE		128
#define HEADER_IMG_ARRAY_OFFSET		0x10

#define HASH_STR_SHA_256		"sha256"
#define HASH_STR_SHA_384		"sha384"
#define HASH_STR_SHA_512		"sha512"
#define HASH_STR_SM3			"sm3"

#define HASH_TYPE_SHA_256		256
#define HASH_TYPE_SHA_384		384
#define HASH_TYPE_SHA_512		512
#define HASH_TYPE_SM3			003

#define IMAGE_HASH_ALGO_DEFAULT		HASH_TYPE_SHA_384
#define IMAGE_HASH_ALGO_DEFAULT_NAME	HASH_STR_SHA_384
#define IMAGE_PADDING_DEFAULT		0x1000

#define DCD_ENTRY_ADDR_IN_SCFW		0x240

#define CONTAINER_ALIGNMENT		0x400
#define CONTAINER_FLAGS_DEFAULT		0x10
#define CONTAINER_FUSE_DEFAULT		0x0

#define SIGNATURE_BLOCK_HEADER_LENGTH	0x10

#define BOOT_IMG_META_MU_RID_SHIFT	10
#define BOOT_IMG_META_PART_ID_SHIFT	20

#define IMAGE_TYPE_MASK	0xF

#define CORE_ID_SHIFT	0x4
#define CORE_ID_MASK	0xF

#define HASH_TYPE_SHIFT	0x8
#define HASH_TYPE_MASK	0x7

#define IMAGE_ENCRYPTED_SHIFT	0x11
#define IMAGE_ENCRYPTED_MASK	0x1

#define IMAGE_A35_DEFAULT_META(PART, SC_R_MU)						\
	(((PART == 0 ) ? PARTITION_ID_AP : PART) << BOOT_IMG_META_PART_ID_SHIFT |	\
	SC_R_MU << BOOT_IMG_META_MU_RID_SHIFT | SC_R_A35_0)

#define IMAGE_A53_DEFAULT_META(PART, SC_R_MU)						\
	(((PART == 0 ) ? PARTITION_ID_AP : PART) << BOOT_IMG_META_PART_ID_SHIFT |	\
	 SC_R_MU << BOOT_IMG_META_MU_RID_SHIFT |					\
	 SC_R_A53_0)

#define IMAGE_A72_DEFAULT_META(PART, SC_R_MU)						\
	(((PART == 0 ) ? PARTITION_ID_AP2 : PART) << BOOT_IMG_META_PART_ID_SHIFT |	\
	 SC_R_MU << BOOT_IMG_META_MU_RID_SHIFT | SC_R_A72_0)

#define IMAGE_M4_0_DEFAULT_META(PART)							\
	(((PART == 0) ? PARTITION_ID_M4 : PART) << BOOT_IMG_META_PART_ID_SHIFT |	\
	SC_R_M4_0_MU_1A << BOOT_IMG_META_MU_RID_SHIFT | SC_R_M4_0_PID0)

#define IMAGE_M4_1_DEFAULT_META(PART)							\
	(((PART == 0) ? PARTITION_ID_M4 : PART) << BOOT_IMG_META_PART_ID_SHIFT |	\
	SC_R_M4_1_MU_1A << BOOT_IMG_META_MU_RID_SHIFT | SC_R_M4_1_PID0)

#define CONTAINER_IMAGE_ARRAY_START_OFFSET	0x2000

uint32_t scfw_flags = 0;

static uint32_t custom_partition = 0;

static void copy_file_aligned(int ifd, const char *datafile, int offset, int align)
{
	int dfd;
	struct stat sbuf;
	unsigned char *ptr;
	uint8_t zeros[0x4000];
	int size;
	int ret;

	if (align > 0x4000) {
		fprintf (stderr, "Wrong alignment requested %d\n",
			align);
		exit (EXIT_FAILURE);
	}

	memset(zeros, 0, sizeof(zeros));

	if ((dfd = open(datafile, O_RDONLY|O_BINARY)) < 0) {
		fprintf (stderr, "Can't open %s: %s\n",
			datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (fstat(dfd, &sbuf) < 0) {
		fprintf (stderr, "Can't stat %s: %s\n",
			datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (sbuf.st_size == 0)
		goto close;

	ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, dfd, 0);
	if (ptr == MAP_FAILED) {
		fprintf (stderr, "Can't read %s: %s\n",
			datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	size = sbuf.st_size;
	ret = lseek(ifd, offset, SEEK_SET);
	if (ret < 0) {
		fprintf(stderr, "%s: lseek error %s\n",
			__func__, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (write(ifd, ptr, size) != size) {
		fprintf (stderr, "Write error %s\n",
			strerror(errno));
		exit (EXIT_FAILURE);
	}

	align = ALIGN(size, align) - size;

	if (write(ifd, (char *)&zeros, align) != align) {
		fprintf(stderr, "Write error: %s\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	(void) munmap((void *)ptr, sbuf.st_size);
close:
	(void) close (dfd);

}

static void set_imx_hdr_v3(struct imx_header_v3 *imxhdr, uint32_t dcd_len,
		uint32_t flash_offset, uint32_t hdr_base, uint32_t cont_id)
{
	struct flash_header_v3 *fhdr_v3 = &imxhdr->fhdr[cont_id];

	/* Set magic number */
	fhdr_v3->tag = IVT_HEADER_TAG_B0;
	fhdr_v3->version = IVT_VERSION_B0;
}

static void set_image_hash(struct boot_img *img, char *filename, uint32_t hash_type, int size)
{
	FILE *fp = NULL;
	char sha_command[512];
	char digest_type[10];
	char hash[2 * HASH_MAX_LEN + 1];
	int digest_length = 0;
	int i;

	switch (hash_type) {
	case HASH_TYPE_SHA_256:
		img->hab_flags |= IMG_FLAG_HASH_SHA256;
		strcpy(digest_type, "sha256sum" );
		digest_length = 64;
		break;
	case HASH_TYPE_SHA_384:
		img->hab_flags |= IMG_FLAG_HASH_SHA384;
		strcpy(digest_type, "sha384sum" );
		digest_length = 96;
		break;
	case HASH_TYPE_SHA_512:
		img->hab_flags |= IMG_FLAG_HASH_SHA512;
		strcpy(digest_type, "sha512sum" );
		digest_length = 128;
		break;
	case HASH_TYPE_SM3:
		img->hab_flags |= IMG_FLAG_HASH_SM3;
		strcpy(digest_type, "sm3sum" );
		digest_length = 64;
		break;
	default:
		fprintf(stderr, "Wrong hash type selected (%d) !!!\n\n",
				hash_type);
		exit(EXIT_FAILURE);
		break;
	}

	if (!size || !filename)
		sprintf(sha_command, "%s /dev/null", digest_type);
	else
		sprintf(sha_command, "cat \'%s\' /dev/zero | dd status=none bs=1 count=%d | %s",
			filename, size, digest_type);

	memset(img->hash, 0, HASH_MAX_LEN);

	fp = popen(sha_command, "r");
	if (fp == NULL) {
		fprintf(stderr, "Failed to run command hash\n" );
		exit(EXIT_FAILURE);
	}

	if (fgets(hash, digest_length + 1, fp) == NULL) {
		fprintf(stderr, "Failed to hash file: %s\n", filename ? filename : "<none>");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < strlen(hash)/2; i++)
		sscanf(hash + 2*i, "%02hhx", &img->hash[i]);

	pclose(fp);
}

#define append(p, s, l) do {memcpy(p, (uint8_t *)s, l); p += l; } while (0)

static uint8_t *flatten_container_header(struct imx_header_v3 *imx_header,
					uint8_t containers_count,
					uint32_t *size_out, uint32_t file_offset)
{
	uint8_t *flat = NULL;
	uint8_t *ptr = NULL;
	uint16_t size = 0;
	int i;

	/* Compute size of all container headers */
	for (i = 0; i < containers_count; i++) {

		struct flash_header_v3 *container = &imx_header->fhdr[i];

		container->sig_blk_offset = HEADER_IMG_ARRAY_OFFSET +
			container->num_images * IMG_ARRAY_ENTRY_SIZE;

		container->length = HEADER_IMG_ARRAY_OFFSET +
			(IMG_ARRAY_ENTRY_SIZE * container->num_images) + sizeof(struct sig_blk_hdr);

		/* Print info needed by CST to sign the container header */
		printf("CST: CONTAINER %d offset: 0x%x\n", i, file_offset + size);
		printf("CST: CONTAINER %d: Signature Block: offset is at 0x%x\n", i,
						file_offset + size + container->length - SIGNATURE_BLOCK_HEADER_LENGTH);

		printf("\tOffsets = \t0x%x \t0x%x\n", file_offset + size,
						file_offset + size + container->length - SIGNATURE_BLOCK_HEADER_LENGTH);

		size += ALIGN(container->length, container->padding);
	}

	flat = calloc(size, sizeof(uint8_t));
	if (!flat) {
		fprintf(stderr, "Failed to allocate memory (%d)\n", size);
		exit(EXIT_FAILURE);
	}

	ptr = flat;
	*size_out = size;

	for (i = 0; i < containers_count; i++) {
		struct flash_header_v3 *container = &imx_header->fhdr[i];
		uint32_t container_start_offset = ptr - flat;
		int j;

		/* Append container header */
		append(ptr, container, HEADER_IMG_ARRAY_OFFSET);

		/* Adjust images offset to start from container headers start */
		for (j = 0; j < container->num_images; j++)
			container->img[j].offset -= container_start_offset + file_offset;

		/* Append each image array entry */
		for (j = 0; j < container->num_images; j++)
			append(ptr, &container->img[j], sizeof(struct boot_img));

		append(ptr, &container->sig_blk_hdr, sizeof(struct sig_blk_hdr));

		/* Padding for container (if necessary) */
		ptr += ALIGN(container->length, container->padding) - container->length;
	}

	return flat;
}

static uint64_t read_dcd_offset(char *filename)
{
	int dfd;
	struct stat sbuf;
	uint8_t *ptr;
	uint64_t offset = 0;

	dfd = open(filename, O_RDONLY|O_BINARY);
	if (dfd < 0) {
		fprintf(stderr, "Can't open %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fstat(dfd, &sbuf) < 0) {
		fprintf(stderr, "Can't stat %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, dfd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "Can't read %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	offset = *(uint32_t *)(ptr + DCD_ENTRY_ADDR_IN_SCFW);

	(void) munmap((void *)ptr, sbuf.st_size);
	(void) close(dfd);

	return offset;
}

static uint32_t get_hash_algo(char *images_hash)
{
	uint32_t hash_algo;
	const char *hash_name;

	if (!images_hash) {
		hash_algo = IMAGE_HASH_ALGO_DEFAULT;
		hash_name = IMAGE_HASH_ALGO_DEFAULT_NAME;
	} else if (!strcmp(images_hash, HASH_STR_SHA_256)) {
		hash_algo = HASH_TYPE_SHA_256;
		hash_name = HASH_STR_SHA_256;
	} else if (!strcmp(images_hash, HASH_STR_SHA_384)) {
		hash_algo = HASH_TYPE_SHA_384;
		hash_name = HASH_STR_SHA_384;
	} else if (!strcmp(images_hash, HASH_STR_SHA_512)) {
		hash_algo = HASH_TYPE_SHA_512;
		hash_name = HASH_STR_SHA_512;
	} else if (!strcmp(images_hash, HASH_STR_SM3)) {
		hash_algo = HASH_TYPE_SM3;
		hash_name = HASH_STR_SM3;
	} else {
		fprintf(stderr,
			"\nERROR: %s is an invalid hash argument\n"
			"    Expected values: %s, %s, %s, %s\n\n",
			images_hash, HASH_STR_SHA_256, HASH_STR_SHA_384,
			HASH_STR_SHA_512, HASH_STR_SM3);
		exit(EXIT_FAILURE);
	}

	printf("image hash type:\t%s\n", hash_name);
	return hash_algo;
}

static void set_image_array_entry(struct flash_header_v3 *container, soc_type_t soc,
		const image_t *image_stack, uint32_t offset,
		uint32_t size, char *tmp_filename, bool dcd_skip, char *images_hash)
{
	uint64_t entry = image_stack->entry;
	uint64_t dst = image_stack->dst;
	uint64_t core = image_stack->ext;
	uint32_t meta;
	char *tmp_name = "";
	option_type_t type = image_stack->option;
	struct boot_img *img = &container->img[container->num_images];

	if (container->num_images >= MAX_NUM_IMGS) {
		fprintf(stderr, "Error: Container allows 8 images at most\n");
		exit(EXIT_FAILURE);
	}

	img->offset = offset;  /* Is re-adjusted later */
	img->size = size;

	set_image_hash(img, tmp_filename, get_hash_algo(images_hash), size);

	switch (type) {
	case SECO:
		if (container->num_images > 0) {
			fprintf(stderr, "Error: SECO container only allows 1 image\n");
			exit(EXIT_FAILURE);
		}

		img->hab_flags |= IMG_TYPE_SECO;
		img->hab_flags |= CORE_SECO << BOOT_IMG_FLAGS_CORE_SHIFT;
		tmp_name = "SECO";
		img->dst = 0x20C00000;
		img->entry = 0x20000000;

		break;
	case SENTINEL:
		if (container->num_images > 0) {
			fprintf(stderr, "Error: SENTINEL container only allows 1 image\n");
			exit(EXIT_FAILURE);
		}

		img->hab_flags |= IMG_TYPE_SENTINEL;
		img->hab_flags |= CORE_ULP_SENTINEL << BOOT_IMG_FLAGS_CORE_SHIFT;
		tmp_name = "SENTINEL";
		img->dst = 0XE7FE8000; /* S400 IRAM base */
		img->entry = 0XE7FE8000;

		break;
	case AP:
		if ((soc == QX || soc == DXL) && core == CORE_CA35)
			meta = IMAGE_A35_DEFAULT_META(image_stack->part, image_stack->mu);
		else if (soc == QM && core == CORE_CA53)
			meta = IMAGE_A53_DEFAULT_META(image_stack->part, image_stack->mu);
		else if (soc == QM && core == CORE_CA72)
			meta = IMAGE_A72_DEFAULT_META(image_stack->part, image_stack->mu);
		else if (((soc == ULP) || (soc == IMX9)) && core == CORE_CA35)
			meta = 0;
		else {
			fprintf(stderr, "Error: invalid AP core id: %" PRIi64 "\n", core);
			exit(EXIT_FAILURE);
		}
		img->hab_flags |= IMG_TYPE_EXEC;
		if ((soc == ULP) || (soc == IMX9))
			img->hab_flags |= CORE_ULP_CA35 << BOOT_IMG_FLAGS_CORE_SHIFT;
		else
			img->hab_flags |= CORE_CA53 << BOOT_IMG_FLAGS_CORE_SHIFT; /* On B0, only core id = 4 is valid */
		tmp_name = "AP";
		img->dst = entry;
		img->entry = entry;
		img->meta = meta;
		custom_partition = 0;
		break;
	case M4:
		if ((soc == ULP) || (soc == IMX9)) {
			core = CORE_ULP_CM33;
			meta = 0;
		} else if (core == 0) {
			core = CORE_CM4_0;
			meta = IMAGE_M4_0_DEFAULT_META(custom_partition);
		} else if (core == 1) {
			core = CORE_CM4_1;
			meta = IMAGE_M4_1_DEFAULT_META(custom_partition);
		} else {
			fprintf(stderr, "Error: invalid m4 core id: %" PRIi64 "\n", core);
			exit(EXIT_FAILURE);
		}

		img->hab_flags |= IMG_TYPE_EXEC;
		img->hab_flags |= core << BOOT_IMG_FLAGS_CORE_SHIFT;
		tmp_name = "M4";
		if ((entry & 0x7) != 0)
			fprintf(stderr, "\n\nWarning: M4 Destination address is not 8 byte aligned\n\n");

		if (dst)
			img->dst = dst;
		else
			img->dst = entry;

		img->entry = entry;
		img->meta = meta;
		custom_partition = 0;
		break;
	case DATA:
		img->hab_flags |= IMG_TYPE_DATA;
		if ((soc == ULP) || (soc == IMX9))
			if (core == CORE_CM4_0)
				img->hab_flags |= CORE_ULP_CM33 << BOOT_IMG_FLAGS_CORE_SHIFT;
			else
				img->hab_flags |= CORE_ULP_CA35 << BOOT_IMG_FLAGS_CORE_SHIFT;
		else
			img->hab_flags |= core << BOOT_IMG_FLAGS_CORE_SHIFT;
		tmp_name = "DATA";
		img->dst = entry;
		break;
	case MSG_BLOCK:
		img->hab_flags |= IMG_TYPE_DATA;
		img->hab_flags |= CORE_CA35 << BOOT_IMG_FLAGS_CORE_SHIFT;
		img->meta = core << BOOT_IMG_META_MU_RID_SHIFT;
		tmp_name = "MSG_BLOCK";
		img->dst = entry;
		break;
	case SCFW:
		img->hab_flags |= scfw_flags & 0xFFFF0000;
		img->hab_flags |= IMG_TYPE_EXEC;
		img->hab_flags |= CORE_SC << BOOT_IMG_FLAGS_CORE_SHIFT;
		tmp_name = "SCFW";
		img->dst = 0x1FFE0000;
		img->entry = 0x1FFE0000;

		/* Lets add the DCD now */
		if (!dcd_skip) {
			container->num_images++;
			img = &container->img[container->num_images];
			img->hab_flags |= IMG_TYPE_DCD_DDR;
			img->hab_flags |= CORE_SC << BOOT_IMG_FLAGS_CORE_SHIFT;
			set_image_hash(img, "/dev/null", IMAGE_HASH_ALGO_DEFAULT, 0);
			img->offset = offset + img->size;
			img->entry = read_dcd_offset(tmp_filename);
			img->dst = img->entry - 1;
		}
		break;
	case UPOWER:
		if (soc == ULP) {
			img->hab_flags |= IMG_TYPE_EXEC;
			img->hab_flags |= CORE_ULP_UPOWER << BOOT_IMG_FLAGS_CORE_SHIFT;
			tmp_name = "UPOWER";
			img->dst = 0x28300200; /* UPOWER code RAM */
			img->entry = 0x28300200;
		}
		break;
	case DUMMY_V2X:
		img->hab_flags |= IMG_TYPE_V2X_DUMMY;
		img->hab_flags |= CORE_SC << BOOT_IMG_FLAGS_CORE_SHIFT;
		tmp_name = "V2X Dummy";
		img->dst = entry;
		img->entry = entry;
		img->size = 0; /* dummy image has no size */
		break;
	case FCB:
		img->hab_flags |= IMG_TYPE_FCB_CHK;
		img->dst = entry;
		tmp_name = "FCB";
		break;
	default:
		fprintf(stderr, "unrecognized image type (%d)\n", type);
		exit(EXIT_FAILURE);
	}

	printf("%s file_offset = 0x%x size = 0x%x\n", tmp_name, offset, size);

	container->num_images++;
}

static void set_container(struct flash_header_v3 *container,  uint16_t sw_version,
			uint32_t alignment, uint32_t flags, uint16_t fuse_version)
{
	container->sig_blk_hdr.tag = 0x90;
	container->sig_blk_hdr.length = sizeof(struct sig_blk_hdr);
	container->sw_version = sw_version;
	container->padding = alignment;
	container->fuse_version = fuse_version;
	container->flags = flags;
	printf("flags:\t\t\t0x%x\n", container->flags);
}

static int get_container_image_start_pos(image_t *image_stack, uint32_t align, soc_type_t soc,
					 uint32_t *scu_cont_hdr_off)
{
	image_t *img_sp = image_stack;
	/* 8K total container header */
	int file_off = CONTAINER_IMAGE_ARRAY_START_OFFSET,  ofd = -1;
	struct flash_header_v3 header;

	while (img_sp->option != NO_IMG) {
		if (img_sp->option != APPEND) {
			img_sp++;
			continue;
		}

		ofd = open(img_sp->filename, O_RDONLY);
		if (ofd < 0) {
			printf("Failure open first container file %s\n", img_sp->filename);
			break;
		}

		if (soc == DXL) {
			/* Skip SECO container, jump to V2X container */
			if (lseek(ofd, CONTAINER_ALIGNMENT, SEEK_SET) < 0) {
				printf("Failure Skip SECO header \n");
				exit(EXIT_FAILURE);
			}
		}

		if (read(ofd, &header, sizeof(header)) != sizeof(header)) {
			printf("Failure Read header \n");
			exit(EXIT_FAILURE);
		}

		close(ofd);

		if (header.tag != IVT_HEADER_TAG_B0) {
			printf("header tag mismatch %x\n", header.tag);
		} else if (header.num_images == 0) {
			printf("image num is 0 \n");
		} else {
			file_off = header.img[header.num_images - 1].offset + header.img[header.num_images - 1].size;
			if (soc == DXL) {
				file_off += CONTAINER_ALIGNMENT;
				*scu_cont_hdr_off = CONTAINER_ALIGNMENT + ALIGN(header.length, CONTAINER_ALIGNMENT);
			} else {
				*scu_cont_hdr_off = ALIGN(header.length, CONTAINER_ALIGNMENT);
			}
			file_off = ALIGN(file_off, align);
		}

		img_sp++;
	}

	return file_off;
}

static void check_file(struct stat *sbuf, char *filename)
{
	int tmp_fd  = open(filename, O_RDONLY | O_BINARY);

	if (tmp_fd < 0) {
		fprintf(stderr, "%s: Can't open: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fstat(tmp_fd, sbuf) < 0) {
		fprintf(stderr, "%s: Can't stat: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	close(tmp_fd);
}

static void copy_file(int ifd, const char *datafile, int pad, int offset)
{
	int dfd;
	struct stat sbuf;
	unsigned char *ptr;
	int tail;
	uint8_t zeros[4096];
	int size, ret;

	memset(zeros, 0, sizeof(zeros));

	if ((dfd = open(datafile, O_RDONLY|O_BINARY)) < 0) {
		fprintf(stderr, "Can't open %s: %s\n", datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (fstat(dfd, &sbuf) < 0) {
		fprintf(stderr, "Can't stat %s: %s\n", datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (sbuf.st_size == 0)
		goto close;

	ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, dfd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "Can't read %s: %s\n", datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	size = sbuf.st_size;
	ret = lseek(ifd, offset, SEEK_SET);
	if (ret < 0) {
		fprintf(stderr, "%s: lseek error %s\n", __func__, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (write(ifd, ptr, size) != size) {
		fprintf(stderr, "Write error %s\n", strerror(errno));
		exit (EXIT_FAILURE);
	}

	tail = size % 4;
	pad = pad - size;
	if ((pad == 1) && (tail != 0)) {

		if (write(ifd, zeros, 4 - tail) != 4 - tail) {
			fprintf(stderr, "Write error on %s\n", strerror(errno));
			exit (EXIT_FAILURE);
		}
	} else if (pad > 1) {
		while (pad > 0) {
			int todo = sizeof(zeros);

			if (todo > pad)
				todo = pad;
			if (write(ifd, (char *)&zeros, todo) != todo) {
				fprintf(stderr, "Write error: %s\n",
					strerror(errno));
				exit(EXIT_FAILURE);
			}
			pad -= todo;
		}
	}

	(void) munmap((void *)ptr, sbuf.st_size);
close:
	(void) close (dfd);
}

static int pblsize;

static int build_container_qx_qm_b0(soc_type_t soc, uint32_t sector_size, uint32_t ivt_offset,
				    char *out_file, bool emmc_fastboot, image_t *image_stack,
				    bool dcd_skip, uint8_t fuse_version, uint16_t sw_version,
				    char *images_hash)
{
	int file_off, ofd = -1;
	unsigned int dcd_len = 0;
	struct imx_header_v3 imx_header = {};
	image_t *img_sp = image_stack;
	struct stat sbuf;
	uint32_t size = 0;
	uint32_t file_padding = 0;
	int ret;
	const char *platform;

	int container = -1;
	int cont_img_count = 0; /* indexes to arrange the container */

	if (image_stack == NULL) {
		fprintf(stderr, "Empty image stack ");
		exit(EXIT_FAILURE);
	}

	switch (soc) {
	case QX: platform = "i.MX8QXP B0"; break;
	case QM: platform = "i.MX8QM B0"; break;
	case DXL: platform = "i.MX8DXL A0"; break;
	case ULP: platform = "i.MX8ULP A0"; break;
	case IMX9: platform = "i.MX9"; break;
	default:
		exit(EXIT_FAILURE);
	}

	printf("Platform:\t\t%s\n", platform);

	set_imx_hdr_v3(&imx_header, dcd_len, ivt_offset, INITIAL_LOAD_ADDR_SCU_ROM, 0);
	set_imx_hdr_v3(&imx_header, 0, ivt_offset, INITIAL_LOAD_ADDR_AP_ROM, 1);

	printf("ivt_offset:\t\t0x%x\n", ivt_offset);

	file_off = get_container_image_start_pos(image_stack, sector_size, soc, &file_padding);
	printf("container image offset (aligned): 0x%x\n", file_off);

	printf("csf_off:\t\t0x%x\n", ivt_offset + file_off);

	/* step through image stack and generate the header */
	img_sp = image_stack;

	while (img_sp->option != NO_IMG) { /* stop once we reach null terminator */
		int isize;
		switch (img_sp->option) {
		case FCB:
		case AP:
		case M4:
		case SCFW:
		case DATA:
		case UPOWER:
		case MSG_BLOCK:
		case SENTINEL:
			if (container < 0) {
				fprintf(stderr, "No container found\n");
				exit(EXIT_FAILURE);
			}
			check_file(&sbuf, img_sp->filename);
			isize = ALIGN(sbuf.st_size, sector_size);
			if (pblsize && isize > ALIGN(pblsize, 1024))
				isize = ALIGN(pblsize, 1024);
			set_image_array_entry(&imx_header.fhdr[container],
						soc,
						img_sp,
						file_off,
						isize,
						img_sp->filename,
						dcd_skip,
						images_hash);
			img_sp->src = file_off;

			file_off += ALIGN(sbuf.st_size, sector_size);
			cont_img_count++;
			break;

		case DUMMY_V2X:
			if (container < 0) {
				fprintf(stderr, "No container found\n");
				exit(EXIT_FAILURE);
			}
			set_image_array_entry(&imx_header.fhdr[container],
						soc,
						img_sp,
						file_off,
						0,
						NULL,
						dcd_skip,
						images_hash);
			img_sp->src = file_off;

			cont_img_count++;
			break;

		case SECO:
			if (container < 0) {
				fprintf(stderr, "No container found\n");
				exit(EXIT_FAILURE);
			}
			check_file(&sbuf, img_sp->filename);
			set_image_array_entry(&imx_header.fhdr[container],
						soc,
						img_sp,
						file_off,
						sbuf.st_size,
						img_sp->filename,
						dcd_skip,
						"sha384");
			img_sp->src = file_off;

			file_off += sbuf.st_size;
			cont_img_count++;
			break;

		case NEW_CONTAINER:
			container++;
			set_container(&imx_header.fhdr[container], sw_version,
					CONTAINER_ALIGNMENT,
					CONTAINER_FLAGS_DEFAULT,
					fuse_version);
			cont_img_count = 0; /* reset img count when moving to new container */
			scfw_flags = 0;
			break;

		case APPEND:
			/* nothing to do here, the container is appended in the output */
			break;
		case FLAG:
			/* override the flags for scfw in current container */
			scfw_flags = img_sp->entry & 0xFFFF0000;/* mask off bottom 16 bits */
			break;
		case FILEOFF:
			if (file_off > img_sp->dst) {
				fprintf(stderr, "FILEOFF address less than current file offset!!!\n");
				exit(EXIT_FAILURE);
			}
			if (img_sp->dst != ALIGN(img_sp->dst, sector_size)) {
				fprintf(stderr, "FILEOFF address is not aligned to sector size!!!\n");
				exit(EXIT_FAILURE);
			}
			file_off = img_sp->dst;
			break;
		case PARTITION: /* keep custom partition until next executable image */
			custom_partition = img_sp->entry; /* use a global var for default behaviour */
			break;
		default:
			fprintf(stderr, "unrecognized option in input stack (%d)\n", img_sp->option);
			exit(EXIT_FAILURE);
		}
		img_sp++;/* advance index */
	}

	/* Open output file */
	ofd = open(out_file, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0666);
	if (ofd < 0) {
		fprintf(stderr, "%s: Can't open: %s\n",
				out_file, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Append container (if specified) */
	img_sp = image_stack;
	while (img_sp->option != NO_IMG) {
		if (img_sp->option == APPEND)
			copy_file(ofd, img_sp->filename, 0, 0);

		img_sp++;
	}

	/* Add padding or skip appended container */
	ret = lseek(ofd, file_padding, SEEK_SET);
	if (ret < 0) {
		fprintf(stderr, "%s: lseek error %s\n", __func__, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Note: Image offset are not contained in the image */
	uint8_t *tmp = flatten_container_header(&imx_header, container + 1, &size, file_padding);
	/* Write image header */
	if (write(ofd, tmp, size) != size) {
		fprintf(stderr, "error writing image hdr\n");
		exit(1);
	}

	/* Clean-up memory used by the headers */
	free(tmp);

	if (emmc_fastboot)
		ivt_offset = 0; /*set ivt offset to 0 if emmc */

	/* step through the image stack again this time copying images to final bin */
	img_sp = image_stack;
	while (img_sp->option != NO_IMG) { /* stop once we reach null terminator */
		if (img_sp->option == M4 ||
		    img_sp->option == AP ||
		    img_sp->option == DATA ||
		    img_sp->option == SCD ||
		    img_sp->option == SCFW ||
		    img_sp->option == SECO ||
		    img_sp->option == MSG_BLOCK ||
		    img_sp->option == UPOWER ||
		    img_sp->option == SENTINEL ||
		    img_sp->option == FCB) {
			copy_file_aligned(ofd, img_sp->filename, img_sp->src, sector_size);
		}
		img_sp++;
	}

	/* Close output file */
	close(ofd);
	return 0;
}

static struct img_flags parse_image_flags(uint32_t flags, char *flag_list, soc_type_t soc)
{
	struct img_flags img_flags;

	strcpy(flag_list, "(");

	/* first extract the image type */
	strcat(flag_list, "IMG TYPE: ");
	img_flags.type = flags & IMAGE_TYPE_MASK;

	switch (img_flags.type) {

	case 0x3:
		strcat(flag_list, "Executable");
		break;
	case 0x4:
		strcat(flag_list, "Data");
		break;
	case 0x5:
		strcat(flag_list, "DDR Init");
		break;
	case 0x6:
		if ((soc == ULP) || (soc == IMX9))
			strcat(flag_list, "SENTINEL");
		else
			strcat(flag_list, "SECO");
		break;
	case 0x7:
		strcat(flag_list, "Provisioning");
		break;
	case 0x8:
		if (soc == IMX9)
		    strcat(flag_list, "FCB Check");
		else
		    strcat(flag_list, "DEK validation");
		break;
	case 0xB:
		strcat(flag_list, "Primary V2X FW image");
		break;
	case 0xC:
		strcat(flag_list, "Secondary V2X FW image");
		break;
	case 0xD:
		strcat(flag_list, "V2X ROM Patch image");
		break;
	case 0xE:
		strcat(flag_list, "V2X Dummy image");
		break;
	default:
		strcat(flag_list, "Invalid img type");
		break;
	}
	strcat(flag_list, " | ");

	/* next get the core id */
	strcat(flag_list, "CORE ID: ");
	img_flags.core_id = (flags >> CORE_ID_SHIFT) & CORE_ID_MASK;

	if ((soc == ULP) || (soc == IMX9)) {
		switch (img_flags.core_id) {
		case CORE_ULP_CM33:
			strcat(flag_list, "CORE_CM33");
			break;
		case CORE_ULP_SENTINEL:
			strcat(flag_list, "CORE_SENTINEL");
			break;
		case CORE_ULP_UPOWER:
			strcat(flag_list, "CORE_UPOWER");
			break;
		case CORE_ULP_CA35:
			if (soc == IMX9)
				strcat(flag_list, "CORE_CA55");
			else
				strcat(flag_list, "CORE_CA35");
			break;
		default:
			strcat(flag_list, "Invalid core id");
			break;
		}
	} else {
		switch (img_flags.core_id) {
		case CORE_SC:
			strcat(flag_list, "CORE_SC");
			break;
		case CORE_CM4_0:
			strcat(flag_list, "CORE_CM4_0");
			break;
		case CORE_CM4_1:
			strcat(flag_list, "CORE_CM4_1");
			break;
		case CORE_CA53:
			strcat(flag_list, "CORE_CA53");
			break;
		case CORE_CA72:
			strcat(flag_list, "CORE_CA72");
			break;
		case CORE_SECO:
			strcat(flag_list, "CORE_SECO");
			break;
		case CORE_V2X_P:
			strcat(flag_list, "CORE_V2X_P");
			break;
		case CORE_V2X_S:
			strcat(flag_list, "CORE_V2X_S");
			break;
		default:
			strcat(flag_list, "Invalid core id");
			break;
		}
	}
	strcat(flag_list, " | ");

	/* next get the hash type */
	strcat(flag_list, "HASH TYPE: ");
	img_flags.hash_type = (flags >> HASH_TYPE_SHIFT) & HASH_TYPE_MASK;

	switch (img_flags.hash_type) {
	case 0x0:
		strcat(flag_list, "SHA256");
		break;
	case 0x1:
		strcat(flag_list, "SHA384");
		break;
	case 0x2:
		strcat(flag_list, "SHA512");
		break;
	case 0x3:
		strcat(flag_list, "SM3");
		break;
	default:
		break;
	}
	strcat(flag_list, " | ");

	/* lastly, read the encrypted bit */
	strcat(flag_list, "ENCRYPTED: ");
	img_flags.encrypted = (flags >> IMAGE_ENCRYPTED_SHIFT) & IMAGE_ENCRYPTED_MASK;

	if (img_flags.encrypted)
		strcat(flag_list, "YES");
	else
		strcat(flag_list, "NO");

	/* terminate flag string */
	strcat(flag_list, ")");

	return img_flags;
}

static void print_image_array_fields(struct flash_header_v3 *container_hdrs, soc_type_t soc, bool app_cntr)
{
	struct boot_img img; /* image array entry */
	struct img_flags img_flags; /* image hab flags */
	int hash_length = 0;
	char img_name[32]; /* scfw, bootloader, etc. */
	char hash_name[8]; /* sha256, sha384, or sha512 */
	char flag_string[128]; /* text representation of image hab flags */
	int i;

	for (i = 0; i < container_hdrs->num_images; i++) {
		/* get the next image array entry */
		img = container_hdrs->img[i];

		/* get the image flags */
		img_flags = parse_image_flags(img.hab_flags, flag_string, soc);

		/* determine the type of image */
		switch (img_flags.type) {
		case 0x3:
			if ((soc == ULP) || (soc == IMX9)) {
				if (img_flags.core_id == CORE_ULP_UPOWER)
					strcpy(img_name, "uPower FW");
				else if ((img_flags.core_id == CORE_ULP_CA35))
					if (app_cntr)
						strcpy(img_name, "A core Image");
					else
						strcpy(img_name, "Bootloader");
				else if ((img_flags.core_id == CORE_ULP_CM33))
					strcpy(img_name, "M33");

			} else {
				if (img_flags.core_id == CORE_SC)
					strcpy(img_name, "SCFW");
				else if ((img_flags.core_id == CORE_CA53) || (img_flags.core_id == CORE_CA72))
					if (app_cntr)
						strcpy(img_name, "A core Image");
					else
						strcpy(img_name, "Bootloader");
				else if (img_flags.core_id == CORE_CM4_0)
					strcpy(img_name, "M4_0");
				else if (img_flags.core_id == CORE_CM4_1)
					strcpy(img_name, "M4_1");
			}
			break;
		case 0x4:
			strcpy(img_name, "Data");
			break;
		case 0x5:
			strcpy(img_name, "DDR Init");
			break;
		case 0x6:
			if ((soc == ULP) || (soc == IMX9))
				strcpy(img_name, "SENTINEL FW");
			else
				strcpy(img_name, "SECO FW");
			break;
		case 0x7:
			strcpy(img_name, "Provisioning");
			break;
		case 0x8:
			if (soc == IMX9)
				strcpy(img_name, "FCB Check");
			else
				strcpy(img_name, "DEK Validation");
			break;
		case 0xB:
			strcpy(img_name, "Primary V2X FW image");
			break;
		case 0xC:
			strcpy(img_name, "Secondary V2X FW image");
			break;
		case 0xD:
			strcpy(img_name, "V2X ROM Patch image");
			break;
		case 0xE:
			strcpy(img_name, "V2X Dummy image");
			break;
		default:
			strcpy(img_name, "Unknown image");
			break;
		}

		/* get the image hash type */
		switch (img_flags.hash_type) {
		case 0x0:
			hash_length = 256 / 8;
			strcpy(hash_name, "SHA256");
			break;
		case 0x1:
			hash_length = 384 / 8;
			strcpy(hash_name, "SHA384");
			break;
		case 0x2:
			hash_length = 512 / 8;
			strcpy(hash_name, "SHA512");
			break;
		case 0x3:
			hash_length = 256 / 8;
			strcpy(hash_name, "SM3");
			break;
		default:
			strcpy(hash_name, "Unknown");
			break;
		}

		/* print the image array fields */
		printf("%sIMAGE %d (%s)%s\n", "\x1B[33m", i+1, img_name, "\x1B[37m");
		printf("Offset: %#X\n", img.offset);
		printf("Size: %#X (%d)\n", img.size, img.size);
		printf("Load Addr: %#lX\n", img.dst);
		printf("Entry Addr: %#lX\n", img.entry);
		printf("Flags: %#X %s\n", img.hab_flags, flag_string);

		/* only print metadata and hash if the image isn't DDR init */
		if (img_flags.type != 0x5) {
			int j;

			printf("Metadata: %#X\n", img.meta);

			/* print the image hash */
			printf("Hash: ");
			for (j = 0; j < hash_length; j++)
				printf("%02x", img.hash[j]);

			printf(" (%s)\n", hash_name);

		}
		printf("\n");
	}
}

static void print_container_hdr_fields(struct flash_header_v3 *container_hdrs, int num_cntrs,
				       soc_type_t soc, bool app_cntr)
{
	int i;

	for (i = 0; i < num_cntrs; i++) {
		printf("\n");
		printf("*********************************\n");
		printf("*				*\n");
		if (app_cntr)
			printf("*          APP CONTAINER %-2d     *\n", i + 1);
		else
			printf("*          ROM CONTAINER %-2d     *\n", i + 1);
		printf("*				*\n");
		printf("*********************************\n\n");
		printf("%16s", "Length: ");
		printf("%#X (%d)\n", container_hdrs->length, container_hdrs->length);
		printf("%16s", "Tag: ");
		printf("%#X\n", container_hdrs->tag);
		printf("%16s", "Version: ");
		printf("%#X\n", container_hdrs->version);
		printf("%16s", "Flags: ");
		printf("%#X\n", container_hdrs->flags);
		printf("%16s", "Num images: ");
		printf("%d\n", container_hdrs->num_images);
		printf("%16s", "Fuse version: ");
		printf("%#X\n", container_hdrs->fuse_version);
		printf("%16s", "SW version: ");
		printf("%#X\n", container_hdrs->sw_version);
		printf("%16s", "Sig blk offset: ");
		printf("%#X\n\n", container_hdrs->sig_blk_offset);

		print_image_array_fields(container_hdrs, soc, app_cntr);

		container_hdrs++;
	}
}

static int get_container_size(struct flash_header_v3 *phdr)
{
	uint8_t i = 0;
	uint32_t max_offset = 0, img_end;

	max_offset = phdr->length;

	for (i = 0; i < phdr->num_images; i++) {
		img_end = phdr->img[i].offset + phdr->img[i].size;
		if (img_end > max_offset)
			max_offset = img_end;
	}

	if (phdr->sig_blk_offset != 0) {
		uint16_t len = phdr->sig_blk_hdr.length;

		if (phdr->sig_blk_offset + len > max_offset)
			max_offset = phdr->sig_blk_offset + len;
	}

	return max_offset;
}

static int search_app_container(struct flash_header_v3 *container_hdrs, int num_cntrs, int ifd,
				struct flash_header_v3 *app_container_hdr)
{
	int off[MAX_NUM_OF_CONTAINER];
	int end = 0, last = 0;
	int img_array_entries = 0;
	ssize_t rd_err;
	off_t err;
	int i;

	off[0] = 0;

	for (i = 0; i < num_cntrs; i++) {
		end = get_container_size(&container_hdrs[i]);
		if (end + off[i] > last)
			last = end + off[i];

		if ((i + 1) < num_cntrs)
			off[i + 1] = off[i] + ALIGN(container_hdrs[i].length, CONTAINER_ALIGNMENT);
	}

	/* Check app container tag at each 1KB beginning until 16KB */
	last = ALIGN(last, 0x400);
	for (i = 0; i < 16; i++) {
		last = last + (i * 0x400);
		err = lseek(ifd, last, SEEK_SET);
		if (err < 0)
			break;

		rd_err = read(ifd, (void *)app_container_hdr, 16);
		if (rd_err < 0) {
			fprintf(stderr, "Error reading from input binary\n");
			exit(EXIT_FAILURE);
		}

		/* check that the current container has a valid tag */
		if (app_container_hdr->tag != IVT_HEADER_TAG_B0)
			continue;

		if (app_container_hdr->num_images > MAX_NUM_IMGS) {
			fprintf(stderr, "This container includes %d images, beyond max 8 images\n",
				app_container_hdr->num_images);
			exit(EXIT_FAILURE);
		}

		/* compute the size of the image array */
		img_array_entries = app_container_hdr->num_images * sizeof(struct boot_img);

		/* read in the full image array */
		rd_err = read(ifd, (void *)&app_container_hdr->img, img_array_entries);
		if (rd_err < 0) {
			fprintf(stderr, "Error reading from input binary\n");
			exit(EXIT_FAILURE);
		}

		/* read in signature block header */
		if (app_container_hdr->sig_blk_offset != 0) {
			lseek(ifd, last + app_container_hdr->sig_blk_offset, SEEK_SET);
			rd_err = read(ifd, (void *)&app_container_hdr->sig_blk_hdr, sizeof(struct sig_blk_hdr));
			if (rd_err == -1) {
				fprintf(stderr, "Error reading from input binary\n");
				exit(EXIT_FAILURE);
			}
		}

		return last;
	}

	return 0;
}

static int extract_container_images(struct flash_header_v3 *container_hdr, char *ifname, int num_cntrs,
				    int ifd, soc_type_t soc, int app_cntr_off)
{
	uint32_t img_offset = 0; /* image offset from container header */
	uint32_t img_size = 0; /* image size */
	uint32_t file_off = 0; /* current offset within container binary */
	const uint32_t pad = 0;
	int ofd = 0;
	int ret = 0;
	uint32_t seco_off = 0;
	char dd_cmd[512]; /* dd cmd to extract each image from container binary */
	struct stat buf;
	FILE *f_ptr = NULL; /* file pointer to the dd process */
	char *mem_ptr; /* pointer to input container in memory */
	int i, j;

	printf("Extracting container images...\n");

	/* create output directory if it does not exist */
	if (stat("extracted_imgs", &buf) == -1)
		mkdir("extracted_imgs", S_IRWXU|S_IRWXG|S_IRWXO);

	/* open container binary and map to memory */
	fstat(ifd, &buf);
	mem_ptr = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, ifd, 0);

	for (i = 0; i < num_cntrs; i++) {
		for (j = 0; j < container_hdr->num_images; j++) {

			/* first get the image offset and size from the container header */
			img_offset = container_hdr->img[j].offset;
			img_size = container_hdr->img[j].size;

			if (!img_size) {
				/* check for images with zero size (DDR Init) */
				continue;
			} else if (app_cntr_off > 0) {
				sprintf(dd_cmd, "dd status=none if=%s of=extracted_imgs/app_container%d_img%d.bin ibs=1 skip=%d count=%d conv=notrunc",
						ifname, i+1, j+1, app_cntr_off+img_offset, img_size);
				printf("APP Container %d Image %d -> extracted_imgs/app_container%d_img%d.bin\n", i+1, j+1, i+1, j+1);

			} else if ((i == 0) && (soc != DXL)) { /* first container is always SECO FW */
				int k;

				/* open output file */
				ofd = open("extracted_imgs/ahab-container.img", O_CREAT|O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);

				/* first copy container header to output image */
				ret = write(ofd, (void *)mem_ptr, 1024);
				if (ret < 0)
					fprintf(stderr, "Error writing to output file\n");


				/* next, pad the output with zeros until the start of the image */
				for (k = 0; k < (img_offset-CONTAINER_ALIGNMENT)/4; k++)
					ret = write(ofd, (void *)&pad, 4);

				/* now write the fw image to the output file */
				ret = write(ofd, (void *)(mem_ptr+img_offset), img_size);
				if (ret < 0)
					fprintf(stderr, "Error writing to output file\n");

				/* close output file and unmap input file */
				close(ofd);

				printf("Container %d Image %d -> extracted_imgs/ahab-container.img\n", i+1, j+1);

			} else if ((i < 2 ) && (soc == DXL)) { /* Second Container is Always V2X for DXL */
				if (i == 0) {
					/* open output file */
					ofd = open("extracted_imgs/ahab-container.img", O_CREAT|O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);

					/* first copy container header to output image */
					ret = write(ofd, (void *)mem_ptr, 0x400);
					if (ret < 0)
						fprintf(stderr, "Error writing to output file1\n");

					/* For DXL go to next container to copy header */
					seco_off = img_offset;
					continue;
				} else if (i == 1 && j == 0) {
					int k;

					/* copy v2x container header and seco fw */
					ret = write(ofd,(void *) mem_ptr + file_off, container_hdr->length);
					if (ret < 0)
						fprintf(stderr, "Error writing to output file2\n");


					/* next, pad the output with zeros until the start of SECO image */
					for (k = 0; k < (seco_off - (file_off + container_hdr->length)) / 4; k++)
						ret = write(ofd, (void *)&pad, 4);

					/* now write the SECO fw image to the output file */
					ret = write(ofd, (void *)(mem_ptr+seco_off), file_off + img_offset - seco_off);
					if (ret < 0)
						fprintf(stderr, "Error writing to output file3: %x\n",ret);
				}

				/* now write the next image to the output file */
				ret = write(ofd, (void *)(mem_ptr + file_off + img_offset), img_size);
				if (ret < 0)
					fprintf(stderr, "Error writing to output file4: %x\n",ret);

				/* Iterate through V2X container for other images */
				if (j < (container_hdr->num_images - 1))
					continue;

				/* close output file and unmap input file */
				close(ofd);


				printf("Container %d Image %d -> extracted_imgs/v2x-container.img\n", i+1, j+1);

			} else {
				sprintf(dd_cmd, "dd status=none if=%s of=extracted_imgs/container%d_img%d.bin ibs=1 skip=%d count=%d conv=notrunc",
						ifname, i+1, j+1, file_off+img_offset, img_size);
				printf("Container %d Image %d -> extracted_imgs/container%d_img%d.bin\n", i+1, j+1, i+1, j+1);
			}

			/* run dd command to extract current image from container */
			fprintf(stderr, "FOOO: %s\n", dd_cmd);
			f_ptr = popen(dd_cmd, "r");
			if (f_ptr == NULL) {
				fprintf(stderr, "Failed to extract image\n");
				exit(EXIT_FAILURE);
			}

			/* close the pipe */
			pclose(f_ptr);
		}

		file_off += ALIGN(container_hdr->length, CONTAINER_ALIGNMENT);
		container_hdr++;
	}

	munmap((void *)mem_ptr, buf.st_size);
	printf("Done\n\n");
	return 0;
}

static int parse_container_hdrs_qx_qm_b0(char *ifname, bool extract, soc_type_t soc, off_t file_off)
{
	int ifd; /* container file descriptor */
	int max_containers = (soc == DXL) ? 3 : 2;
	int cntr_num = 0; /* number of containers in binary */
	int img_array_entries = 0; /* number of images in container */
	ssize_t rd_err;
	struct flash_header_v3 container_headers[MAX_NUM_OF_CONTAINER];
	struct flash_header_v3 app_container_header;
	int app_cntr_off;

	/* initialize region of memory where flash header will be stored */
	memset((void *)container_headers, 0, sizeof(container_headers));

	/* open container binary */
	ifd = open(ifname, O_RDONLY|O_BINARY);

	if (file_off) /* inital offset within container binary */
		lseek(ifd, file_off, SEEK_SET);

	while (cntr_num < max_containers) {

		/* read in next container header up to the image array */
		rd_err = read(ifd, (void *)&container_headers[cntr_num], 16);
		if (rd_err == -1) {
			fprintf(stderr, "Error reading from input binary\n");
			exit(EXIT_FAILURE);
		}

		/* check that the current container has a valid tag */
		if (container_headers[cntr_num].tag != IVT_HEADER_TAG_B0)
			break;

		if (container_headers[cntr_num].num_images > MAX_NUM_IMGS) {
			fprintf(stderr, "This container includes %d images, beyond max 8 images\n",
				container_headers[cntr_num].num_images);
			exit(EXIT_FAILURE);
		}

		/* compute the size of the image array */
		img_array_entries = container_headers[cntr_num].num_images * sizeof(struct boot_img);

		/* read in the full image array */
		rd_err = read(ifd, (void *)&container_headers[cntr_num].img, img_array_entries);
		if (rd_err == -1) {
			fprintf(stderr, "Error reading from input binary\n");
			exit(EXIT_FAILURE);
		}

		if (container_headers[cntr_num].sig_blk_offset != 0) {
			/* read in signature block header */
			lseek(ifd, file_off + container_headers[cntr_num].sig_blk_offset, SEEK_SET);
			rd_err = read(ifd, (void *)&container_headers[cntr_num].sig_blk_hdr, sizeof(struct sig_blk_hdr));
			if (rd_err == -1) {
				fprintf(stderr, "Error reading from input binary\n");
				exit(EXIT_FAILURE);
			}
		}

		/* seek to next container in binary */
		file_off += ALIGN(container_headers[cntr_num].length, CONTAINER_ALIGNMENT);
		lseek(ifd, file_off, SEEK_SET);

		/* increment current container count */
		cntr_num++;
	}


	print_container_hdr_fields(container_headers, cntr_num, soc, false);

	if (extract)
		extract_container_images(container_headers, ifname, cntr_num, ifd, soc, 0);

	app_cntr_off = search_app_container(container_headers, cntr_num, ifd, &app_container_header);

	if (app_cntr_off > 0) {
		print_container_hdr_fields(&app_container_header, 1, soc, true);
		if (extract)
			extract_container_images(&app_container_header, ifname, 1, ifd, soc, app_cntr_off);
	}

	close(ifd);

	return 0;

}

#define IMG_STACK_SIZE			32 /* max of 32 images for commandline images */

/*
 * Read commandline parameters and construct the header in order
 *
 * This will then construct the image according to the header and
 *
 * parameters passed in
 *
 */
int main(int argc, char **argv)
{
	int c;
	char *ofname = NULL;
	char *ifname = NULL;
	bool output = false;
	bool dcd_skip = false;
	bool emmc_fastboot = false;
	bool extract = false;
	bool parse = false;

	int container = -1;
	image_t param_stack[IMG_STACK_SIZE];/* stack of input images */
	int p_idx = 0;/* param index counter */
	off_t file_off = 0;

	uint32_t ivt_offset = IVT_OFFSET_SD;
	uint32_t sector_size = 0x400; /* default sector size */
	soc_type_t soc = NONE; /* Initially No SOC defined */

	uint8_t  fuse_version = 0;
	uint16_t sw_version   = 0;
	char     *images_hash = NULL;

	static struct option long_options[] = {
		{"scfw", required_argument, NULL, 'f'},
		{"seco", required_argument, NULL, 'O'},
		{"m4", required_argument, NULL, 'm'},
		{"ap", required_argument, NULL, 'a'},
		{"dcd", required_argument, NULL, 'd'},
		{"out", required_argument, NULL, 'o'},
		{"flags", required_argument, NULL, 'l'},
		{"scd", required_argument, NULL, 'x'},
		{"csf", required_argument, NULL, 'z'},
		{"dev", required_argument, NULL, 'e'},
		{"soc", required_argument, NULL, 's'},
		{"dummy",required_argument, NULL, 'y'},
		{"container", no_argument, NULL, 'c'},
		{"partition", required_argument, NULL, 'p'},
		{"append", no_argument, NULL, 'A'},
		{"data", required_argument, NULL, 'D'},
		{"fileoff", required_argument, NULL, 'P'},
		{"msg_blk", required_argument, NULL, 'M'},
		{"fuse_version", required_argument, NULL, 'u'},
		{"sw_version", required_argument, NULL, 'v'},
		{"images_hash", required_argument, NULL, 'h'},
		{"extract", required_argument, NULL, 'X'},
		{"parse", required_argument, NULL, 'R'},
		{"sentinel", required_argument, NULL, 'i'},
		{"upower", required_argument, NULL, 'w'},
		{"fcb", required_argument, NULL, 'b'},
		{"padding", required_argument, NULL, 'G'},
		{"pblsize", required_argument, NULL, 0x1000},
		{NULL, 0, NULL, 0}
	};

	/* scan in parameters in order */
	while (1) {
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long_only (argc, argv, ":f:m:a:d:o:l:x:z:e:p:cu:v:h:i:w:",
			long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
		case 0:
			fprintf(stderr, "option %s", long_options[option_index].name);
			if (optarg)
				fprintf(stderr, " with arg %s", optarg);
			fprintf(stderr, "\n");
			break;
		case 'A':
			param_stack[p_idx].option = APPEND;
			param_stack[p_idx++].filename = argv[optind++];
			break;
		case 'p':
			printf("PARTITION:\t%s\n", optarg);
			param_stack[p_idx].option = PARTITION;
			param_stack[p_idx++].entry = (uint32_t) strtoll(optarg, NULL, 0);
			break;
		case 's':
			if (!strncmp(optarg, "QX", 2)) {
				soc = QX;
			} else if (!strncmp(optarg, "QM", 2)) {
				soc = QM;
			} else if (!strncmp(optarg, "DXL", 3)) {
				soc = DXL;
			} else if (!strncmp(optarg, "ULP", 3)) {
				soc = ULP;
			} else if (!strncmp(optarg, "IMX9", 4)) {
				soc = IMX9;
			} else {
				printf("unrecognized SOC: %s \n",optarg);
				exit(EXIT_FAILURE);
			}
			printf("SOC: %s \n",optarg);
			break;
		case 'b':
			printf("FCB:\t%s\n", optarg);
			param_stack[p_idx].option = FCB;
			param_stack[p_idx].filename = optarg;
			if (optind < argc && *argv[optind] != '-') {
				param_stack[p_idx].entry = (uint32_t) strtoll(argv[optind++], NULL, 0);
				p_idx++;
			} else {
				fprintf(stderr, "\n-fcb option require Two arguments: filename, load address in hex\n\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'i':
			printf("SENTINEL:\t%s\n", optarg);
			param_stack[p_idx].option = SENTINEL;
			param_stack[p_idx++].filename = optarg;
			break;
		case 'w':
			printf("UPOWER:\t%s\n", optarg);
			param_stack[p_idx].option = UPOWER;
			param_stack[p_idx++].filename = optarg;
			break;
		case 'f':
			printf("SCFW:\t%s\n", optarg);
			param_stack[p_idx].option = SCFW;
			param_stack[p_idx++].filename = optarg;
			break;
		case 'O':
			printf("SECO:\t%s\n", optarg);
			param_stack[p_idx].option = SECO;
			param_stack[p_idx++].filename = optarg;
			break;
		case 'd':
			printf("DCD:\t%s\n", optarg);
			if (soc == DXL) {
				if (!strncmp(optarg, "skip", 4)) {
					dcd_skip = true;
				} else {
					fprintf(stderr, "\n-dcd option requires argument skip\n\n");
					exit(EXIT_FAILURE);
				}
			} else if ((soc == ULP) || (soc == IMX9)) {
				fprintf(stderr, "\n-dcd option is not used on ULP and IMX9\n\n");
				exit(EXIT_FAILURE);
			} else {
				param_stack[p_idx].option = DCD;
				param_stack[p_idx].filename = optarg;
				p_idx++;
			}
			break;
		case 'D':
			if ((soc == DXL) || (soc == ULP) || (soc == IMX9)) {
				printf("Data:\t%s", optarg);
				param_stack[p_idx].option = DATA;
				param_stack[p_idx].filename = optarg;
				if ((optind < argc && *argv[optind] != '-') &&
				    (optind+1 < argc && *argv[optind+1] != '-' )) {
					if (!strncmp(argv[optind], "a53", 3))
						param_stack[p_idx].ext = CORE_CA53;
					else if (!strncmp(argv[optind], "a55", 3))
						param_stack[p_idx].ext = CORE_CA35; /* fake id for a55 */
					else if (!strncmp(argv[optind], "a35", 3))
						param_stack[p_idx].ext = CORE_CA35;
					else if (!strncmp(argv[optind], "a72", 3))
						param_stack[p_idx].ext = CORE_CA72;
					else if (!strncmp(argv[optind], "m4_1", 4))
						param_stack[p_idx].ext = CORE_CM4_1;
					else if (!strncmp(argv[optind], "m4", 2))
						param_stack[p_idx].ext = CORE_CM4_0;
					else {
						fprintf(stderr, "ERROR: incorrect core ID for --data option: %s\n", argv[optind]);
						exit(EXIT_FAILURE);
					}
					printf("\tcore: %s\n", argv[optind++]);
					param_stack[p_idx].entry = (uint32_t) strtoll(argv[optind++], NULL, 0);
				} else {
					fprintf(stderr, "\n-data option require THREE arguments: filename, core: a55/a35/a53/a72/m4_0/m4_1, load address in hex\n\n");
					exit(EXIT_FAILURE);
				}
				p_idx++;
			} else {
				fprintf(stderr, "\n-data option is only used with -rev B0, or DXL or ULP or IMX9 soc.\n\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'm':
			printf("CM4:\t%s", optarg);
			param_stack[p_idx].option = M4;
			param_stack[p_idx].filename = optarg;
			if ((optind < argc && *argv[optind] != '-') &&
			    (optind + 1 < argc &&*argv[optind+1] != '-' )) {
				param_stack[p_idx].ext = strtol(argv[optind++], NULL, 0);
				param_stack[p_idx].entry = (uint32_t) strtoll(argv[optind++], NULL, 0);
				param_stack[p_idx].dst = 0;
				printf("\tcore: %" PRIi64, param_stack[p_idx].ext);
				printf(" entry addr: 0x%08" PRIx64 , param_stack[p_idx].entry);
				if (optind < argc && *argv[optind] != '-') {
					param_stack[p_idx].dst = (uint32_t) strtoll(argv[optind++], NULL, 0);
					printf(" load addr: 0x%08" PRIx64 , param_stack[p_idx].dst);
				}
				printf("\n");
				p_idx++;
			} else {
				fprintf(stderr, "\n-m4 option require FOUR arguments: filename, core: 0/1, entry address in hex, load address in hex(optional)\n\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'a':
			printf("AP:\t%s", optarg);
			param_stack[p_idx].option = AP;
			param_stack[p_idx].filename = optarg;
			if ((optind < argc && *argv[optind] != '-') &&
			    (optind + 1 < argc &&*argv[optind+1] != '-' )) {
				if (!strncmp(argv[optind], "a53", 3))
					param_stack[p_idx].ext = CORE_CA53;
				else if (!strncmp(argv[optind], "a55", 3))
					param_stack[p_idx].ext = CORE_CA35; /* fake id for a55 */
				else if (!strncmp(argv[optind], "a35", 3))
					param_stack[p_idx].ext = CORE_CA35;
				else if (!strncmp(argv[optind], "a72", 3))
					param_stack[p_idx].ext = CORE_CA72;
				else {
					fprintf(stderr, "ERROR: AP Core not found %s\n", argv[optind+2]);
					exit(EXIT_FAILURE);
				}
				printf("\tcore: %s", argv[optind++]);

				param_stack[p_idx].entry = (uint32_t) strtoll(argv[optind++], NULL, 0);
				param_stack[p_idx].mu = SC_R_MU_0A;
				param_stack[p_idx].part = 1;

				if (optind < argc && *argv[optind] != '-') {
					if (!strncmp(argv[optind], "mu0", 3))
						param_stack[p_idx].mu = SC_R_MU_0A;
					else if (!strncmp(argv[optind], "mu3", 3))
						param_stack[p_idx].mu = SC_R_MU_3A;
					else {
						fprintf(stderr, "ERROR: MU number %s not found\n", argv[optind]);
						exit(EXIT_FAILURE);
					}
					printf("\tMU: %s ", argv[optind++]);
				}
				if (optind < argc && *argv[optind] != '-') {
					if (!strncmp(argv[optind], "pt", 2) &&
					    (argv[optind][2] > '0') &&
					    (argv[optind][2] != '2') &&
					    (argv[optind][2] <= '9')) {
						char str[2];
						str[0] = argv[optind][2];
						str[1] = '\0';
						param_stack[p_idx].part = strtoll(str, NULL, 0);
					} else {
						fprintf(stderr, "ERROR: partition number %s not found\n", argv[optind]);
						exit(EXIT_FAILURE);
					}
					printf("\tPartition: %s ", argv[optind++]);
				}
				printf(" addr: 0x%08" PRIx64 "\n", param_stack[p_idx++].entry);
			} else {
				fprintf(stderr, "\n-ap option require THREE arguments: filename, a35/a53/a72, start address in hex\n\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'l':
			printf("FLAG:\t%s\n", optarg);
			param_stack[p_idx].option = FLAG;
			param_stack[p_idx++].entry = (uint32_t) strtoll(optarg, NULL, 0);
			break;
		case 'o':
			printf("Output:\t%s\n", optarg);
			ofname = optarg;
			output = true;
			break;
		case 'x':
			printf("SCD:\t%s\n", optarg);
			param_stack[p_idx].option = SCD;
			param_stack[p_idx++].filename = optarg;
			break;
		case 'z':
			printf("CSF:\t%s\n", optarg);
			param_stack[p_idx].option = CSF;
			param_stack[p_idx++].filename = optarg;
			break;
		case 'e':
			printf("BOOT DEVICE:\t%s\n", optarg);
			if (!strcmp(optarg, "flexspi")) {
				ivt_offset = IVT_OFFSET_FLEXSPI;
			} else if (!strcmp(optarg, "sd")) {
				ivt_offset = IVT_OFFSET_SD;
			} else if (!strcmp(optarg, "nand")) {
				sector_size = 0x8000;/* sector size for NAND */
				if ((soc == DXL) || (soc == IMX9)) {
					if (optind < argc && *argv[optind] != '-') {
						if (!strcmp(argv[optind], "4K")) {
							sector_size = 0x1000;
						} else if (!strcmp(argv[optind], "8K")) {
							sector_size = 0x2000;
						} else if (!strcmp(argv[optind], "16K")) {
							sector_size = 0x4000;
						} else
							printf("\nwrong nand page size:\r\n 4K\r\n8K\r\n16K\n\n");
					} else {
						printf("\n-dev nand requires the page size:\r\n 4K\r\n8K\r\n16K\n\n");
					}
				}
			} else if (!strcmp(optarg, "emmc_fast")) {
					ivt_offset = IVT_OFFSET_EMMC;
					emmc_fastboot = true;/* emmc boot */
			} else {
				printf("\n-dev option, Valid boot devices are:\r\n sd\r\nflexspi\r\nnand\n\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'c':
			printf("New Container: \t%d\n",++container);
			param_stack[p_idx++].option = NEW_CONTAINER;
			break;
		case ':':
			fprintf(stderr, "option %c missing arguments\n", optopt);
			exit(EXIT_FAILURE);
			break;
		case 'P':
			printf("FILEOFF:\t%s\n", optarg);
			param_stack[p_idx].option = FILEOFF;
			param_stack[p_idx++].dst = (uint64_t) strtoll(optarg, NULL, 0);
			break;
		case 'M':
			printf("MSG BLOCK:\t%s", optarg);
			param_stack[p_idx].option = MSG_BLOCK;
			param_stack[p_idx].filename = optarg;
			if ((optind < argc && *argv[optind] != '-') &&
			    (optind+1 < argc &&*argv[optind + 1] != '-' )) {
				if (!strncmp(argv[optind], "fuse", 4))
					param_stack[p_idx].ext = SC_R_OTP;
				else if (!strncmp(argv[optind], "debug", 5))
					param_stack[p_idx].ext = SC_R_DEBUG;
				else if (!strncmp(argv[optind], "field", 5))
					param_stack[p_idx].ext = SC_R_ROM_0;
				else if (!strncmp(argv[optind], "zero", 4))
					param_stack[p_idx].ext = SC_R_PWM_0;
				else if (!strncmp(argv[optind], "patch", 5))
					param_stack[p_idx].ext = SC_R_SNVS;
				else if (!strncmp(argv[optind], "degrade", 7))
					param_stack[p_idx].ext = SC_R_DC_0;
				else {
					fprintf(stderr, "ERROR: MSG type not found %s\n", argv[optind+2]);
					exit(EXIT_FAILURE);
				}
				printf("\ttype: %s", argv[optind++]);

				param_stack[p_idx].entry = (uint32_t) strtoll(argv[optind++], NULL, 0);

				printf(" addr: 0x%08" PRIx64 "\n", param_stack[p_idx++].entry);
			} else {
				fprintf(stderr, "\nmsg block option require THREE arguments: filename, debug/fuse/field/patch, start address in hex\n\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'u':
			fuse_version = (uint8_t) (strtoll(optarg, NULL, 0) & 0xFF);
			break;
		case 'v':
			sw_version = (uint16_t) (strtoll(optarg, NULL, 0) & 0xFFFF);
			break;
		case 'h':
			images_hash = optarg;
			break;
		case 'X':
			printf("Input container binary to be deconstructed: %s\n", optarg);
			ifname = optarg;
			extract = true;
			break;
		case 'R':
			printf("Input container binary to be parsed: %s\n", optarg);
			ifname = optarg;
			parse = true;
			break;
		case 'y':
			printf("Dummy V2X image at:\t%s\n", optarg);
			param_stack[p_idx].option = DUMMY_V2X;
			param_stack[p_idx++].entry = (uint64_t) strtoll(optarg, NULL, 0);
			break;
		case 'G':
			printf("Padding length:\t%s bytes\n", optarg);
			file_off = atoi(optarg);
			break;
		case 0x1000:
			pblsize = atoi(optarg);
			break;
		case '?':
		default:
			/* invalid option */
			fprintf(stderr, "option '%c' is invalid: ignored\n",
				optopt);
			exit(EXIT_FAILURE);
		}
	}

	if (!parse) {
		printf("CONTAINER FUSE VERSION:\t0x%02x\n", fuse_version);
		printf("CONTAINER SW VERSION:\t0x%04x\n", sw_version);
	}

	param_stack[p_idx].option = NO_IMG; /* null terminate the img stack */

	if (soc == NONE) {
		fprintf(stderr, " No SOC defined");
		exit(EXIT_FAILURE);
	}

	if (parse || extract) {
		parse_container_hdrs_qx_qm_b0(ifname, extract, soc, file_off);
		return 0;
	}

	if (container < 0) {
		/* check to make sure there is at least 1 container defined */
		fprintf(stderr, " No Container defined");
		exit(EXIT_FAILURE);
	}

	if (!output) {
		fprintf(stderr, "mandatory args scfw and output file name missing! abort\n");
		exit(EXIT_FAILURE);
	}

	build_container_qx_qm_b0(soc, sector_size, ivt_offset, ofname, emmc_fastboot,
				 (image_t *) param_stack, dcd_skip, fuse_version,
				 sw_version, images_hash);

	printf("DONE.\n");
	printf("Note: Please copy image to offset: IVT_OFFSET + IMAGE_OFFSET\n");

	return 0;
}

