/*
 * Address map functions for Marvell EBU SoCs (Kirkwood, Armada
 * 370/XP, Dove, Orion5x and MV78xx0)
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * based on mbus driver from Linux
 *   (C) Copyright 2008 Marvell Semiconductor
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * The Marvell EBU SoCs have a configurable physical address space:
 * the physical address at which certain devices (PCIe, NOR, NAND,
 * etc.) sit can be configured. The configuration takes place through
 * two sets of registers:
 *
 * - One to configure the access of the CPU to the devices. Depending
 *   on the families, there are between 8 and 20 configurable windows,
 *   each can be use to create a physical memory window that maps to a
 *   specific device. Devices are identified by a tuple (target,
 *   attribute).
 *
 * - One to configure the access to the CPU to the SDRAM. There are
 *   either 2 (for Dove) or 4 (for other families) windows to map the
 *   SDRAM into the physical address space.
 *
 * This driver:
 *
 * - Reads out the SDRAM address decoding windows at initialization
 *   time, and fills the mbus_dram_info structure with these
 *   informations. The exported function mv_mbus_dram_info() allow
 *   device drivers to get those informations related to the SDRAM
 *   address decoding windows. This is because devices also have their
 *   own windows (configured through registers that are part of each
 *   device register space), and therefore the drivers for Marvell
 *   devices have to configure those device -> SDRAM windows to ensure
 *   that DMA works properly.
 *
 * - Provides an API for platform code or device drivers to
 *   dynamically add or remove address decoding windows for the CPU ->
 *   device accesses. This API is mvebu_mbus_add_window_by_id(),
 *   mvebu_mbus_add_window_remap_by_id() and
 *   mvebu_mbus_del_window().
 *
 * - Provides a debugfs interface in /sys/kernel/debug/mvebu-mbus/ to
 *   see the list of CPU -> SDRAM windows and their configuration
 *   (file 'sdram') and the list of CPU -> devices windows and their
 *   configuration (file 'devices').
 */

#define pr_fmt(fmt)	"mvebu-mbus: " fmt

#include <common.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <of.h>
#include <of_address.h>
#include <linux/mbus.h>

/* DDR target is the same on all platforms */
#define TARGET_DDR		0

/* CPU Address Decode Windows registers */
#define WIN_CTRL_OFF		0x0000
#define   WIN_CTRL_ENABLE       BIT(0)
#define   WIN_CTRL_TGT_MASK     0xf0
#define   WIN_CTRL_TGT_SHIFT    4
#define   WIN_CTRL_ATTR_MASK    0xff00
#define   WIN_CTRL_ATTR_SHIFT   8
#define   WIN_CTRL_SIZE_MASK    0xffff0000
#define   WIN_CTRL_SIZE_SHIFT   16
#define WIN_BASE_OFF		0x0004
#define   WIN_BASE_LOW          0xffff0000
#define   WIN_BASE_HIGH         0xf
#define WIN_REMAP_LO_OFF	0x0008
#define   WIN_REMAP_LOW         0xffff0000
#define WIN_REMAP_HI_OFF	0x000c

#define ATTR_HW_COHERENCY	(0x1 << 4)

#define DDR_BASE_CS_OFF(n)	(0x0000 + ((n) << 3))
#define  DDR_BASE_CS_HIGH_MASK  0xf
#define  DDR_BASE_CS_LOW_MASK   0xff000000
#define DDR_SIZE_CS_OFF(n)	(0x0004 + ((n) << 3))
#define  DDR_SIZE_ENABLED       BIT(0)
#define  DDR_SIZE_CS_MASK       0x1c
#define  DDR_SIZE_CS_SHIFT      2
#define  DDR_SIZE_MASK          0xff000000

#define DOVE_DDR_BASE_CS_OFF(n) ((n) << 4)

struct mvebu_mbus_state;

struct mvebu_mbus_soc_data {
	unsigned int num_wins;
	unsigned int num_remappable_wins;
	unsigned int (*win_cfg_offset)(const int win);
	void (*setup_cpu_target)(struct mvebu_mbus_state *s);
};

