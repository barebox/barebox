/*
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
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/mtd/nand.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx-nand.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <mach/imx21-regs.h>
#include <mach/imx25-regs.h>
#include <mach/imx27-regs.h>
#include <mach/imx31-regs.h>
#include <mach/imx35-regs.h>

#define BARE_INIT_FUNCTION(name)  \
	__section(.text_bare_init_##name) \
		name

static void __bare_init noinline imx_nandboot_wait_op_done(void __iomem *regs)
{
	u32 r;

	while (1) {
		r = readw(regs + NFC_V1_V2_CONFIG2);
		if (r & NFC_V1_V2_CONFIG2_INT)
			break;
	};

	r &= ~NFC_V1_V2_CONFIG2_INT;

	writew(r, regs + NFC_V1_V2_CONFIG2);
}

/*
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 */
static void __bare_init imx_nandboot_send_cmd(void *regs, u16 cmd)
{
	writew(cmd, regs + NFC_V1_V2_FLASH_CMD);
	writew(NFC_CMD, regs + NFC_V1_V2_CONFIG2);

	imx_nandboot_wait_op_done(regs);
}

/*
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 * @param       islast  True if this is the last address cycle for command
 */
static void __bare_init noinline imx_nandboot_send_addr(void *regs, u16 addr)
{
	writew(addr, regs + NFC_V1_V2_FLASH_ADDR);
	writew(NFC_ADDR, regs + NFC_V1_V2_CONFIG2);

	/* Wait for operation to complete */
	imx_nandboot_wait_op_done(regs);
}

static void __bare_init imx_nandboot_nfc_addr(void *regs, u32 offs, int pagesize_2k)
{
	imx_nandboot_send_addr(regs, offs & 0xff);

	if (pagesize_2k) {
		imx_nandboot_send_addr(regs, offs & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 11) & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 19) & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 27) & 0xff);
		imx_nandboot_send_cmd(regs, NAND_CMD_READSTART);
	} else {
		imx_nandboot_send_addr(regs, (offs >> 9) & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 17) & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 25) & 0xff);
	}
}

static void __bare_init imx_nandboot_send_page(void *regs, int v1,
		unsigned int ops, int pagesize_2k)
{
	int bufs, i;

	if (v1 && pagesize_2k)
		bufs = 4;
	else
		bufs = 1;

	for (i = 0; i < bufs; i++) {
		/* NANDFC buffer 0 is used for page read/write */
		writew(i, regs + NFC_V1_V2_BUF_ADDR);

		writew(ops, regs + NFC_V1_V2_CONFIG2);

		/* Wait for operation to complete */
		imx_nandboot_wait_op_done(regs);
	}
}

static void __bare_init __memcpy32(void *trg, const void *src, int size)
{
	int i;
	unsigned int *t = trg;
	unsigned const int *s = src;

	for (i = 0; i < (size >> 2); i++)
		*t++ = *s++;
}

static noinline void __bare_init imx_nandboot_get_page(void *regs, int v1,
		u32 offs, int pagesize_2k)
{
	imx_nandboot_send_cmd(regs, NAND_CMD_READ0);
	imx_nandboot_nfc_addr(regs, offs, pagesize_2k);
	imx_nandboot_send_page(regs, v1, NFC_OUTPUT, pagesize_2k);
}

