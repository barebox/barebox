#include <common.h>
#include <asm/sections.h>
#include <debug_ll.h>
#include <disks.h>
#include <init.h>
#include <filetype.h>
#include <io.h>
#include <asm/unaligned.h>
#include <mach/arria10-pinmux.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-system-manager.h>
#include <mach/arria10-fpga.h>
#include <mach/arria10-xload.h>
#include <mach/generic.h>
#include <linux/sizes.h>

int a10_update_bits(unsigned int reg, unsigned int mask,
		    unsigned int val)
{
	unsigned int tmp, orig;
	int ret = 0;

	orig = readl(ARRIA10_FPGAMGRREGS_ADDR + reg);
	tmp = orig & ~mask;
	tmp |= val & mask;

	if (tmp != orig)
		ret = writel(tmp, ARRIA10_FPGAMGRREGS_ADDR + reg);

	return ret;
}

static uint32_t socfpga_a10_fpga_read_stat(void)
{
	return readl(ARRIA10_FPGAMGRREGS_ADDR + A10_FPGAMGR_IMGCFG_STAT_OFST);
}

static int a10_fpga_wait_for_condone(void)
{
	u32 reg, i;

	for (i = 0; i < 0x1000000 ; i++) {
		reg = socfpga_a10_fpga_read_stat();

		if (reg & A10_FPGAMGR_IMGCFG_STAT_F2S_CONDONE_PIN)
			return 0;

		if ((reg & A10_FPGAMGR_IMGCFG_STAT_F2S_NSTATUS_PIN) == 0)
			return -EIO;
	}

	return -ETIMEDOUT;
}

static void a10_fpga_generate_dclks(uint32_t count)
{
	int32_t timeout;

	/* Clear any existing DONE status. */
	writel(A10_FPGAMGR_DCLKSTAT_DCLKDONE, ARRIA10_FPGAMGRREGS_ADDR +
	       A10_FPGAMGR_DCLKSTAT_OFST);

	/* Issue the DCLK regmap. */
	writel(count, ARRIA10_FPGAMGRREGS_ADDR + A10_FPGAMGR_DCLKCNT_OFST);

	/* wait till the dclkcnt done */
	timeout = 10000000;

	while (!readl(ARRIA10_FPGAMGRREGS_ADDR + A10_FPGAMGR_DCLKSTAT_OFST)) {
		if (timeout-- < 0)
			return;
	}

	/* Clear DONE status. */
	writel(A10_FPGAMGR_DCLKSTAT_DCLKDONE, ARRIA10_FPGAMGRREGS_ADDR +
	       A10_FPGAMGR_DCLKSTAT_OFST);
}

static unsigned int a10_fpga_get_cd_ratio(unsigned int cfg_width,
					  bool encrypt, bool compress)
{
	unsigned int cd_ratio;

	/*
	 * cd ratio is dependent on cfg width and whether the bitstream
	 * is encrypted and/or compressed.
	 *
	 * | width | encr. | compr. | cd ratio | value |
	 * |  16   |   0   |   0    |     1    |   0   |
	 * |  16   |   0   |   1    |     4    |   2   |
	 * |  16   |   1   |   0    |     2    |   1   |
	 * |  16   |   1   |   1    |     4    |   2   |
	 * |  32   |   0   |   0    |     1    |   0   |
	 * |  32   |   0   |   1    |     8    |   3   |
	 * |  32   |   1   |   0    |     4    |   2   |
	 * |  32   |   1   |   1    |     8    |   3   |
	 */
	if (!compress && !encrypt)
		return CDRATIO_x1;

	if (compress)
		cd_ratio = CDRATIO_x4;
	else
		cd_ratio = CDRATIO_x2;

	/* If 32 bit, double the cd ratio by incrementing the field  */
	if (cfg_width == CFGWDTH_32)
		cd_ratio += 1;

	return cd_ratio;
}

static int a10_fpga_set_cdratio(unsigned int cfg_width,
				const void *buf)
{
	unsigned int cd_ratio;
	int encrypt, compress;
	u32 *rbf_data = (u32 *)buf;

	encrypt = (rbf_data[69] >> 2) & 3;
	encrypt = encrypt != 0;

	compress = (rbf_data[229] >> 1) & 1;
	compress = !compress;

	cd_ratio = a10_fpga_get_cd_ratio(cfg_width, encrypt, compress);

	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_02_OFST,
			A10_FPGAMGR_IMGCFG_CTL_02_CDRATIO_MASK,
			cd_ratio << A10_FPGAMGR_IMGCFG_CTL_02_CDRATIO_SHIFT);

	return 0;
}

