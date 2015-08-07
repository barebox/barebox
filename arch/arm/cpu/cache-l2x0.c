#define pr_fmt(fmt) "l2x0: " fmt

#include <common.h>
#include <init.h>
#include <io.h>
#include <asm/mmu.h>
#include <asm/cache-l2x0.h>

#define CACHE_LINE_SIZE		32

static void __iomem *l2x0_base;
static uint32_t l2x0_way_mask; /* Bitmask of active ways */

static inline void cache_wait(void __iomem *reg, unsigned long mask)
{
	/* wait for the operation to complete */
	while (readl(reg) & mask)
		;
}

static inline void cache_sync(void)
{
	void __iomem *base = l2x0_base;
	writel(0, base + L2X0_CACHE_SYNC);
	cache_wait(base + L2X0_CACHE_SYNC, 1);
}

static inline void l2x0_clean_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel(addr, base + L2X0_CLEAN_LINE_PA);
}

static inline void l2x0_inv_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel(addr, base + L2X0_INV_LINE_PA);
}

static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;

	/* Clean by PA followed by Invalidate by PA */
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel(addr, base + L2X0_CLEAN_LINE_PA);
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel(addr, base + L2X0_INV_LINE_PA);
}

static inline void l2x0_inv_all(void)
{
	/* invalidate all ways */
	writel(l2x0_way_mask, l2x0_base + L2X0_INV_WAY);
	cache_wait(l2x0_base + L2X0_INV_WAY, l2x0_way_mask);
	cache_sync();
}

static void l2x0_inv_range(unsigned long start, unsigned long end)
{
	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		l2x0_flush_line(start);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		l2x0_flush_line(end);
	}

	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_inv_line(start);
			start += CACHE_LINE_SIZE;
		}
	}
	cache_wait(l2x0_base + L2X0_INV_LINE_PA, 1);
	cache_sync();
}

static void l2x0_clean_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;

	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_clean_line(start);
			start += CACHE_LINE_SIZE;
		}
	}
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	cache_sync();
}

static void l2x0_flush_range(unsigned long start, unsigned long end)
{
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_flush_line(start);
			start += CACHE_LINE_SIZE;
		}
	}
	cache_wait(l2x0_base + L2X0_CLEAN_INV_LINE_PA, 1);
	cache_sync();
}

static void l2x0_flush_all(void)
{
	writel(l2x0_way_mask, l2x0_base + L2X0_CLEAN_INV_WAY);
	cache_wait(l2x0_base + L2X0_CLEAN_INV_WAY, l2x0_way_mask);
	cache_sync();
}

static void l2x0_disable(void)
{
	writel(0xff, l2x0_base + L2X0_CLEAN_INV_WAY);
	while (readl(l2x0_base + L2X0_CLEAN_INV_WAY));
	writel(0, l2x0_base + L2X0_CTRL);
}

void __init l2x0_init(void __iomem *base, __u32 aux_val, __u32 aux_mask)
{
	__u32 aux;
	__u32 cache_id;
	int ways;
	const char *type;

	l2x0_base = base;

	cache_id = readl(l2x0_base + L2X0_CACHE_ID);
	aux = readl(l2x0_base + L2X0_AUX_CTRL);

	/* Determine the number of ways */
	switch (cache_id & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L310:
		if (aux & (1 << 16))
			ways = 16;
		else
			ways = 8;
		type = "L310";
		break;
	case L2X0_CACHE_ID_PART_L210:
		ways = (aux >> 13) & 0xf;
		type = "L210";
		break;
	default:
		/* Assume unknown chips have 8 ways */
		ways = 8;
		type = "L2x0 series";
		break;
	}

       l2x0_way_mask = (1 << ways) - 1;

	/*
	 * Check if l2x0 controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl(l2x0_base + L2X0_CTRL) & 1)) {

		/* l2x0 controller is disabled */

		aux = readl(l2x0_base + L2X0_AUX_CTRL);
		aux &= aux_mask;
		aux |= aux_val;
		writel(aux, l2x0_base + L2X0_AUX_CTRL);

		l2x0_inv_all();

		/* enable L2X0 */
		writel(1, l2x0_base + L2X0_CTRL);
	}

	outer_cache.inv_range = l2x0_inv_range;
	outer_cache.clean_range = l2x0_clean_range;
	outer_cache.flush_range = l2x0_flush_range;
	outer_cache.disable = l2x0_disable;
	outer_cache.flush_all = l2x0_flush_all;

	pr_debug("%s cache controller enabled\n", type);
	pr_debug("l2x0: %d ways, CACHE_ID 0x%08x, AUX_CTRL 0x%08x\n",
			ways, cache_id, aux);
}