struct mvebu_mbus_state {
	struct device_node *np;
	void __iomem *mbuswins_base;
	void __iomem *sdramwins_base;
	struct dentry *debugfs_root;
	struct dentry *debugfs_sdram;
	struct dentry *debugfs_devs;
	struct resource pcie_mem_aperture;
	struct resource pcie_io_aperture;
	const struct mvebu_mbus_soc_data *soc;
};

static struct mvebu_mbus_state mbus_state;
static struct mbus_dram_target_info mbus_dram_info;

/*
 * Functions to manipulate the address decoding windows
 */

static void mvebu_mbus_read_window(struct mvebu_mbus_state *mbus,
				   int win, int *enabled, u64 *base,
				   u32 *size, u8 *target, u8 *attr,
				   u64 *remap)
{
	void __iomem *addr = mbus->mbuswins_base +
		mbus->soc->win_cfg_offset(win);
	u32 basereg = readl(addr + WIN_BASE_OFF);
	u32 ctrlreg = readl(addr + WIN_CTRL_OFF);

	if (!(ctrlreg & WIN_CTRL_ENABLE)) {
		*enabled = 0;
		return;
	}

	*enabled = 1;
	*base = ((u64)basereg & WIN_BASE_HIGH) << 32;
	*base |= (basereg & WIN_BASE_LOW);
	*size = (ctrlreg | ~WIN_CTRL_SIZE_MASK) + 1;

	if (target)
		*target = (ctrlreg & WIN_CTRL_TGT_MASK) >> WIN_CTRL_TGT_SHIFT;

	if (attr)
		*attr = (ctrlreg & WIN_CTRL_ATTR_MASK) >> WIN_CTRL_ATTR_SHIFT;

	if (remap) {
		if (win < mbus->soc->num_remappable_wins) {
			u32 remap_low = readl(addr + WIN_REMAP_LO_OFF);
			u32 remap_hi  = readl(addr + WIN_REMAP_HI_OFF);
			*remap = ((u64)remap_hi << 32) | remap_low;
		} else
			*remap = 0;
	}
}

static void mvebu_mbus_disable_window(struct mvebu_mbus_state *mbus,
				      int win)
{
	void __iomem *addr;

	addr = mbus->mbuswins_base + mbus->soc->win_cfg_offset(win);

	writel(0, addr + WIN_BASE_OFF);
	writel(0, addr + WIN_CTRL_OFF);
	if (win < mbus->soc->num_remappable_wins) {
		writel(0, addr + WIN_REMAP_LO_OFF);
		writel(0, addr + WIN_REMAP_HI_OFF);
	}
}

/* Checks whether the given window number is available */
static int mvebu_mbus_window_is_free(struct mvebu_mbus_state *mbus,
				     const int win)
{
	void __iomem *addr = mbus->mbuswins_base +
		mbus->soc->win_cfg_offset(win);
	u32 ctrl = readl(addr + WIN_CTRL_OFF);
	return !(ctrl & WIN_CTRL_ENABLE);
}

/*
 * Checks whether the given (base, base+size) area doesn't overlap an
 * existing region
 */
static int mvebu_mbus_window_conflicts(struct mvebu_mbus_state *mbus,
				       phys_addr_t base, size_t size,
				       u8 target, u8 attr)
{
	u64 end = (u64)base + size - 1;
	int win;

	for (win = 0; win < mbus->soc->num_wins; win++) {
		u64 wbase, wend;
		u32 wsize;
		u8 wtarget, wattr;
		int enabled;

		mvebu_mbus_read_window(mbus, win,
				       &enabled, &wbase, &wsize,
				       &wtarget, &wattr, NULL);

		if (!enabled)
			continue;

		wend = wbase + wsize - 1;

		/*
		 * Check if the current window overlaps with the
		 * proposed physical range
		 */
		if ((u64)base < wend && end > wbase)
			return 0;

		/*
		 * Check if target/attribute conflicts
		 */
		if (target == wtarget && attr == wattr)
			return 0;
	}

	return 1;
}

