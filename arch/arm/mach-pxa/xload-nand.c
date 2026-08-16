// SPDX-License-Identifier: GPL-2.0-only
/*
 * Load barebox from NAND in the PXA3xx first stage.
 *
 * The Boot ROM reads the NTIM header from the start of NAND, copies the OBM
 * listed in it into internal SRAM and jumps there. It does not come back, so
 * the OBM has to fetch the next image itself.
 *
 * Everything here runs from internal SRAM before DRAM is usable for anything
 * but the destination, so it uses no global data and no stack beyond what the
 * entry function set up.
 *
 * The NTIM itself is in the first erase block, which the chip guarantees to be
 * good, so it is always where the Boot ROM and this code expect it. Everything
 * behind it is read past bad blocks.
 */

#include <common.h>
#include <linux/sizes.h>
#include <asm/io.h>
#include <mach/pxa/xload.h>

#define NAND_BASE		0x43100000

#define NDCR			0x00
#define NDTR0CS0		0x04
#define NDTR1CS0		0x0c
#define NDSR			0x14
#define NDDB			0x40
#define NDCB0			0x48

#define NDCR_SPARE_EN		(1 << 31)
#define NDCR_ND_RUN		(1 << 28)

#define NDSR_WRCMDREQ		(1 << 0)
#define NDSR_RDDREQ		(1 << 1)

/*
 * Values taken from the vendor OBM. NDCR configures a x8 device with 2K
 * pages; the timings are the ones the previous stage is known to work with.
 *
 * SPARE_EN is ours: it makes the controller hand out the spare area along
 * with the data, which is where the bad block marker is. With ECC_EN clear
 * that is all 64 bytes of it.
 */
#define NDCR_INIT		(0x01045fff | NDCR_SPARE_EN)
#define NDTR0CS0_INIT		0x00161c1c
#define NDTR1CS0_INIT		0x0f3d00f2

/*
 * Read command for a 2K page device: 0x00 followed by 0x30, five address
 * cycles, double byte command.
 */
#define NDCB0_READ_PAGE		0x000d3000

#define NAND_PAGE_SIZE		2048
#define NAND_OOB_SIZE		64
#define NAND_BLOCK_SIZE		SZ_128K

/* A device with more bad blocks than this in front of the image is broken. */
#define NAND_MAX_BAD_BLOCKS	16

static void pxa_nand_init(void __iomem *base)
{
	int i;

	writel(0, base + NDCR);
	writel(NDTR0CS0_INIT, base + NDTR0CS0);
	writel(NDTR1CS0_INIT, base + NDTR1CS0);
	writel(0x00000fff, base + NDSR);
	writel(NDCR_INIT, base + NDCR);

	/* Drain whatever the controller has left in the data buffer. */
	for (i = 0; i < 16; i++)
		readl(base + NDDB);
}

/*
 * Read one page. 'offset' is a physical byte offset into the device and has to
 * be page aligned. @buf takes the data and @oob the spare area; either may be
 * NULL to read past that part of the page without keeping it. The controller
 * hands out both whatever we do with them, so they have to be read either way.
 */
static void pxa_nand_read_page(void __iomem *base, u32 offset, void *buf,
			       void *oob)
{
	u32 *dest;
	u32 addr;
	int i;

	writel(readl(base + NDCR) | NDCR_ND_RUN, base + NDCR);

	while (!(readl(base + NDSR) & NDSR_WRCMDREQ))
		;

	readl(base + NDDB);
	readl(base + NDDB);

	/* column in the low bits, page number from bit 16 upwards */
	addr = (offset & 0x7ff) | ((offset << 5) & 0xffff0000);

	writel(NDCB0_READ_PAGE, base + NDCB0);
	writel(addr, base + NDCB0);
	writel(0, base + NDCB0);

	while (!(readl(base + NDSR) & NDSR_RDDREQ))
		;
	writel(NDSR_RDDREQ, base + NDSR);

	dest = buf;
	for (i = 0; i < NAND_PAGE_SIZE / 4; i++) {
		u32 val = readl(base + NDDB);

		if (dest)
			*dest++ = val;
	}

	/* The spare area follows the data. */
	dest = oob;
	for (i = 0; i < NAND_OOB_SIZE / 4; i++) {
		u32 val = readl(base + NDDB);

		if (dest)
			*dest++ = val;
	}
}

