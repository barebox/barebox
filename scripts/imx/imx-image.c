// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Sascha Hauer, Pengutronix

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/kernel.h>
#include <sys/file.h>
#include "../compiler.h"
#include "../common.h"

#include "imx.h"

#include <include/filetype.h>
#include <include/linux/sizes.h>

#define FLASH_HEADER_OFFSET 0x400
#define ARM_HEAD_SIZE_INDEX	(ARM_HEAD_SIZE_OFFSET / sizeof(uint32_t))

#include "../common.c"

/*
 * Conservative DCD element limit set to restriction v2 header size to
 * HEADER_SIZE
 */
#define MAX_DCD ((HEADER_LEN - FLASH_HEADER_OFFSET - sizeof(struct imx_flash_header_v2)) / sizeof(u32))

static uint32_t dcdtable[MAX_DCD];
static int curdcd;
static bool create_usb_image;
static char *prgname;

/*
 * ============================================================================
 * i.MX flash header v1 handling. Found on i.MX35 and i.MX51
 * ============================================================================
 */


static uint32_t bb_header_aarch32[] = {
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0x65726162,	/* 'bare'   */
	0x00786f62,	/* 'box\0'  */
	0x00000000,
	0x00000000,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
};

static uint32_t bb_header_aarch64[] = {
	0x14000000,	/* 1: b 1b  */
	0x14000000,	/* 1: b 1b  */
	0x14000000,	/* 1: b 1b  */
	0x14000000,	/* 1: b 1b  */
	0x14000000,	/* 1: b 1b  */
	0x14000000,	/* 1: b 1b  */
	0x14000000,	/* 1: b 1b  */
	0x14000000,	/* 1: b 1b  */
	0x65726162,	/* 'bare'   */
	0x00786f62,	/* 'box\0'  */
	0x00000000,
	0x00000000,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
};

struct hab_rsa_public_key {
	uint8_t rsa_exponent[4]; /* RSA public exponent */
	uint32_t rsa_modulus; /* RSA modulus pointer */
	uint16_t exponent_size; /* Exponent size in bytes */
	uint16_t modulus_size; /* Modulus size in bytes*/
	uint8_t init_flag; /* Indicates if key initialized */
};

#ifdef IMXIMAGE_SSL_SUPPORT
#define PUBKEY_ALGO_LEN 2048

#include <openssl/x509v3.h>
#include <openssl/bn.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
void RSA_get0_key(const RSA *r, const BIGNUM **n,
		  const BIGNUM **e, const BIGNUM **d)
{
	if (n != NULL)
		*n = r->n;
	if (e != NULL)
		*e = r->e;
	if (d != NULL)
		*d = r->d;
}

RSA *EVP_PKEY_get0_RSA(EVP_PKEY *pkey)
{
    if (pkey->type != EVP_PKEY_RSA)
        return NULL;

    return pkey->pkey.rsa;
}
#endif

static int extract_key(const char *certfile, uint8_t **modulus, int *modulus_len,
	uint8_t **exponent, int *exponent_len)
{
	const BIGNUM *n, *e;
	EVP_PKEY *pkey;
	FILE *fp;
	X509 *cert;
	RSA *rsa_key;

	fp = fopen(certfile, "r");
	if (!fp) {
		fprintf(stderr, "unable to open certfile %s: %s\n", certfile,
			strerror(errno));
		return -errno;
	}

	cert = PEM_read_X509(fp, NULL, NULL, NULL);
	if (!cert) {
		fprintf(stderr, "unable to parse certificate in: %s\n", certfile);
		fclose(fp);
		return -errno;
	}

	fclose(fp);

	pkey = X509_get_pubkey(cert);
	if (!pkey) {
		fprintf(stderr, "unable to extract public key from certificate");
		return -EINVAL;
	}

	rsa_key = EVP_PKEY_get0_RSA(pkey);
	if (!rsa_key) {
		fprintf(stderr, "unable to extract RSA public key");
		return -EINVAL;
	}

	RSA_get0_key(rsa_key, &n, &e, NULL);
	*modulus_len = BN_num_bytes(n);
	*modulus = malloc(*modulus_len);
	BN_bn2bin(n, *modulus);

	*exponent_len = BN_num_bytes(e);
	*exponent = malloc(*exponent_len);
	BN_bn2bin(e, *exponent);

	EVP_PKEY_free(pkey);
	X509_free(cert);

	return 0;
}