static int mvebu_mbus_find_window(struct mvebu_mbus_state *mbus,
				  phys_addr_t base, size_t size)
{
	int win;

	for (win = 0; win < mbus->soc->num_wins; win++) {
		u64 wbase;
		u32 wsize;
		int enabled;

		mvebu_mbus_read_window(mbus, win,
				       &enabled, &wbase, &wsize,
				       NULL, NULL, NULL);

		if (!enabled)
			continue;

		if (base == wbase && size == wsize)
			return win;
	}

	return -ENODEV;
}

static int mvebu_mbus_setup_window(struct mvebu_mbus_state *mbus,
				   int win, phys_addr_t base, size_t size,
				   phys_addr_t remap, u8 target,
				   u8 attr)
{
	void __iomem *addr = mbus->mbuswins_base +
		mbus->soc->win_cfg_offset(win);
	u32 ctrl, remap_addr;

	ctrl = ((size - 1) & WIN_CTRL_SIZE_MASK) |
		(attr << WIN_CTRL_ATTR_SHIFT)    |
		(target << WIN_CTRL_TGT_SHIFT)   |
		WIN_CTRL_ENABLE;

	writel(base & WIN_BASE_LOW, addr + WIN_BASE_OFF);
	writel(ctrl, addr + WIN_CTRL_OFF);
	if (win < mbus->soc->num_remappable_wins) {
		if (remap == MVEBU_MBUS_NO_REMAP)
			remap_addr = base;
		else
			remap_addr = remap;
		writel(remap_addr & WIN_REMAP_LOW, addr + WIN_REMAP_LO_OFF);
		writel(0, addr + WIN_REMAP_HI_OFF);
	}

	return 0;
}

static int mvebu_mbus_alloc_window(struct mvebu_mbus_state *mbus,
				   phys_addr_t base, size_t size,
				   phys_addr_t remap, u8 target,
				   u8 attr)
{
	int win;

	if (remap == MVEBU_MBUS_NO_REMAP) {
		for (win = mbus->soc->num_remappable_wins;
		     win < mbus->soc->num_wins; win++)
			if (mvebu_mbus_window_is_free(mbus, win))
				return mvebu_mbus_setup_window(mbus, win, base,
							       size, remap,
							       target, attr);
	}


	for (win = 0; win < mbus->soc->num_wins; win++)
		if (mvebu_mbus_window_is_free(mbus, win))
			return mvebu_mbus_setup_window(mbus, win, base, size,
						       remap, target, attr);

	return -ENOMEM;
}

/*
 * SoC-specific functions and definitions
 */

static unsigned int armada_370_xp_mbus_win_offset(int win)
{
	/* The register layout is a bit annoying and the below code
	 * tries to cope with it.
	 * - At offset 0x0, there are the registers for the first 8
	 *   windows, with 4 registers of 32 bits per window (ctrl,
	 *   base, remap low, remap high)
	 * - Then at offset 0x80, there is a hole of 0x10 bytes for
	 *   the internal registers base address and internal units
	 *   sync barrier register.
	 * - Then at offset 0x90, there the registers for 12
	 *   windows, with only 2 registers of 32 bits per window
	 *   (ctrl, base).
	 */
	if (win < 8)
		return win << 4;
	else
		return 0x90 + ((win - 8) << 3);
}

static unsigned int orion5x_mbus_win_offset(int win)
{
	return win << 4;
}

static unsigned int mv78xx0_mbus_win_offset(int win)
{
	if (win < 8)
		return win << 4;
	else
		return 0x900 + ((win - 8) << 4);
}