static int a10_fpga_init(void *buf)
{
	uint32_t stat, mask;
	uint32_t val;
	uint32_t timeout;

	val = CFGWDTH_32 << A10_FPGAMGR_IMGCFG_CTL_02_CFGWIDTH_SHIFT;
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_02_OFST,
			A10_FPGAMGR_IMGCFG_CTL_02_CFGWIDTH, val);

	a10_fpga_set_cdratio(CFGWDTH_32, buf);

	mask = A10_FPGAMGR_IMGCFG_STAT_F2S_NCONFIG_PIN |
		A10_FPGAMGR_IMGCFG_STAT_F2S_NSTATUS_PIN;
	/* Make sure no external devices are interfering */
	timeout = 10000;
	while ((socfpga_a10_fpga_read_stat() & mask) != mask) {
		if (timeout-- < 0)
			return -ETIMEDOUT;
	}

	/* S2F_NCE = 1 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_01_OFST,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_NCE,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_NCE);
	/* S2F_PR_REQUEST = 0 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_01_OFST,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_PR_REQUEST, 0);
	/* EN_CFG_CTRL = 0 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_02_OFST,
			A10_FPGAMGR_IMGCFG_CTL_02_EN_CFG_CTRL, 0);
	/* S2F_NCONFIG = 1 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_00_OFST,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NCONFIG,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NCONFIG);
	/* S2F_NSTATUS_OE = 0 and S2f_CONDONE_OE = 0 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_00_OFST,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NSTATUS_OE |
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_CONDONE_OE,
			   0);
	/* Enable overrides: S2F_NENABLE_CONFIG = 0 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_01_OFST,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_NENABLE_CONFIG, 0);
	/* Enable overrides: S2F_NENABLE_NCONFIG = 0 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_00_OFST,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NENABLE_NCONFIG, 0);
	/* Disable unused overrides: S2F_NENABLE_NSTATUS = 1 and S2F_NENABLE_CONDONE = 1 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_00_OFST,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NENABLE_NSTATUS |
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NENABLE_CONDONE,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NENABLE_NSTATUS |
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NENABLE_CONDONE);
	/* Drive chip select S2F_NCE = 0 */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_01_OFST,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_NCE, 0);

	mask = A10_FPGAMGR_IMGCFG_STAT_F2S_NCONFIG_PIN |
	       A10_FPGAMGR_IMGCFG_STAT_F2S_NSTATUS_PIN;

	timeout = 100000;
	while ((socfpga_a10_fpga_read_stat() & mask) != mask) {
		if (timeout-- < 0)
			return -ETIMEDOUT;
	}

	/* reset the configuration */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_00_OFST,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NCONFIG, 0);

	timeout = 1000000;
	while ((socfpga_a10_fpga_read_stat() &
		       A10_FPGAMGR_IMGCFG_STAT_F2S_NSTATUS_PIN) != 0) {
		if (timeout-- < 0)
			return -ETIMEDOUT;
	}

	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_00_OFST,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NCONFIG,
			A10_FPGAMGR_IMGCFG_CTL_00_S2F_NCONFIG);

	mask = A10_FPGAMGR_IMGCFG_STAT_F2S_NSTATUS_PIN;
	/* wait for nstatus == 1 */
	timeout = 1000000;
	while ((socfpga_a10_fpga_read_stat() & mask) != mask) {
		if (timeout-- < 0) {
			writel(socfpga_a10_fpga_read_stat(), 0xFFD06238);
			return -ETIMEDOUT;
		}
	}

	stat = socfpga_a10_fpga_read_stat();
	if ((stat & A10_FPGAMGR_IMGCFG_STAT_F2S_CONDONE_PIN) != 0)
		return -EINVAL;
	if ((stat & A10_FPGAMGR_IMGCFG_STAT_F2S_CONDONE_OE) == 0)
		return -EINVAL;

	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_02_OFST,
			A10_FPGAMGR_IMGCFG_CTL_02_EN_CFG_CTRL |
			A10_FPGAMGR_IMGCFG_CTL_02_EN_CFG_DATA,
			A10_FPGAMGR_IMGCFG_CTL_02_EN_CFG_DATA |
			A10_FPGAMGR_IMGCFG_CTL_02_EN_CFG_CTRL);

	/* Check fpga_usermode */
	if ((socfpga_a10_fpga_read_stat() & 0x6) == 0x6)
		return -EIO;

	return 0;
}

static int a10_fpga_write(void *buf, size_t count)
{
	const uint32_t *buf32 = buf;
	uint32_t reg;

	/* Stop if FPGA is configured */
	reg = socfpga_a10_fpga_read_stat();

	if (reg & A10_FPGAMGR_IMGCFG_STAT_F2S_CONDONE_PIN)
		return -ENOSPC;

	/* Write out the complete 32-bit chunks */
	while (count >= sizeof(uint32_t)) {
		writel(*buf32, ARRIA10_FPGAMGRDATA_ADDR);
		buf32++;
		count -= sizeof(u32);
	}

	/* Write out remaining non 32-bit chunks */
	if (count) {
		const uint8_t *buf8 = (const uint8_t *)buf32;
		uint32_t word = 0;

		while (count--) {
			word |= *buf8;
			word <<= 8;
			buf8++;
		}

		writel(word, ARRIA10_FPGAMGRDATA_ADDR);
	}

	return 0;
}

