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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/mtd/nand.h>
#include <mach/imx-nand.h>
#include <mach/generic.h>
#include <mach/imx-regs.h>

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
	if (readl(IMX_SYSTEM_CTL_BASE + 0x14) & (1 << 5))
		return 1;
	else
		return 0;
#endif
#ifdef CONFIG_ARCH_IMX27
	if (readl(IMX_SYSTEM_CTL_BASE + 0x14) & (1 << 5))
		return 1;
	else
		return 0;
#endif
#ifdef CONFIG_ARCH_IMX31
	if (readl(IMX_CCM_BASE + CCM_RCSR) & RCSR_NFMS)
		return 1;
	else
		return 0;
#endif
#if defined(CONFIG_ARCH_IMX35) || defined(CONFIG_ARCH_IMX25)
	if (readl(IMX_CCM_BASE + CCM_RCSR) & (1 << 8))
		return 1;
	else
		return 0;
#endif
}

void __bare_init imx_nand_load_image(void *dest, int size)
{
	u32 tmp, page, block, blocksize, pagesize;
	int pagesize_2k = 1;
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

	base = (void __iomem *)IMX_NFC_BASE;
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

	block = page = 0;

	while (1) {
		page = 0;
		while (page * pagesize < blocksize) {
			debug("page: %d block: %d dest: %p src "
					"0x%08x\n",
					page, block, dest,
					block * blocksize +
					page * pagesize);

			imx_nandboot_send_cmd(regs, NAND_CMD_READ0);
			imx_nandboot_nfc_addr(regs, block * blocksize +
					page * pagesize, pagesize_2k);
			imx_nandboot_send_page(regs, NFC_OUTPUT, pagesize_2k);
			page++;

			if (pagesize_2k) {
				if ((readw(spare0) & 0xff) != 0xff)
					continue;
			} else {
				if ((readw(spare0 + 4) & 0xff00) != 0xff00)
					continue;
			}

			__memcpy32(dest, base, pagesize);
			dest += pagesize;
			size -= pagesize;

			if (size <= 0)
				return;
		}
		block++;
	}
}

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