static void mvebu_mbus_default_setup_cpu_target(struct mvebu_mbus_state *mbus)
{
	int i;
	int cs;

	mbus_dram_info.mbus_dram_target_id = TARGET_DDR;

	for (i = 0, cs = 0; i < 4; i++) {
		u32 base = readl(mbus->sdramwins_base + DDR_BASE_CS_OFF(i));
		u32 size = readl(mbus->sdramwins_base + DDR_SIZE_CS_OFF(i));

		/*
		 * We only take care of entries for which the chip
		 * select is enabled, and that don't have high base
		 * address bits set (devices can only access the first
		 * 32 bits of the memory).
		 */
		if ((size & DDR_SIZE_ENABLED) &&
		    !(base & DDR_BASE_CS_HIGH_MASK)) {
			struct mbus_dram_window *w;

			w = &mbus_dram_info.cs[cs++];
			w->cs_index = i;
			w->mbus_attr = 0xf & ~(1 << i);
			w->base = base & DDR_BASE_CS_LOW_MASK;
			w->size = (size | ~DDR_SIZE_MASK) + 1;
		}
	}
	mbus_dram_info.num_cs = cs;
}

static void mvebu_mbus_dove_setup_cpu_target(struct mvebu_mbus_state *mbus)
{
	int i;
	int cs;

	mbus_dram_info.mbus_dram_target_id = TARGET_DDR;

	for (i = 0, cs = 0; i < 2; i++) {
		u32 map = readl(mbus->sdramwins_base + DOVE_DDR_BASE_CS_OFF(i));

		/*
		 * Chip select enabled?
		 */
		if (map & 1) {
			struct mbus_dram_window *w;

			w = &mbus_dram_info.cs[cs++];
			w->cs_index = i;
			w->mbus_attr = 0; /* CS address decoding done inside */
					  /* the DDR controller, no need to  */
					  /* provide attributes */
			w->base = map & 0xff800000;
			w->size = 0x100000 << (((map & 0x000f0000) >> 16) - 4);
		}
	}

	mbus_dram_info.num_cs = cs;
}

static const struct mvebu_mbus_soc_data
armada_370_xp_mbus_data __maybe_unused = {
	.num_wins            = 20,
	.num_remappable_wins = 8,
	.win_cfg_offset      = armada_370_xp_mbus_win_offset,
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
};

static const struct mvebu_mbus_soc_data
dove_mbus_data __maybe_unused = {
	.num_wins            = 8,
	.num_remappable_wins = 4,
	.win_cfg_offset      = orion5x_mbus_win_offset,
	.setup_cpu_target    = mvebu_mbus_dove_setup_cpu_target,
};

static const struct mvebu_mbus_soc_data
kirkwood_mbus_data __maybe_unused = {
	.num_wins            = 8,
	.num_remappable_wins = 4,
	.win_cfg_offset      = orion5x_mbus_win_offset,
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
};

/*
 * Some variants of Orion5x have 4 remappable windows, some other have
 * only two of them.
 */
static const struct mvebu_mbus_soc_data
orion5x_4win_mbus_data __maybe_unused = {
	.num_wins            = 8,
	.num_remappable_wins = 4,
	.win_cfg_offset      = orion5x_mbus_win_offset,
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
};

static const struct mvebu_mbus_soc_data
orion5x_2win_mbus_data__maybe_unused = {
	.num_wins            = 8,
	.num_remappable_wins = 2,
	.win_cfg_offset      = orion5x_mbus_win_offset,
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
};

static const struct mvebu_mbus_soc_data
mv78xx0_mbus_data __maybe_unused = {
	.num_wins            = 14,
	.num_remappable_wins = 8,
	.win_cfg_offset      = mv78xx0_mbus_win_offset,
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
};