static int add_srk(void *buf, int offset, uint32_t loadaddr, const char *srkfile)
{
	struct imx_flash_header *hdr = buf + offset;
	struct hab_rsa_public_key *key = buf + 0xc00;
	uint8_t *exponent = NULL, *modulus = NULL, *modulus_dest;
	int exponent_len = 0, modulus_len = 0;
	int ret;

	hdr->super_root_key = loadaddr + 0xc00;

	key->init_flag = 1;
	key->exponent_size = htole16(3);

	ret = extract_key(srkfile, &modulus, &modulus_len, &exponent, &exponent_len);
	if (ret)
		return ret;

	modulus_dest = (void *)(key + 1);

	memcpy(modulus_dest, modulus, modulus_len);

	key->modulus_size = htole16(modulus_len);
	key->rsa_modulus = htole32(hdr->super_root_key + sizeof(*key));

	if (exponent_len > 4)
		return -EINVAL;

	key->exponent_size = exponent_len;
	memcpy(&key->rsa_exponent, exponent, key->exponent_size);

	return 0;
}
#else
static int add_srk(void *buf, int offset, uint32_t loadaddr, const char *srkfile)
{
	fprintf(stderr, "This version of imx-image is compiled without SSL support\n");

	return -EINVAL;
}
#endif /* IMXIMAGE_SSL_SUPPORT */

static int dcd_ptr_offset;
static uint32_t dcd_ptr_content;

static size_t add_header_v1(struct config_data *data, void *buf)
{
	struct imx_flash_header *hdr;
	int dcdsize = curdcd * sizeof(uint32_t);
	int offset = data->image_ivt_offset;
	uint32_t loadaddr = data->image_load_addr;
	uint32_t imagesize = data->load_size;

	buf += offset;
	hdr = buf;

	hdr->app_code_jump_vector = loadaddr + 0x1000;
	hdr->app_code_barker = 0x000000b1;
	hdr->app_code_csf = 0x0;
	hdr->dcd_ptr_ptr = loadaddr + offset + offsetof(struct imx_flash_header, dcd);
	hdr->super_root_key = 0x0;
	hdr->dcd =  loadaddr + offset + offsetof(struct imx_flash_header, dcd_barker);

	hdr->app_dest = loadaddr;
	hdr->dcd_barker = DCD_BARKER;
	if (create_usb_image) {
		dcd_ptr_offset = offsetof(struct imx_flash_header, dcd_block_len) + offset;
		hdr->dcd_block_len = 0;
		dcd_ptr_content = dcdsize;
	} else {
		hdr->dcd_block_len = dcdsize;
	}

	buf += sizeof(struct imx_flash_header);

	memcpy(buf, dcdtable, dcdsize);

	buf += dcdsize;

	if (data->csf) {
		hdr->app_code_csf = loadaddr + imagesize;
		imagesize += CSF_LEN;
	}

	*(uint32_t *)buf = imagesize;

	return imagesize;
}

static int write_mem_v1(uint32_t addr, uint32_t val, int width, int set_bits, int clear_bits)
{
	if (set_bits || clear_bits) {
		fprintf(stderr, "This SoC does not support setting/clearing bits\n");
		return -EINVAL;
	}

	if (curdcd > MAX_DCD - 3) {
		fprintf(stderr, "At maximum %d dcd entried are allowed\n", (int)MAX_DCD);
		return -ENOMEM;
	}

	dcdtable[curdcd++] = width;
	dcdtable[curdcd++] = addr;
	dcdtable[curdcd++] = val;

	return 0;
}

/*
 * ============================================================================
 * i.MX flash header v2 handling. Found on i.MX50, i.MX53 and i.MX6
 * ============================================================================
 */