static bool pxa_nand_block_is_bad(void __iomem *base, u32 offset)
{
	/* pxa_nand_read_page() fills this a word at a time */
	u8 oob[NAND_OOB_SIZE] __aligned(4);

	pxa_nand_read_page(base, offset, NULL, oob);

	/*
	 * The bad block marker is the first byte of the spare area on a large
	 * page device.
	 */
	return oob[0] != 0xff;
}

/*
 * Read @size bytes from @offset, which is an offset into the good blocks of
 * the device rather than into the device itself: bad blocks are skipped and do
 * not count towards it. That is the same thing mtd_peb_write_file() does when
 * the update handler writes the image, so the two agree on where an image is
 * without either of them having to record where the bad blocks are.
 *
 * Returns false if the image could not be read.
 */
static bool pxa_nand_read(void __iomem *base, u32 offset, void *buf, u32 size)
{
	unsigned int skipped = 0;
	u32 phys = 0, log = 0;

	while (size) {
		/*
		 * Only the first page of a block carries the marker, so this
		 * is the one point where a block can be rejected.
		 */
		if (!(phys & (NAND_BLOCK_SIZE - 1)) &&
		    pxa_nand_block_is_bad(base, phys)) {
			if (++skipped > NAND_MAX_BAD_BLOCKS)
				return false;
			phys += NAND_BLOCK_SIZE;
			continue;
		}

		/* Not at the image yet, so there is nothing to keep. */
		if (log < offset) {
			phys += NAND_PAGE_SIZE;
			log += NAND_PAGE_SIZE;
			continue;
		}

		pxa_nand_read_page(base, phys, buf, NULL);
		buf += NAND_PAGE_SIZE;
		phys += NAND_PAGE_SIZE;
		log += NAND_PAGE_SIZE;
		size -= min_t(u32, size, NAND_PAGE_SIZE);
	}

	return true;
}

/**
 * pxa_nand_load_image - load the next image out of NAND
 * @image_id:	NTIM id of the image to load, eg NTIM_ID_BOOT
 *
 * Reads the NTIM header from the start of the device, looks up the entry for
 * @image_id and copies that image to the load address the header gives. The
 * flash offset in that entry counts good blocks only; see pxa_nand_read().
 *
 * The header is read from flash rather than taken from the address the Boot
 * ROM loaded it to, so that this does not depend on the ROM's scratch area
 * still being intact.
 *
 * What this expects of the flash. None of it is discovered: the register
 * values at the top of this file are one board's, recovered from its vendor
 * OBM, and the geometry is hardcoded to match them, so a device that differs
 * in any of these needs more than new timings.
 *
 *  - 2048 byte pages. That is NDCR PAGE_SZ, the five address cycles and the
 *    double byte command in NDCB0_READ_PAGE, and the 11 column bits in the
 *    address. A 512 byte page device needs all four changed.
 *
 *  - An 8 bit bus, ie NDCR DWIDTH_M and DWIDTH_C clear.
 *
 *  - 64 pages to an erase block, ie NDCR PG_PER_BLK set. NAND_BLOCK_SIZE has
 *    to say the same thing and nothing checks that it does.
 *
 *  - 64 bytes of spare area, which is what the controller hands out for a 2K
 *    page with SPARE_EN set and ECC_EN clear.
 *
 *  - The bad block marker in the first byte of the spare area of a block's
 *    first page. Parts that mark the second or the last page instead are not
 *    noticed.
 *
 * Return: the load address of the image, or NULL if it could not be loaded
 */
void *pxa_nand_load_image(u32 image_id)
{
	void __iomem *base = IOMEM(NAND_BASE);
	u8 page[NAND_PAGE_SIZE] __aligned(4);
	struct ntim_header *hdr;
	struct ntim_image *img;
	u32 i, num_images;

	pxa_nand_init(base);
	pxa_nand_read_page(base, 0, page, NULL);

	hdr = (void *)page;
	if (hdr->identifier != NTIM_ID_TIMH ||
	    hdr->oem_unique_id != NTIM_OEM_UNIQUE_ID)
		return NULL;

	num_images = hdr->num_images;
	if (!num_images || num_images > NTIM_MAX_IMAGES)
		return NULL;

	img = (void *)(hdr + 1);

	for (i = 0; i < num_images; i++) {
		if (img[i].image_id != image_id)
			continue;

		if (!pxa_nand_read(base, img[i].flash_entry_addr,
				   (void *)img[i].load_addr, img[i].image_size))
			return NULL;

		return (void *)img[i].load_addr;
	}

	return NULL;
}