static struct of_device_id mvebu_mbus_dt_ids[] = {
#if defined(CONFIG_ARCH_ARMADA_370) || defined(CONFIG_ARCH_ARMADA_XP)
	{ .compatible = "marvell,armada370-mbus",
	  .data = &armada_370_xp_mbus_data, },
	{ .compatible = "marvell,armadaxp-mbus",
	  .data = &armada_370_xp_mbus_data, },
#endif
#if defined(CONFIG_ARCH_DOVE)
	{ .compatible = "marvell,dove-mbus",
	  .data = &dove_mbus_data, },
#endif
#if defined(CONFIG_ARCH_KIRKWOOD)
	{ .compatible = "marvell,kirkwood-mbus",
	  .data = &kirkwood_mbus_data, },
#endif
#if defined(CONFIG_ARCH_ORION5X)
	{ .compatible = "marvell,orion5x-88f5281-mbus",
	  .data = &orion5x_4win_mbus_data, },
	{ .compatible = "marvell,orion5x-88f5182-mbus",
	  .data = &orion5x_2win_mbus_data, },
	{ .compatible = "marvell,orion5x-88f5181-mbus",
	  .data = &orion5x_2win_mbus_data, },
	{ .compatible = "marvell,orion5x-88f6183-mbus",
	  .data = &orion5x_4win_mbus_data, },
#endif
#if defined(CONFIG_ARCH_MV78XX0)
	{ .compatible = "marvell,mv78xx0-mbus",
	  .data = &mv78xx0_mbus_data, },
#endif
	{ },
};

/*
 * Public API of the driver
 */
const struct mbus_dram_target_info *mvebu_mbus_dram_info(void)
{
	return &mbus_dram_info;
}

int mvebu_mbus_add_window_remap_by_id(unsigned int target,
				      unsigned int attribute,
				      phys_addr_t base, size_t size,
				      phys_addr_t remap)
{
	struct mvebu_mbus_state *s = &mbus_state;

	if (!mvebu_mbus_window_conflicts(s, base, size, target, attribute)) {
		pr_err("cannot add window '%x:%x', conflicts with another window\n",
		       target, attribute);
		return -EINVAL;
	}

	return mvebu_mbus_alloc_window(s, base, size, remap, target, attribute);
}

int mvebu_mbus_add_window_by_id(unsigned int target, unsigned int attribute,
				phys_addr_t base, size_t size)
{
	return mvebu_mbus_add_window_remap_by_id(target, attribute, base,
						 size, MVEBU_MBUS_NO_REMAP);
}

int mvebu_mbus_del_window(phys_addr_t base, size_t size)
{
	int win;

	win = mvebu_mbus_find_window(&mbus_state, base, size);
	if (win < 0)
		return win;

	mvebu_mbus_disable_window(&mbus_state, win);
	return 0;
}

void mvebu_mbus_get_pcie_mem_aperture(struct resource *res)
{
	if (!res)
		return;
	*res = mbus_state.pcie_mem_aperture;
}

void mvebu_mbus_get_pcie_io_aperture(struct resource *res)
{
	if (!res)
		return;
	*res = mbus_state.pcie_io_aperture;
}

/*
 * The window IDs in the ranges DT property have the following format:
 *  - bits 28 to 31: MBus custom field
 *  - bits 24 to 27: window target ID
 *  - bits 16 to 23: window attribute ID
 *  - bits  0 to 15: unused
 */
#define CUSTOM(id) (((id) & 0xF0000000) >> 28)
#define TARGET(id) (((id) & 0x0F000000) >> 24)
#define ATTR(id)   (((id) & 0x00FF0000) >> 16)

static int mbus_dt_setup_win(struct mvebu_mbus_state *mbus,
			     u32 base, u32 size, u8 target, u8 attr)
{
	if (!mvebu_mbus_window_conflicts(mbus, base, size, target, attr)) {
		pr_err("cannot add window '%04x:%04x', conflicts with another window\n",
		       target, attr);
		return -EBUSY;
	}

	if (mvebu_mbus_alloc_window(mbus, base, size, MVEBU_MBUS_NO_REMAP,
				    target, attr)) {
		pr_err("cannot add window '%04x:%04x', too many windows\n",
		       target, attr);
		return -ENOMEM;
	}
	return 0;
}