static size_t
add_header_v2(const struct config_data *data, void *buf, uint32_t offset,
	      size_t header_len, unsigned int csf_slot)
{
	struct imx_flash_header_v2 *hdr;
	int dcdsize = curdcd * sizeof(uint32_t);
	uint32_t loadaddr = data->image_load_addr;
	uint32_t imagesize = data->load_size;

	if (data->pbl_code_size) {
		/*
		 * Restrict the imagesize to the PBL if given.
		 * Also take the alignment for CSF into account.
		 */
		imagesize = roundup(data->pbl_code_size + header_len, 0x4);
		if (data->csf)
			imagesize = roundup(imagesize, 0x1000);
	}

	buf += offset;
	hdr = buf;

	hdr->header.tag		= TAG_IVT_HEADER;
	hdr->header.length	= htobe16(32);
	hdr->header.version	= IVT_VERSION;

	/* IMX_CPU_IMX8MQ is a special case with fixed offset */
	if (data->cpu_type == IMX_CPU_IMX8MQ)
		hdr->entry	= loadaddr + HEADER_LEN;
	else
		hdr->entry	= loadaddr + header_len;
	if (dcdsize)
		hdr->dcd_ptr = loadaddr + offset + offsetof(struct imx_flash_header_v2, dcd_header);
	if (create_usb_image) {
		dcd_ptr_content = hdr->dcd_ptr;
		dcd_ptr_offset = offsetof(struct imx_flash_header_v2, dcd_ptr) + offset;
		hdr->dcd_ptr = 0;
	}
	hdr->boot_data_ptr	= loadaddr + offset + offsetof(struct imx_flash_header_v2, boot_data);
	hdr->self		= loadaddr + offset;

	hdr->boot_data.start	= loadaddr;
	if (!data->csf && data->max_load_size
	    && imagesize > data->max_load_size)
		hdr->boot_data.size	= data->max_load_size;
	else
		hdr->boot_data.size	= imagesize;

	if (data->sign_image) {
		hdr->csf = loadaddr + imagesize + (csf_slot * CSF_LEN);
		hdr->boot_data.size += CSF_LEN;
		if (data->flexspi_csf)
			hdr->boot_data.size += CSF_LEN;
	} else if (data->pbl_code_size && data->csf) {
		/*
		 * For i.MX8 the CSF space is added via the linker script, so
		 * the CSF length needs to be added if HABV4 is enabled but
		 * signing is not.
		 */
		hdr->boot_data.size += CSF_LEN;
		if (data->flexspi_csf)
			hdr->boot_data.size += CSF_LEN;
	}

	buf += sizeof(*hdr);

	if (dcdsize) {
		hdr->dcd_header.tag	= TAG_DCD_HEADER;
		hdr->dcd_header.length	= htobe16(sizeof(uint32_t) + dcdsize);
		hdr->dcd_header.version	= DCD_VERSION;
		memcpy(buf, dcdtable, dcdsize);
	}

	return imagesize;
}

#define LUT_PAD_1	0
#define LUT_PAD_2	1
#define LUT_PAD_4	2
#define LUT_PAD_8	3

static size_t add_flexspi_fcfb_header(const struct config_data *data, void *buf)
{
	uint32_t fcfb_offset = data->image_flexspi_fcfb_offset;
	const struct imx_fcfb_nor nor_conf = {
		.memcfg = {
			.tag = htobe32(FCFB_HEAD_TAG),
			.version = htole32(FCFB_VERSION),
			.read_sample = FCFB_SAMLPE_CLK_SRC_INTERNAL,
			/* flash CS hold time, recommended by RM */
			.datahold =  0x03,
			/* flash CS setup time, recommended by RM */
			.datasetup = 0x03,
			/* 3 - Hyperflash, 12/13 serial NAND, 0 - other */
			.coladdrwidth = 0,
			.devcfgenable = 0,
			.cmd_enable =  0,
			.controllermisc =  0,
			.dev_type = FCFB_DEVTYPE_SERIAL_NOR,
			.sflash_pad = FCFB_SFLASH_PADS_SINGLE,
			.serial_clk = FCFB_SERIAL_CLK_FREQ_50MHZ,
			.sflashA1 = htole32(SZ_256M),
			.lut.seq[0] = {
				.instr = {
					htole16(LUT_DEF(LUT_CMD, LUT_PAD_1, 0x0b)),
					htole16(LUT_DEF(LUT_ADDR, LUT_PAD_1, 24)),
					htole16(LUT_DEF(LUT_DUMMY, LUT_PAD_1, 8)),
					htole16(LUT_DEF(LUT_NXP_READ, LUT_PAD_1, 4)),
					htole16(LUT_DEF(LUT_STOP, LUT_PAD_1, 0)),
				},
			},
		},
	};

	buf += fcfb_offset;
	memcpy(buf, &nor_conf, sizeof(nor_conf));

	return sizeof(nor_conf);
}