void __bare_init imx_nand_load_image(void *dest, int v1, int size, void __iomem *base,
		int pagesize_2k)
{
	u32 tmp, page, block, blocksize, pagesize, badblocks;
	int bbt = 0;
	void *regs, *spare0;

	if (pagesize_2k) {
		pagesize = 2048;
		blocksize = 128 * 1024;
	} else {
		pagesize = 512;
		blocksize = 16 * 1024;
	}

	if (v1) {
		regs = base + 0xe00;
		spare0 = base + 0x800;
	} else {
		regs = base + 0x1e00;
		spare0 = base + 0x1000;
	}

	imx_nandboot_send_cmd(regs, NAND_CMD_RESET);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	writew(0x2, regs + NFC_V1_V2_CONFIG);

	/* Unlock Block Command for given address range */
	writew(0x4, regs + NFC_V1_V2_WRPROT);

	tmp = readw(regs + NFC_V1_V2_CONFIG1);
	tmp |= NFC_V1_V2_CONFIG1_ECC_EN;
	if (!v1)
		/* currently no support for 218 byte OOB with stronger ECC */
		tmp |= NFC_V2_CONFIG1_ECC_MODE_4;
	tmp &= ~(NFC_V1_V2_CONFIG1_SP_EN | NFC_V1_V2_CONFIG1_INT_MSK);
	writew(tmp, regs + NFC_V1_V2_CONFIG1);

	if (!v1) {
		if (pagesize_2k)
			writew(NFC_V2_SPAS_SPARESIZE(64), regs + NFC_V2_SPAS);
		else
			writew(NFC_V2_SPAS_SPARESIZE(16), regs + NFC_V2_SPAS);
	}

	/*
	 * Check if this image has a bad block table embedded. See
	 * imx_bbu_external_nand_register_handler for more information
	 */
	badblocks = *(uint32_t *)(base + ARM_HEAD_SPARE_OFS);
	if (badblocks == IMX_NAND_BBT_MAGIC) {
		bbt = 1;
		badblocks = *(uint32_t *)(base + ARM_HEAD_SPARE_OFS + 4);
	}

	block = page = 0;

	while (1) {
		page = 0;

		imx_nandboot_get_page(regs, v1, block * blocksize +
				page * pagesize, pagesize_2k);

		if (bbt) {
			if (badblocks & (1 << block)) {
				block++;
				continue;
			}
		} else if (pagesize_2k) {
			if ((readw(spare0) & 0xff) != 0xff) {
				block++;
				continue;
			}
		} else {
			if ((readw(spare0 + 4) & 0xff00) != 0xff00) {
				block++;
				continue;
			}
		}

		while (page * pagesize < blocksize) {
			debug("page: %d block: %d dest: %p src "
					"0x%08x\n",
					page, block, dest,
					block * blocksize +
					page * pagesize);
			if (page)
				imx_nandboot_get_page(regs, v1, block * blocksize +
					page * pagesize, pagesize_2k);

			page++;

			__memcpy32(dest, base, pagesize);
			dest += pagesize;
			size -= pagesize;

			if (size <= 0)
				return;
		}
		block++;
	}
}

void BARE_INIT_FUNCTION(imx21_nand_load_image)(void *dest, int size,
                void __iomem *base, int pagesize_2k)
{
        imx_nand_load_image(dest, 1, size, base, pagesize_2k);
}

void BARE_INIT_FUNCTION(imx25_nand_load_image)(void *dest, int size,
                void __iomem *base, int pagesize_2k)
{
        imx_nand_load_image(dest, 0, size, base, pagesize_2k);
}

void BARE_INIT_FUNCTION(imx27_nand_load_image)(void *dest, int size,
                void __iomem *base, int pagesize_2k)
{
        imx_nand_load_image(dest, 1, size, base, pagesize_2k);
}

void BARE_INIT_FUNCTION(imx31_nand_load_image)(void *dest, int size,
                void __iomem *base, int pagesize_2k)
{
        imx_nand_load_image(dest, 1, size, base, pagesize_2k);
}

void BARE_INIT_FUNCTION(imx35_nand_load_image)(void *dest, int size,
                void __iomem *base, int pagesize_2k)
{
        imx_nand_load_image(dest, 0, size, base, pagesize_2k);
}

static inline int imx21_pagesize_2k(void)
{
	if (readl(MX21_SYSCTRL_BASE_ADDR + 0x14) & (1 << 5))
		return 1;
	else
		return 0;
}

static inline int imx25_pagesize_2k(void)
{
	if (readl(MX25_CCM_BASE_ADDR + MX25_CCM_RCSR) & (1 << 8))
		return 1;
	else
		return 0;
}

