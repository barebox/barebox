#include <common.h>
#include <init.h>
#include <asm/io.h>
#include <asm/mmu.h>

#define CACHE_LINE_SIZE		32

static void __iomem *l2x0_base;

#define L2X0_CACHE_ID			0x000
#define L2X0_CACHE_TYPE			0x004
#define L2X0_CTRL			0x100
#define L2X0_AUX_CTRL			0x104
#define L2X0_TAG_LATENCY_CTRL		0x108
#define L2X0_DATA_LATENCY_CTRL		0x10C
#define L2X0_EVENT_CNT_CTRL		0x200
#define L2X0_EVENT_CNT1_CFG		0x204
#define L2X0_EVENT_CNT0_CFG		0x208
#define L2X0_EVENT_CNT1_VAL		0x20C
#define L2X0_EVENT_CNT0_VAL		0x210
#define L2X0_INTR_MASK			0x214
#define L2X0_MASKED_INTR_STAT		0x218
#define L2X0_RAW_INTR_STAT		0x21C
#define L2X0_INTR_CLEAR			0x220
#define L2X0_CACHE_SYNC			0x730
#define L2X0_INV_LINE_PA		0x770
#define L2X0_INV_WAY			0x77C
#define L2X0_CLEAN_LINE_PA		0x7B0
#define L2X0_CLEAN_LINE_IDX		0x7B8
#define L2X0_CLEAN_WAY			0x7BC
#define L2X0_CLEAN_INV_LINE_PA		0x7F0
#define L2X0_CLEAN_INV_LINE_IDX		0x7F8
#define L2X0_CLEAN_INV_WAY		0x7FC
#define L2X0_LOCKDOWN_WAY_D		0x900
#define L2X0_LOCKDOWN_WAY_I		0x904
#define L2X0_TEST_OPERATION		0xF00
#define L2X0_LINE_DATA			0xF10
#define L2X0_LINE_TAG			0xF30
#define L2X0_DEBUG_CTRL			0xF40

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
	writel(0xff, l2x0_base + L2X0_INV_WAY);
	cache_wait(l2x0_base + L2X0_INV_WAY, 0xff);
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

static void l2x0_disable(void)
{
	writel(0xff, l2x0_base + L2X0_CLEAN_INV_WAY);
	while (readl(l2x0_base + L2X0_CLEAN_INV_WAY));
	writel(0, l2x0_base + L2X0_CTRL);
}

void __init l2x0_init(void __iomem *base, __u32 aux_val, __u32 aux_mask)
{
	__u32 aux;

	l2x0_base = base;

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
}