static size_t
add_flexspi_header(const struct config_data *data, void **_buf, size_t *header_len)
{
	uint32_t ivt_offset = data->image_flexspi_ivt_offset;
	size_t size;
	size_t len;
	void *buf;

	if (!flexspi_image(data))
		return 0;

	if (data->signed_hdmi_firmware_file) {
		free(*_buf);
		fprintf(stderr, "Signed HDMI firmware and FlexSPI compatible image is not supported!\n");
		exit(1);
	}

	/*
	 * Extend the header to be able to build build one image which can be
	 * used for: USB/SD/eMMC/eMMC-Boot/QSPI/barebox-chainload.
	 */
	buf = realloc(*_buf, *header_len + FLEXSPI_HEADER_LEN);
	if (!buf)
		exit(1);

	*_buf = buf;

	size = add_flexspi_fcfb_header(data, buf);

	/*
	 * The following table list the offsets we need to ensure for
	 * the one image approach.
	 *
	 *                              | i.MX8MM | i.MX8MN/P |
	 * -----------------------------+---------+-----------+
	 * SD/eMMC primary image offset |    0    |   0/32K   |
	 * FlexSPI primary image offset |    0    |     4K    |
	 * SD/eMMC-IVT offset           |    1K   |     0     |
	 * SD/eMMC-IVT image entry      |    8K   |     8K    |
	 * FlexSPI-IVT offset           |    4K   |     0     |
	 * FlexSPI-IVT image entry      |    8K   |     4K    |
	 *
	 * According the above table the rom-loader for i.MX8MM will
	 * search for the image on the same place (8K). On the other
	 * hand the rom-loader for the i.MX8MN/P will look for it at
	 * 8K for SD/eMMC case or at 4K for FlexSPI case.
	 */
	len = *header_len;
	if (data->cpu_type == IMX_CPU_IMX8MM)
		len += FLEXSPI_HEADER_LEN;

	if (data->cpu_type == IMX_CPU_IMX8MP ||
	    data->cpu_type == IMX_CPU_IMX8MN)
		buf += SZ_4K;

	size += add_header_v2(data, buf, ivt_offset, len, 1);

	*header_len += FLEXSPI_HEADER_LEN;

	return size;
}

static void usage(const char *prgname)
{
	fprintf(stderr, "usage: %s [OPTIONS]\n\n"
		"-c <config>  specify configuration file\n"
		"-f <input>   input image file\n"
		"-o <output>  output file\n"
		"-b           add barebox header to image. If used, barebox recognizes\n"
		"             the image as regular barebox image which can be used as\n"
		"             second stage image\n"
		"-d           write DCD table only\n"
		"-e           prepare image for encryption and use Freescale's Code Signing\n"
		"             to encrypt image. Note that the device-specific encapsulated\n"
		"             DEK as cryptgraphic blob needs to be appended afterwards\n"
		"-s           use Freescale's Code Signing Tool (CST) to sign the image\n"
		"             'cst' is expected to be in PATH or given via the environment\n"
		"             variable 'CST'\n"
		"-u           create USB image suitable for imx-usb-loader\n"
		"             necessary for signed images (-s) only\n"
		"-h           this help\n", prgname);
	exit(1);
}

static uint32_t last_write_cmd;
static int last_cmd_len;
static uint32_t *last_dcd;

static void check_last_dcd(uint32_t cmd)
{
	cmd &= 0xff0000ff;

	if (cmd == last_write_cmd) {
		last_cmd_len += sizeof(uint32_t) * 2;
		return;
	}

	/* write length ... */
	if (last_write_cmd)
		*last_dcd = htobe32(last_write_cmd | (last_cmd_len << 8));

	if ((cmd >> 24) == TAG_WRITE) {
		last_write_cmd = cmd;
		last_dcd = &dcdtable[curdcd++];
		last_cmd_len = sizeof(uint32_t) * 3;
	} else {
		last_write_cmd = 0;
	}
}

static int write_mem_v2(uint32_t addr, uint32_t val, int width, int set_bits, int clear_bits)
{
	uint32_t cmd;

	cmd = (TAG_WRITE << 24) | width;

	if (set_bits && clear_bits)
		return -EINVAL;

	if (set_bits)
		cmd |= 3 << 3;
	if (clear_bits)
		cmd |= 1 << 3;

	if (curdcd > MAX_DCD - 3) {
		fprintf(stderr, "At maximum %d dcd entried are allowed\n", (int)MAX_DCD);
		return -ENOMEM;
	}

	check_last_dcd(cmd);

	dcdtable[curdcd++] = htobe32(addr);
	dcdtable[curdcd++] = htobe32(val);

	return 0;
}