static int mbus_parse_ranges(struct mvebu_mbus_state *mbus, int *addr_cells,
			     int *c_addr_cells, int *c_size_cells,
			     int *cell_count, const __be32 **ranges_start,
			     const __be32 **ranges_end)
{
	struct device_node *node = mbus->np;
	const __be32 *prop;
	int ranges_len, tuple_len;

	/* Allow a node with no 'ranges' property */
	*ranges_start = of_get_property(node, "ranges", &ranges_len);
	if (*ranges_start == NULL) {
		*addr_cells = *c_addr_cells = *c_size_cells = *cell_count = 0;
		*ranges_start = *ranges_end = NULL;
		return 0;
	}
	*ranges_end = *ranges_start + ranges_len / sizeof(__be32);

	*addr_cells = of_n_addr_cells(node);

	prop = of_get_property(node, "#address-cells", NULL);
	*c_addr_cells = be32_to_cpup(prop);

	prop = of_get_property(node, "#size-cells", NULL);
	*c_size_cells = be32_to_cpup(prop);

	*cell_count = *addr_cells + *c_addr_cells + *c_size_cells;
	tuple_len = (*cell_count) * sizeof(__be32);

	if (ranges_len % tuple_len) {
		pr_warn("malformed ranges entry '%s'\n", node->name);
		return -EINVAL;
	}
	return 0;
}

