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

static void __bare_init noinline imx_nandboot_wait_op_done(void *regs)
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

static void __bare_init imx_nandboot_send_page(void *regs,
		unsigned int ops, int pagesize_2k)
{
	int bufs, i;

	if (nfc_is_v1() && pagesize_2k)
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

static int __maybe_unused is_pagesize_2k(void)
{
#ifdef CONFIG_ARCH_IMX21
	if (readl(MX21_SYSCTRL_BASE_ADDR + 0x14) & (1 << 5))
		return 1;
	else
		return 0;
#endif
#if defined(CONFIG_ARCH_IMX25)
	if (readl(MX25_CCM_BASE_ADDR + MX25_CCM_RCSR) & (1 << 8))
		return 1;
	else
		return 0;
#endif
#ifdef CONFIG_ARCH_IMX27
	if (readl(MX27_SYSCTRL_BASE_ADDR + 0x14) & (1 << 5))
		return 1;
	else
		return 0;
#endif
#ifdef CONFIG_ARCH_IMX31
	if (readl(MX31_CCM_BASE_ADDR + MX31_CCM_RCSR) & MX31_RCSR_NFMS)
		return 1;
	else
		return 0;
#endif
#if defined(CONFIG_ARCH_IMX35)
	if (readl(MX35_CCM_BASE_ADDR + MX35_CCM_RCSR) & (1 << 8))
		return 1;
	else
		return 0;
#endif
}

static noinline void __bare_init imx_nandboot_get_page(void *regs,
		u32 offs, int pagesize_2k)
{
	imx_nandboot_send_cmd(regs, NAND_CMD_READ0);
	imx_nandboot_nfc_addr(regs, offs, pagesize_2k);
	imx_nandboot_send_page(regs, NFC_OUTPUT, pagesize_2k);
}

void __bare_init imx_nand_load_image(void *dest, int size)
{
	u32 tmp, page, block, blocksize, pagesize, badblocks;
	int pagesize_2k = 1, bbt = 0;
	void *regs, *base, *spare0;

#if defined(CONFIG_NAND_IMX_BOOT_512)
	pagesize_2k = 0;
#elif defined(CONFIG_NAND_IMX_BOOT_2K)
	pagesize_2k = 1;
#else
	pagesize_2k = is_pagesize_2k();
#endif

	if (pagesize_2k) {
		pagesize = 2048;
		blocksize = 128 * 1024;
	} else {
		pagesize = 512;
		blocksize = 16 * 1024;
	}

#ifdef CONFIG_ARCH_IMX21
	base = (void __iomem *)MX21_NFC_BASE_ADDR;
#endif
#ifdef CONFIG_ARCH_IMX25
	base = (void __iomem *)MX25_NFC_BASE_ADDR;
#endif
#ifdef CONFIG_ARCH_IMX27
	base = (void __iomem *)MX27_NFC_BASE_ADDR;
#endif
#ifdef CONFIG_ARCH_IMX31
	base = (void __iomem *)MX31_NFC_BASE_ADDR;
#endif
#ifdef CONFIG_ARCH_IMX35
	base = (void __iomem *)MX35_NFC_BASE_ADDR;
#endif
	if (nfc_is_v21()) {
		regs = base + 0x1e00;
		spare0 = base + 0x1000;
	} else if (nfc_is_v1()) {
		regs = base + 0xe00;
		spare0 = base + 0x800;
	}

	imx_nandboot_send_cmd(regs, NAND_CMD_RESET);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	writew(0x2, regs + NFC_V1_V2_CONFIG);

	/* Unlock Block Command for given address range */
	writew(0x4, regs + NFC_V1_V2_WRPROT);

	tmp = readw(regs + NFC_V1_V2_CONFIG1);
	tmp |= NFC_V1_V2_CONFIG1_ECC_EN;
	if (nfc_is_v21())
		/* currently no support for 218 byte OOB with stronger ECC */
		tmp |= NFC_V2_CONFIG1_ECC_MODE_4;
	tmp &= ~(NFC_V1_V2_CONFIG1_SP_EN | NFC_V1_V2_CONFIG1_INT_MSK);
	writew(tmp, regs + NFC_V1_V2_CONFIG1);

	if (nfc_is_v21()) {
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

		imx_nandboot_get_page(regs, block * blocksize +
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
				imx_nandboot_get_page(regs, block * blocksize +
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

/*
 * This function assumes the currently running binary has been
 * copied from its current position to an offset. It returns
 * to the calling function - offset.
 * NOTE: The calling function may not return itself since it still
 * works on the old content of the lr register. Only call this
 * from a __noreturn function.
 */
static __bare_init __naked void jump_sdram(unsigned long offset)
{
	__asm__ __volatile__ (
			"sub lr, lr, %0;"
			"mov pc, lr;" : : "r"(offset)
			);
}

/*
 * Load and start barebox from NAND. This function also checks if we are really
 * running inside the NFC address space. If not, barebox is started from the
 * currently running address without loading anything from NAND.
 */
int __bare_init imx_barebox_boot_nand_external(unsigned long nfc_base)
{
	u32 r;
	u32 *src, *trg;
	int i;

	/* skip NAND boot if not running from NFC space */
	r = get_pc();
	if (r < nfc_base || r > nfc_base + 0x800)
		return 0;

	src = (unsigned int *)nfc_base;
	trg = (unsigned int *)ld_var(_text);

	/* Move ourselves out of NFC SRAM */
	for (i = 0; i < 0x800 / sizeof(int); i++)
		*trg++ = *src++;

	return 1;
}

/*
 * SoC specific entries for booting in external NAND mode. To be called from
 * the board specific entry code. This is safe to call even if not booting from
 * NAND. In this case the booting is continued without loading an image from
 * NAND. This function needs a stack to be set up.
 */
#ifdef CONFIG_ARCH_IMX21
void __bare_init __noreturn imx21_barebox_boot_nand_external(void)
{
	unsigned long nfc_base = MX21_NFC_BASE_ADDR;

	if (imx_barebox_boot_nand_external(nfc_base)) {
		jump_sdram(nfc_base - ld_var(_text));
		imx_nand_load_image((void *)ld_var(_text),
				ld_var(barebox_image_size));
	}

	imx21_barebox_entry(0);
}
#endif

#ifdef CONFIG_ARCH_IMX25
void __bare_init __noreturn imx25_barebox_boot_nand_external(void)
{
	unsigned long nfc_base = MX25_NFC_BASE_ADDR;

	if (imx_barebox_boot_nand_external(nfc_base)) {
		jump_sdram(nfc_base - ld_var(_text));
		imx_nand_load_image((void *)ld_var(_text),
				ld_var(_barebox_image_size));
	}

	imx25_barebox_entry(0);
}
#endif

#ifdef CONFIG_ARCH_IMX27
void __bare_init __noreturn imx27_barebox_boot_nand_external(void)
{
	unsigned long nfc_base = MX27_NFC_BASE_ADDR;

	if (imx_barebox_boot_nand_external(nfc_base)) {
		jump_sdram(nfc_base - ld_var(_text));
		imx_nand_load_image((void *)ld_var(_text),
				ld_var(_barebox_image_size));
	}

	imx27_barebox_entry(0);
}
#endif

#ifdef CONFIG_ARCH_IMX31
void __bare_init __noreturn imx31_barebox_boot_nand_external(void)
{
	unsigned long nfc_base = MX31_NFC_BASE_ADDR;

	if (imx_barebox_boot_nand_external(nfc_base)) {
		jump_sdram(nfc_base - ld_var(_text));
		imx_nand_load_image((void *)ld_var(_text),
				ld_var(_barebox_image_size));
	}

	imx31_barebox_entry(0);
}
#endif

#ifdef CONFIG_ARCH_IMX35
void __bare_init __noreturn imx35_barebox_boot_nand_external(void)
{
	unsigned long nfc_base = MX35_NFC_BASE_ADDR;

	if (imx_barebox_boot_nand_external(nfc_base)) {
		jump_sdram(nfc_base - ld_var(_text));
		imx_nand_load_image((void *)ld_var(_text),
				ld_var(_barebox_image_size));
	}

	imx35_barebox_entry(0);
}
#endif

#define CONFIG_NAND_IMX_BOOT_DEBUG
#ifdef CONFIG_NAND_IMX_BOOT_DEBUG
#include <command.h>

static int do_nand_boot_test(int argc, char *argv[])
{
	void *dest;
	int size;

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	dest = (void *)strtoul_suffix(argv[1], NULL, 0);
	size = strtoul_suffix(argv[2], NULL, 0);

	imx_nand_load_image(dest, size);

	return 0;
}

static const __maybe_unused char cmd_nand_boot_test_help[] =
"Usage: nand_boot_test <dest> <size>\n"
"This command loads the booloader from the NAND memory like the reset\n"
"routine does. Its intended for development tests only";

BAREBOX_CMD_START(nand_boot_test)
	.cmd		= do_nand_boot_test,
	.usage		= "load bootloader from NAND",
	BAREBOX_CMD_HELP(cmd_nand_boot_test_help)
BAREBOX_CMD_END
#endif