static void xread(int fd, void *buf, int len)
{
	int ret;

	while (len) {
		ret = read(fd, buf, len);
		if (ret < 0) {
			fprintf(stderr, "read failed: %s\n", strerror(errno));
			exit(1);
		}

		if (!ret)
			return;
		buf += ret;
		len -= ret;
	}
}

static void xwrite(int fd, void *buf, int len)
{
	int ret;

	while (len) {
		ret = write(fd, buf, len);
		if (ret < 0) {
			fprintf(stderr, "write failed: %s\n", strerror(errno));
			exit(1);
		}
		buf += ret;
		len -= ret;
	}
}

static void write_dcd(const char *outfile)
{
	int outfd;
	int dcdsize = curdcd * sizeof(uint32_t);

	outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (outfd < 0) {
		fprintf(stderr, "Cannot open %s for wrinting: %s\n", outfile,
			strerror(errno));
		exit(1);
	}

	xwrite(outfd, dcdtable, dcdsize);
}

static int check(const struct config_data *data, uint32_t cmd, uint32_t addr,
		 uint32_t mask)
{
	if (data->header_version != 2) {
		fprintf(stderr, "DCD check command is not available or "
				"not yet implemented for this SOC\n");
		return -EINVAL;
	}
	if (curdcd > MAX_DCD - 3) {
		fprintf(stderr, "At maximum %d dcd entried are allowed\n", (int)MAX_DCD);
		return -ENOMEM;
	}

	check_last_dcd(cmd);

	dcdtable[curdcd++] = htobe32(cmd);
	dcdtable[curdcd++] = htobe32(addr);
	dcdtable[curdcd++] = htobe32(mask);

	return 0;
}

static int write_mem(const struct config_data *data, uint32_t addr,
		     uint32_t val, int width, int set_bits, int clear_bits)
{
	switch (data->header_version) {
	case 1:
		return write_mem_v1(addr, val, width, set_bits, clear_bits);
	case 2:
		return write_mem_v2(addr, val, width, set_bits, clear_bits);
	default:
		return -EINVAL;
	}
}

static int nop(const struct config_data *data)
{
	const struct imx_ivt_header nop_header = {
		.tag = TAG_NOP,
		.length = htobe16(4),
		.version = 0,
	};

	switch (data->header_version) {
	case 1:
		fprintf(stderr, "DCD command NOP not implemented on DCD v1\n");
		return -EINVAL;
	case 2:
		if (curdcd > MAX_DCD - 1) {
			fprintf(stderr, "At maximum %d DCD entries allowed\n",
				(int)MAX_DCD);
			return -ENOMEM;
		}

		check_last_dcd(*((uint32_t *) &nop_header));
		dcdtable[curdcd++] = *((uint32_t *) &nop_header);
		return 0;
	default:
		return -EINVAL;
	}
}

/*
 * This uses the Freescale Code Signing Tool (CST) to sign the image.
 * The cst is expected to be executable as 'cst' or if exists, the content
 * of the environment variable 'CST' is used.
 */