static int mbus_dt_setup(struct mvebu_mbus_state *mbus)
{
	int addr_cells, c_addr_cells, c_size_cells;
	int i, ret, cell_count;
	const __be32 *r, *ranges_start, *ranges_end;

	ret = mbus_parse_ranges(mbus, &addr_cells, &c_addr_cells,
				&c_size_cells, &cell_count,
				&ranges_start, &ranges_end);
	if (ret < 0)
		return ret;

	for (i = 0, r = ranges_start; r < ranges_end; r += cell_count, i++) {
		u32 windowid, base, size;
		u8 target, attr;

		/*
		 * An entry with a non-zero custom field do not
		 * correspond to a static window, so skip it.
		 */
		windowid = of_read_number(r, 1);
		if (CUSTOM(windowid))
			continue;

		target = TARGET(windowid);
		attr = ATTR(windowid);

		base = of_read_number(r + c_addr_cells, addr_cells);
		size = of_read_number(r + c_addr_cells + addr_cells,
				      c_size_cells);
		ret = mbus_dt_setup_win(mbus, base, size, target, attr);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static void mvebu_mbus_get_pcie_resources(struct device_node *np,
			  struct resource *mem, struct resource *io)
{
	u32 reg[2];
	int ret;

	/*
	 * These are optional, so we make sure that resource_size(x) will
	 * return 0.
	 */
	memset(mem, 0, sizeof(struct resource));
	mem->end = -1;
	memset(io, 0, sizeof(struct resource));
	io->end = -1;

	ret = of_property_read_u32_array(np, "pcie-mem-aperture",
					 reg, ARRAY_SIZE(reg));
	if (!ret) {
		mem->start = reg[0];
		mem->end = mem->start + reg[1] - 1;
		mem->flags = IORESOURCE_MEM;
	}

	ret = of_property_read_u32_array(np, "pcie-io-aperture",
					 reg, ARRAY_SIZE(reg));
	if (!ret) {
		io->start = reg[0];
		io->end = io->start + reg[1] - 1;
		io->flags = IORESOURCE_IO;
	}
}

int mvebu_mbus_init(void)
{
	struct device_node *np, *controller;
	const struct of_device_id *match;
	const __be32 *prop;
	int win;

	np = of_find_matching_node_and_match(NULL, mvebu_mbus_dt_ids, &match);
	if (!np) {
		pr_err("could not find a matching SoC family\n");
		return -ENODEV;
	}
	mbus_state.np = np;
	mbus_state.soc = (struct mvebu_mbus_soc_data *)match->data;

	prop = of_get_property(np, "controller", NULL);
	if (!prop) {
		pr_err("required 'controller' property missing\n");
		return -EINVAL;
	}

	controller = of_find_node_by_phandle(be32_to_cpup(prop));
	if (!controller) {
		pr_err("could not find an 'mbus-controller' node\n");
		return -ENODEV;
	}

	mbus_state.mbuswins_base = of_iomap(controller, 0);
	if (!mbus_state.mbuswins_base) {
		pr_err("cannot get MBUS register address\n");
		return -ENOMEM;
	}

	mbus_state.sdramwins_base = of_iomap(controller, 1);
	if (!mbus_state.sdramwins_base) {
		pr_err("cannot get SDRAM register address\n");
		return -ENOMEM;
	}

	/* Get optional pcie-{mem,io}-aperture properties */
	mvebu_mbus_get_pcie_resources(np, &mbus_state.pcie_mem_aperture,
					  &mbus_state.pcie_io_aperture);

	for (win = 0; win < mbus_state.soc->num_wins; win++)
		mvebu_mbus_disable_window(&mbus_state, win);

	mbus_state.soc->setup_cpu_target(&mbus_state);

	/* Setup statically declared windows in the DT */
	return mbus_dt_setup(&mbus_state);
}

struct mbus_range {
	const char *compatible;
	u32 mbusid;
	u32 remap;
	struct list_head list;
};

#define MBUS_ID(t,a)	(((t) << 24) | ((attr) << 16))
static LIST_HEAD(mbus_ranges);

void mvebu_mbus_add_range(const char *compatible, u8 target, u8 attr, u32 remap)
{
	struct mbus_range *r = xzalloc(sizeof(*r));

	r->compatible = strdup(compatible);
	r->mbusid = MBUS_ID(target, attr);
	r->remap = remap;
	list_add_tail(&r->list, &mbus_ranges);
}

/*
 * Barebox always remaps internal registers to 0xf1000000 on every SoC.
 * As we (and Linux) need a working DT and there is no way to tell the current
 * remap address, fixup any provided DT to ensure custom MBUS_IDs are correct.
 */
static int mvebu_mbus_of_fixup(struct device_node *root, void *context)
{
	struct device_node *np;

	for_each_matching_node_from(np, root, mvebu_mbus_dt_ids) {
		struct property *p;
		int n, pa, na, ns, lenp, size;
		u32 *ranges;

		p = of_find_property(np, "ranges", &lenp);
		if (!p)
			return -EINVAL;

		pa = of_n_addr_cells(np);
		if (of_property_read_u32(np, "#address-cells", &na) ||
		    of_property_read_u32(np, "#size-cells", &ns))
			return -EINVAL;

		size = pa + na + ns;
		ranges = xzalloc(lenp);
		of_property_read_u32_array(np, "ranges", ranges, lenp/4);

		/*
		 * Iterate through each ranges tuple and fixup the custom
		 * window ranges low base address. Because Armada XP supports
		 * LPAE, it has 2 cells for the parent address:
		 *   <windowid child_base high_base low_base size>
		 *
		 * whereas for Armada 370, there's just one:
		 *   <windowid child_base base size>
		 *
		 * For instance, the following tuple:
		 *   <MBUS_ID(0xf0, 0x01) child_base {0} base 0x100000>
		 *
		 * would be fixed-up like:
		 *   <MBUS_ID(0xf0, 0x01) child_base {0} remap 0x100000>
		 */
		for (n = 0; n < lenp/4; n += size) {
			struct mbus_range *r;
			u32 mbusid = ranges[n];

			if (!CUSTOM(mbusid))
				continue;

			list_for_each_entry(r, &mbus_ranges, list) {
				if (!of_machine_is_compatible(r->compatible))
					continue;
				if (r->mbusid == mbusid)
					ranges[n + na + pa - 1] = r->remap;
			}
		}

		if (of_property_write_u32_array(np, "ranges", ranges, lenp/4))
			pr_err("Unable to fixup mbus ranges\n");
		free(ranges);
	}

	return 0;
}

static int mvebu_mbus_fixup_register(void) {
	return of_register_fixup(mvebu_mbus_of_fixup, NULL);
}
pure_initcall(mvebu_mbus_fixup_register);