static inline int imx27_pagesize_2k(void)
{
	if (readl(MX27_SYSCTRL_BASE_ADDR + 0x14) & (1 << 5))
		return 1;
	else
		return 0;
}

static inline int imx31_pagesize_2k(void)
{
	if (readl(MX31_CCM_BASE_ADDR + MX31_CCM_RCSR) & MX31_RCSR_NFMS)
		return 1;
	else
		return 0;
}

static inline int imx35_pagesize_2k(void)
{
	if (readl(MX35_CCM_BASE_ADDR + MX35_CCM_RCSR) & (1 << 8))
		return 1;
	else
		return 0;
}

/*
 * SoC specific entries for booting in external NAND mode. To be called from
 * the board specific entry code. This is safe to call even if not booting from
 * NAND. In this case the booting is continued without loading an image from
 * NAND. This function needs a stack to be set up.
 */

#define DEFINE_EXTERNAL_NAND_ENTRY(soc)					\
									\
void __noreturn BARE_INIT_FUNCTION(imx##soc##_boot_nand_external_cont)  \
			(void *boarddata)				\
{									\
	unsigned long nfc_base = MX##soc##_NFC_BASE_ADDR;		\
	void *sdram = (void *)MX##soc##_CSD0_BASE_ADDR;			\
	uint32_t image_size, r;						\
									\
	image_size = *(uint32_t *)(sdram + 0x2c);			\
									\
	r = get_cr();							\
	r |= CR_I;							\
	set_cr(r);							\
									\
	imx##soc##_nand_load_image(sdram,				\
			image_size,					\
			(void *)nfc_base,				\
			imx##soc##_pagesize_2k());			\
									\
        imx##soc##_barebox_entry(boarddata);				\
}									\
									\
void __noreturn BARE_INIT_FUNCTION(imx##soc##_barebox_boot_nand_external) \
			(void *bd)				\
{									\
	unsigned long nfc_base = MX##soc##_NFC_BASE_ADDR;		\
	unsigned long sdram = MX##soc##_CSD0_BASE_ADDR;			\
	unsigned long boarddata = (unsigned long)bd;			\
	unsigned long __fn;						\
	u32 r;								\
	u32 *src, *trg;							\
	int i;								\
	void __noreturn (*fn)(void *);					\
									\
	r = get_cr();							\
	r &= ~CR_I;							\
	set_cr(r);							\
	/* skip NAND boot if not running from NFC space */		\
	r = get_pc();							\
	if (r < nfc_base || r > nfc_base + 0x800)			\
		imx##soc##_barebox_entry(bd);				\
									\
	src = (unsigned int *)nfc_base;					\
	trg = (unsigned int *)sdram;					\
									\
	/*								\
	 * Copy initial binary portion from NFC SRAM to beginning of	\
	 * SDRAM							\
	 */								\
	for (i = 0; i < 0x800 / sizeof(int); i++)			\
		*trg++ = *src++;					\
									\
	/* The next function we jump to */				\
	__fn = (unsigned long)imx##soc##_boot_nand_external_cont;	\
	/* mask out TEXT_BASE */					\
	__fn &= 0x7ff;							\
	/*								\
	 * and add sdram base instead where we copied the initial	\
	 * binary above							\
	 */								\
	__fn += sdram;							\
									\
	fn = (void *)__fn;						\
									\
	if (boarddata > nfc_base && boarddata < nfc_base + SZ_512K) {	\
		boarddata &= SZ_512K - 1;				\
		boarddata += sdram;					\
	}								\
									\
	fn((void *)boarddata);						\
}

#ifdef BROKEN
DEFINE_EXTERNAL_NAND_ENTRY(21)
#endif
DEFINE_EXTERNAL_NAND_ENTRY(25)
DEFINE_EXTERNAL_NAND_ENTRY(27)
DEFINE_EXTERNAL_NAND_ENTRY(31)
DEFINE_EXTERNAL_NAND_ENTRY(35)