static int hab_sign(struct config_data *data, const char *csfcmds,
		    unsigned int csf_slot)
{
	int fd, outfd, ret, lockfd;
	char *csffile, *command;
	struct stat s;
	char *cst, *cstopts;
	void *buf;
	size_t csf_space = CSF_LEN;
	unsigned int offset = 0;

	cst = getenv("CST");
	if (!cst)
		cst = "cst";

	cstopts = getenv("CST_EXTRA_CMDLINE_OPTIONS");
	if (!cstopts)
		cstopts = "";

	ret = asprintf(&csffile, "%s.slot%u.csfbin", data->outfile, csf_slot);
	if (ret < 0)
		exit(1);

	ret = stat(csffile, &s);
	if (!ret) {
		if (S_ISREG(s.st_mode)) {
			ret = unlink(csffile);
			if (ret) {
				fprintf(stderr, "Cannot remove %s: %s\n",
					csffile, strerror(errno));
				return -errno;
			}
		} else {
			fprintf(stderr, "%s exists and is no regular file\n",
				csffile);
			return -EINVAL;
		}
	}

	/*
	 * Older versions of "cst" want to read the CSF from STDIN,
	 * while newer versions want to read the CSF from a
	 * file. Sadly, the "-i" option doesn't understand "-i -" to
	 * read from STDIN, so we give it "/dev/stdin" instead.
	 */
	ret = asprintf(&command,
		       "%s | grep 'Input CSF text filename'",
		       cst);
	if (ret < 0)
		return -ENOMEM;

	ret = system(command);
	free(command);
	if (ret == -1)
		return -EINVAL;
	else if (ret == 0)
		ret = asprintf(&command, "%s -o %s -i /dev/stdin %s",
			       cst, csffile, cstopts);
	else
		ret = asprintf(&command, "%s -o %s %s;",
			       cst, csffile, cstopts);
	if (ret < 0)
		return -ENOMEM;

	/*
	 * The cst uses "csfsig.bin" as temporary file. This of course breaks when it's
	 * called multiple times as often happens with parallel builds. Until cst learns
	 * how to properly create temporary files without races lock accesses to this
	 * file.
	 */
	lockfd = open(prgname, O_RDONLY);
	if (lockfd < 0) {
		fprintf(stderr, "Cannot open csfsig.bin: %s\n", strerror(errno));
		return -errno;
	}

	ret = flock(lockfd, LOCK_EX);
	if (ret) {
		fprintf(stderr, "Cannot lock csfsig.bin: %s\n", strerror(errno));
		return -errno;
	}

	FILE *f = popen(command, "w");
	if (!f) {
		perror("popen");
		return -errno;
	}

	fwrite(csfcmds, 1, strlen(csfcmds) + 1, f);

	pclose(f);

	flock(lockfd, LOCK_UN);
	close(lockfd);

	/*
	 * the Freescale code signing tool doesn't fail if there
	 * are errors in the command sequence file, it just doesn't
	 * produce any output, so we have to check for existence of
	 * the output file rather than checking the return value of
	 * the cst call.
	 */
	fd = open(csffile, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s: %s\n", csffile, strerror(errno));
		fprintf(stderr, "%s failed\n", cst);
		return -errno;
	}

	ret = fstat(fd, &s);
	if (ret < 0) {
		fprintf(stderr, "stat failed: %s\n", strerror(errno));
		return -errno;
	}

	/*
	 * DEK blob needs to be part of CSF area, in order to properly
	 * load by ROM code. Make space to simply concatenate DEK blob
	 * to the end of image during device flashing procedure.
	 */
	if (data->encrypt_image)
		csf_space -= (data->dek_size + DEK_BLOB_OVERHEAD);

	buf = malloc(csf_space);
	if (!buf)
		return -ENOMEM;

	memset(buf, 0x5a, csf_space);

	if (s.st_size > csf_space) {
		fprintf(stderr, "CSF file size exceeds maximum CSF space of %zu bytes\n",
			csf_space);
	}

	xread(fd, buf, s.st_size);

	close(fd);

	/*
	 * For i.MX8M, write into the reserved CSF section
	 */
	if (cpu_is_mx8m(data))
		outfd = open(data->outfile, O_WRONLY);
	else
		outfd = open(data->outfile, O_WRONLY | O_APPEND);

	if (outfd < 0) {
		fprintf(stderr, "Cannot open %s for writing: %s\n", data->outfile,
			strerror(errno));
		exit(1);
	}

	if (cpu_is_mx8m(data)) {
		/*
		 * For i.MX8 insert the CSF data into the reserved CSF area
		 * right behind the PBL
		 */
		offset = data->header_gap + data->pbl_code_size + HEADER_LEN;
		if (flexspi_image(data))
			offset += FLEXSPI_HEADER_LEN;

		offset += csf_slot * CSF_LEN;

		offset = roundup(offset, 0x1000);
		if (data->signed_hdmi_firmware_file)
			offset += PLUGIN_HDMI_SIZE;

		if (lseek(outfd, offset, SEEK_SET) < 0) {
			perror("lseek");
			exit(1);
		}
	}

	xwrite(outfd, buf, csf_space);

	ret = close(outfd);
	if (ret) {
		perror("close");
		exit(1);
	}

	return 0;
}

static bool cpu_is_aarch64(const struct config_data *data)
{
	return cpu_is_mx8m(data);
}