static int a10_fpga_write_complete(void)
{
	u32 reg;
	int ret;

	/* Wait for condone */
	ret = a10_fpga_wait_for_condone();

	/* Send some clocks to clear out any errors */
	a10_fpga_generate_dclks(256);

	/* Disable s2f dclk and data */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_02_OFST,
			A10_FPGAMGR_IMGCFG_CTL_02_EN_CFG_CTRL, 0);

	/* Deassert chip select */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_01_OFST,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_NCE,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_NCE);

	/* Disable data, dclk, nce, and pr_request override to CSS */
	a10_update_bits(A10_FPGAMGR_IMGCFG_CTL_01_OFST,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_NENABLE_CONFIG,
			A10_FPGAMGR_IMGCFG_CTL_01_S2F_NENABLE_CONFIG);

	/* Return any errors regarding pr_done or pr_error */
	if (ret)
		return ret;

	/* wait for fpga_usermode */
	a10_wait_for_usermode(0x1000000);

	/* Final check */
	reg = socfpga_a10_fpga_read_stat();

	if (((reg & A10_FPGAMGR_IMGCFG_STAT_F2S_USERMODE) == 0) ||
	    ((reg & A10_FPGAMGR_IMGCFG_STAT_F2S_CONDONE_PIN) == 0) ||
	    ((reg & A10_FPGAMGR_IMGCFG_STAT_F2S_NSTATUS_PIN) == 0))
		return -ETIMEDOUT;

	return 0;
}

static struct partition bitstream;
static struct partition bootloader;

int arria10_prepare_mmc(int barebox_part, int rbf_part)
{
	void *buf = (void *)0xffe00000 + SZ_256K - 128 - SECTOR_SIZE;
	struct partition_entry *table;
	uint32_t i;
	int ret;

	arria10_init_mmc();

	/* read partition table */
	ret = arria10_read_blocks(buf, 0x0, SECTOR_SIZE);
	if (ret)
		return ret;

	table = (struct partition_entry *)&buf[446];

	for (i = 0; i < 4; i++) {
		bootloader.type = get_unaligned_le32(&table[i].type);
		if (bootloader.type ==  0xa2) {
			bootloader.first_sec = get_unaligned_le32(&table[i].partition_start);
			break;
		}
	}

	bitstream.first_sec = get_unaligned_le32(&table[rbf_part].partition_start);

	return 0;
}

int arria10_load_fpga(int offset, int bitstream_size)
{
	void *buf = (void *)0xffe00000 + SZ_256K - 256 - SZ_16K;
	int ret;
	uint32_t count;
	uint32_t size = bitstream_size / SECTOR_SIZE;

	if (offset)
		offset = offset / SECTOR_SIZE;

	count = offset;

	arria10_read_blocks(buf, count + bitstream.first_sec, SZ_16K);

	count += SZ_16K / SECTOR_SIZE;

	ret = a10_fpga_init(buf);
	if (ret)
		hang();

	while (count <= size) {
		ret = a10_fpga_write(buf, SZ_16K);
		if (ret == -ENOSPC)
			break;
		count += SZ_16K / SECTOR_SIZE;
		ret = arria10_read_blocks(buf, count, SZ_16K);
	}

	ret = a10_fpga_write_complete();
	if (ret)
		hang();

	return 0;
}

static int image_size(void)
{
	uint32_t *image_end = (void *)__image_end;
	uint32_t payload_len;
	uint32_t pbl_len;
	uint32_t arria10_header_len;
	uint32_t sizep;
	uint32_t arria10_crc;

	/* arria10 header is 512 byte */
	arria10_header_len = 512;
	/* pbl is appended with 4 byte CRC for boot rom */
	arria10_crc = 4;

	/* The length of the PBL image */
	pbl_len = __image_end - _text;

	sizep = 4;

	/* The length of the payload is appended directly behind the PBL */
	payload_len = *(image_end);

	return pbl_len + arria10_header_len + sizep + arria10_crc + payload_len;
}

void arria10_start_image(int offset)
{
	void *buf = (void *)0x0;
	void *in_buf = (void *)SZ_1M;
	uint32_t start;
	int size = 0;
	int ret;
	void __noreturn (*bb)(void);
	uint32_t pbl_len = __image_end - _text;
	uint32_t *image_end = (void *)__image_end;
	uint32_t arria10_header_len;
	uint32_t sizep;
	uint32_t arria10_crc;

	size = image_size();

	start = bootloader.first_sec + offset / SECTOR_SIZE;

	ret = arria10_read_blocks(buf, start, ALIGN(size, SECTOR_SIZE));
	if (ret) {
		puts_ll("Loading image failed\n");
		hang();
	}

	/* arria10 header is 512 byte */
	arria10_header_len = 512;
	sizep = 4;

	/* copy PBL */
	memcpy(in_buf, buf, pbl_len + sizep + arria10_header_len);

	/* pbl is appended with 4 byte CRC for boot rom */
	arria10_crc = 4;

	/* copy payload, skip the Arria10 CRC */
	memcpy(in_buf + pbl_len + sizep + arria10_header_len,
	       buf + pbl_len + sizep + arria10_header_len + arria10_crc, *(image_end));

	bb = in_buf;

	bb();

	hang();
}