int main(int argc, char *argv[])
{
	int opt, ret;
	char *configfile = NULL;
	char *imagename = NULL;
	void *buf;
	size_t insize;
	void *infile;
	struct stat s;
	int outfd;
	bool dcd_only = false;
	int now = 0;
	bool add_barebox_header = false;
	uint32_t barebox_image_size = 0;
	struct config_data data = {
		.image_ivt_offset = 0xffffffff,
		.write_mem = write_mem,
		.check = check,
		.nop = nop,
	};
	uint32_t *bb_header;
	size_t sizeof_bb_header;
	size_t header_len = HEADER_LEN;
	size_t signed_hdmi_firmware_size = 0;

	prgname = argv[0];

	while ((opt = getopt(argc, argv, "c:hf:o:p:bduse")) != -1) {
		switch (opt) {
		case 'c':
			configfile = optarg;
			break;
		case 'f':
			imagename = optarg;
			break;
		case 'o':
			data.outfile = optarg;
			break;
		case 'p':
			data.pbl_code_size = strtoul(optarg, NULL, 0);
			break;
		case 'b':
			add_barebox_header = true;
			break;
		case 'd':
			dcd_only = true;
			break;
		case 's':
			data.sign_image = 1;
			break;
		case 'u':
			create_usb_image = true;
			break;
		case 'e':
			data.encrypt_image = 1;
			break;
		case 'h':
			usage(argv[0]);
		default:
			exit(1);
		}
	}

	if (!imagename && !dcd_only) {
		fprintf(stderr, "image name not given\n");
		exit(1);
	}

	if (!configfile) {
		fprintf(stderr, "config file not given\n");
		exit(1);
	}

	if (!data.outfile) {
		fprintf(stderr, "output file not given\n");
		exit(1);
	}

	if (!dcd_only) {
		ret = stat(imagename, &s);
		if (ret) {
			perror("stat");
			exit(1);
		}

		data.image_size = s.st_size;
	}

	/*
	 * Add HEADER_LEN to the image size for the blank area + IVT + DCD.
	 * Align up to a 4k boundary, because:
	 * - at least i.MX5 NAND boot only reads full NAND pages and misses the
	 *   last partial NAND page.
	 * - i.MX6 SPI NOR boot corrupts the last few bytes of an image loaded
	 *   in very funny ways when the image size is not 4 byte aligned
	 */
	data.load_size = roundup(data.image_size + header_len, 0x1000);

	ret = parse_config(&data, configfile);
	if (ret)
		exit(1);

	if (data.max_load_size && (data.encrypt_image || data.csf)
	    && !cpu_is_mx8m(&data)) {
		fprintf(stderr, "Specifying max_load_size is incompatible with HAB signing/encrypting\n");
		exit(1);
	}

	if (create_usb_image && !data.csf) {
		fprintf(stderr, "Warning: the -u option only has effect with signed images\n");
		create_usb_image = 0;
	}

	if (data.image_ivt_offset == 0xffffffff) {
		if (create_usb_image)
			data.image_ivt_offset = 0x0;
		else
			data.image_ivt_offset = FLASH_HEADER_OFFSET;
	}

	if (!data.header_version) {
		fprintf(stderr, "no SoC given. (missing 'soc' in config)\n");
		exit(1);
	}

	if (data.header_version == 2)
		check_last_dcd(0);

	if (dcd_only) {
		write_dcd(data.outfile);
		exit(0);
	}

	buf = calloc(1, header_len);
	if (!buf)
		exit(1);

	switch (data.header_version) {
	case 1:
		barebox_image_size = add_header_v1(&data, buf);
		if (data.srkfile) {
			ret = add_srk(buf, data.image_ivt_offset, data.image_load_addr,
				      data.srkfile);
			if (ret)
				exit(1);
		}
		break;
	case 2:
		if (data.image_ivt_offset + sizeof(struct imx_flash_header_v2) +
		    MAX_DCD * sizeof(u32) > HEADER_LEN) {
			fprintf(stderr, "i.MX v2 header exceeds SW limit set by imx-image\n");
			exit(1);
		}

		if (data.signed_hdmi_firmware_file) {
			free(buf);
			buf = read_file(data.signed_hdmi_firmware_file,
					&signed_hdmi_firmware_size);
			if (!buf)
				exit(1);

			signed_hdmi_firmware_size =
				roundup(signed_hdmi_firmware_size,
					PLUGIN_HDMI_SIZE);

			header_len += signed_hdmi_firmware_size;

			barebox_image_size += signed_hdmi_firmware_size;

			buf = realloc(buf, header_len);
			if (!buf) {
				perror("realloc");
				exit(1);
			}
		}

		barebox_image_size += add_flexspi_header(&data, &buf, &header_len);
		barebox_image_size += add_header_v2(&data, buf +
						    signed_hdmi_firmware_size,
						    data.image_ivt_offset, header_len, 0);
		break;
	default:
		fprintf(stderr, "Congratulations! You're welcome to implement header version %d\n",
				data.header_version);
		exit(1);
	}

	if (cpu_is_aarch64(&data)) {
		bb_header = bb_header_aarch64;
		sizeof_bb_header = sizeof(bb_header_aarch64);
		/*
		 * Compute jump offset, must be done dynamically as the code
		 * location changes depending on the presence of a signed HDMI
		 * firmware.
		 */
		data.first_opcode |= (data.header_gap + header_len) >> 2;
	} else {
		bb_header = bb_header_aarch32;
		sizeof_bb_header = sizeof(bb_header_aarch32);
	}

	bb_header[0] = data.first_opcode;
	bb_header[ARM_HEAD_SIZE_INDEX] = barebox_image_size;

	infile = read_file(imagename, &insize);
	if (!infile)
		exit(1);

	outfd = open(data.outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (outfd < 0) {
		fprintf(stderr, "Cannot open %s for writing: %s\n", data.outfile,
			strerror(errno));
		exit(1);
	}

	if (data.cpu_type == IMX_CPU_IMX35) {
		xwrite(outfd, buf, header_len);
		xwrite(outfd, buf, header_len);
	} else {
		if (add_barebox_header &&
		    data.image_ivt_offset + data.header_gap < sizeof_bb_header) {
			fprintf(stderr, "barebox header conflicts with IVT\n");
			exit(1);
		}

		if (lseek(outfd, data.header_gap, SEEK_SET) < 0) {
			perror("lseek");
			exit(1);
		}

		xwrite(outfd, buf, header_len);
	}

	if (add_barebox_header) {
		ret = pwrite(outfd, bb_header, sizeof_bb_header, 0);
		if (ret < 0) {
			fprintf(stderr, "pwrite failed: %s\n", strerror(errno));
			exit(1);
		}
		if (data.cpu_type == IMX_CPU_IMX35) {
			ret = pwrite(outfd, bb_header, sizeof_bb_header, header_len);
			if (ret < 0) {
				fprintf(stderr, "pwrite failed: %s\n", strerror(errno));
				exit(1);
			}
		}
	}

	xwrite(outfd, infile, insize);

	/*
	 * The alignment may be required on ARMv7 SoCs like i.MX6/7 for HAB
	 * boot. On newer SoCs like i.MX8MP/N this cause libusb communication
	 * errors while uploading images because these machines request the
	 * exact amount of required bytes and move on afterwards while the host
	 * tool still try to send the whole (padded) file size.
	 */
	if (!cpu_is_mx8m(&data)) {
		/* pad until next 4k boundary */
		now = 4096 - (insize % 4096);
		if (data.csf && now) {
			memset(buf, 0x5a, now);

			xwrite(outfd, buf, now);
		}
	}

	ret = close(outfd);
	if (ret) {
		perror("close");
		exit(1);
	}

	if (data.csf && data.sign_image) {
		ret = hab_sign(&data, data.csf, 0);
		if (ret)
			exit(1);
		if (data.flexspi_csf) {
			ret = hab_sign(&data, data.flexspi_csf, 1);
			if (ret)
				exit(1);
		}
	}

	if (create_usb_image) {
		uint32_t *dcd;

		infile = read_file(data.outfile, &insize);
		if (!infile)
			exit(1);

		dcd = infile + dcd_ptr_offset;
		*dcd = dcd_ptr_content;

		outfd = open(data.outfile, O_WRONLY | O_TRUNC);
		if (outfd < 0) {
			fprintf(stderr, "Cannot open %s: %s\n", data.outfile, strerror(errno));
			exit(1);
		}

		xwrite(outfd, infile, insize);

		close(outfd);
	}

	exit(0);
}
