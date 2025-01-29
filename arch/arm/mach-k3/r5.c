// SPDX-License-Identifier: GPL-2.0-only
#include <io.h>
#include <linux/kernel.h>
#include <mach/k3/r5.h>
#include <asm/armv7r-mpu.h>
#include <init.h>
#include <libfile.h>
#include <fs.h>
#include <firmware.h>
#include <linux/remoteproc.h>
#include <soc/ti/ti_sci_protocol.h>
#include <linux/clk.h>
#include <elf.h>
#include <asm/cache.h>
#include <linux/sizes.h>
#include <barebox.h>

#define RTC_BASE_ADDRESS		0x2b1f0000
#define K3RTC_KICK0_UNLOCK_VALUE	0x83e70b13
#define K3RTC_KICK1_UNLOCK_VALUE	0x95a4f1e0
#define REG_K3RTC_S_CNT_LSW		0x18
#define REG_K3RTC_KICK0			0x70
#define REG_K3RTC_KICK1			0x74

/*
 * RTC Erratum i2327 Workaround for Silicon Revision 1
 *
 * Due to a bug in initial synchronization out of cold power on,
 * IRQ status can get locked infinitely if we do not unlock RTC
 *
 * This workaround *must* be applied within 1 second of power on,
 * So, this is closest point to be able to guarantee the max
 * timing.
 *
 * https://www.ti.com/lit/er/sprz487c/sprz487c.pdf
 */
static void rtc_erratumi2327_init(void)
{
	void __iomem *rtc_base = IOMEM(RTC_BASE_ADDRESS);
	u32 counter;

	/*
	 * If counter has gone past 1, nothing we can do, leave
	 * system locked! This is the only way we know if RTC
	 * can be used for all practical purposes.
	 */
	counter = readl(rtc_base + REG_K3RTC_S_CNT_LSW);
	if (counter > 1)
		return;
	/*
	 * Need to set this up at the very start
	 * MUST BE DONE under 1 second of boot.
	 */
	writel(K3RTC_KICK0_UNLOCK_VALUE, rtc_base + REG_K3RTC_KICK0);
	writel(K3RTC_KICK1_UNLOCK_VALUE, rtc_base + REG_K3RTC_KICK1);
}

#define CTRLMMR_LOCK_KICK0_UNLOCK_VAL	0x68ef3490
#define CTRLMMR_LOCK_KICK1_UNLOCK_VAL	0xd172bc5a
#define CTRLMMR_LOCK_KICK0		0x1008
#define CTRLMMR_LOCK_KICK1		0x100c
#define CTRL_MMR0_PARTITION_SIZE	0x4000
#define WKUP_CTRL_MMR0_BASE		0x43000000
#define CTRL_MMR0_BASE			0x00100000
#define MCU_CTRL_MMR0_BASE		0x04500000
#define PADCFG_MMR0_BASE		0x04080000
#define PADCFG_MMR1_BASE		0x000f0000

static void mmr_unlock(uintptr_t base, u32 partition)
{
	/* Translate the base address */
	uintptr_t part_base = base + partition * CTRL_MMR0_PARTITION_SIZE;

	/* Unlock the requested partition if locked using two-step sequence */
	writel(CTRLMMR_LOCK_KICK0_UNLOCK_VAL, part_base + CTRLMMR_LOCK_KICK0);
	writel(CTRLMMR_LOCK_KICK1_UNLOCK_VAL, part_base + CTRLMMR_LOCK_KICK1);
}

void k3_ctrl_mmr_unlock(void)
{
	rtc_erratumi2327_init();

	/* Unlock all WKUP_CTRL_MMR0 module registers */
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 0);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 1);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 2);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 3);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 4);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 5);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 6);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 7);

	/* Unlock all CTRL_MMR0 module registers */
	mmr_unlock(CTRL_MMR0_BASE, 0);
	mmr_unlock(CTRL_MMR0_BASE, 1);
	mmr_unlock(CTRL_MMR0_BASE, 2);
	mmr_unlock(CTRL_MMR0_BASE, 4);
	mmr_unlock(CTRL_MMR0_BASE, 6);

	/* Unlock all MCU_CTRL_MMR0 module registers */
	mmr_unlock(MCU_CTRL_MMR0_BASE, 0);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 1);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 2);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 3);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 4);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 6);

	/* Unlock PADCFG_CTRL_MMR padconf registers */
	mmr_unlock(PADCFG_MMR0_BASE, 1);
	mmr_unlock(PADCFG_MMR1_BASE, 1);
}

#define CONFIG_SPL_TEXT_BASE	0x43c00000 /* FIXME */
#define CFG_SYS_SDRAM_BASE	0x80000000 /* FIXME */

static struct mpu_region_config k3_mpu_regions[] = {
	{
		.start_addr = 0x00000000,
		.region_no = REGION_0,
		.xn = XN_EN,
		.ap = PRIV_RW_USR_RW,
		.mr_attr = SHARED_WRITE_BUFFERED,
		.reg_size = REGION_4GB,
	}, {
		/* SPL SRAM WB and Write allocate. */
		.start_addr = 0x43c00000,
		.region_no = REGION_1,
		.xn = XN_DIS,
		.ap = PRIV_RW_USR_RW,
		.mr_attr = O_I_WB_RD_WR_ALLOC,
		.reg_size = REGION_8MB,
	}, {
		/* DRAM area WB and Write allocate */
		.start_addr = 0x80000000,
		.region_no = REGION_2,
		.xn = XN_DIS,
		.ap = PRIV_RW_USR_RW,
		.mr_attr = O_I_WB_RD_WR_ALLOC,
		.reg_size = REGION_2GB,
	}, {
		/* mcu_r5fss0_core0 WB and Write allocate */
		.start_addr = 0x41010000,
		.region_no = REGION_3,
		.xn = XN_DIS,
		.ap = PRIV_RW_USR_RW,
		.mr_attr = O_I_WB_RD_WR_ALLOC,
		.reg_size = REGION_8MB,
	},
};

void k3_mpu_setup_regions(void)
{
	armv7r_mpu_setup_regions(k3_mpu_regions, ARRAY_SIZE(k3_mpu_regions));
}

#include <soc/k3/clk.h>

#define PSC_PTCMD               0x120
#define PSC_PTCMD_H             0x124
#define PSC_PTSTAT              0x128
#define PSC_PTSTAT_H            0x12C
#define PSC_PDSTAT              0x200
#define PSC_PDCTL               0x300
#define PSC_MDSTAT              0x800
#define PSC_MDCTL               0xa00

#define MDSTAT_STATE_MASK               0x3f
#define MDSTAT_BUSY_MASK                0x30
#define MDSTAT_STATE_SWRSTDISABLE       0x0
#define MDSTAT_STATE_ENABLE             0x3

static void ti_pd_wait(void __iomem *base, int id)
{
	u32 pdoffset = 0;
	u32 ptstatreg = PSC_PTSTAT;

	if (id > 31) {
		pdoffset = 32;
		ptstatreg = PSC_PTSTAT_H;
	}

	while (readl(base + ptstatreg) & BIT(id - pdoffset));
}

void am625_early_init(void)
{
	void __iomem *pd_base = (void *)0x400000;
	u32 val;
	volatile int i;

	ti_k3_pll_init((void *)0x68c000);

	/* hsdiv0_16fft_main_12_hsdivout0_clk divide by 2 */
	val = readl(0x68c080);
	val &= ~0xff;
	val |= 31;
	writel(val, 0x68c080);

	/* PLL needs a rest, barebox would hang during PLL setup without this delay */
	for (i = 0; i < 1000000; i++);

	/*
	 * configure PLL to 800MHz, with the above divider DDR frequency
	 * results in 400MHz.
	 */
	ti_k3_pll_set_rate((void *)0x68c000, 800000000, 25000000);

	/* Enable DDR controller power domains */
	writel(0x103, pd_base + 0x00000a24);
	writel(0x1, pd_base + 0x00000120);
	ti_pd_wait(pd_base, 0);
	writel(0x103, pd_base + 0x00000a28);
	writel(0x1, pd_base + 0x00000120);
	ti_pd_wait(pd_base, 0);
	writel(0x103, pd_base + 0x00000a2c);
	writel(0x1, pd_base + 0x00000120);
	ti_pd_wait(pd_base, 0);
}

/*
 * The bl31 and optee binaries are relocatable, but these addresses
 * are hardcoded as reserved mem regions in the upstream device trees.
 */
#define BL31_ADDRESS 0x9e780000
#define OPTEE_ADDRESS 0x9e800000

static int k3_r5_start_image(void)
{
	int err;
	void *ti_dm_buf;
	ssize_t size;
	struct firmware fw;
	const struct ti_sci_handle *ti_sci;
	void *bl31 = (void *)BL31_ADDRESS;
	void *barebox = (void *)0x80080000;
	void *optee = (void *)OPTEE_ADDRESS;
	struct elf_image *elf;
	void __noreturn (*ti_dm)(void);
	struct rproc *arm64_rproc;

	ti_sci = ti_sci_get_handle(NULL);
	if (IS_ERR(ti_sci))
		return -EINVAL;

	arm64_rproc = ti_k3_am64_get_handle();
	if (!arm64_rproc) {
		pr_err("Cannot get rproc handle\n");
		return -EINVAL;
	}

	size = read_file_into_buf("/boot/optee.bin", optee, SZ_32M);
	if (size < 0) {
		if (size != -ENOENT) {
			pr_err("Cannot load optee.bin: %pe\n", ERR_PTR(size));
			return size;
		}
		pr_info("optee.bin not found, continue without\n");
	} else {
		pr_debug("Loaded optee.bin (size %u) to 0x%p\n", size, optee);
	}

	size = read_file_into_buf("/boot/barebox.bin", barebox, optee - barebox);
	if (size < 0) {
		pr_err("Cannot load barebox.bin: %pe\n", ERR_PTR(size));
		return size;
	}
	pr_debug("Loaded barebox.bin (size %u) to 0x%p\n", size, barebox);

	size = read_file_into_buf("/boot/bl31.bin", bl31, barebox - optee);
	if (size < 0) {
		pr_err("Cannot load bl31.bin: %pe\n", ERR_PTR(size));
		return size;
	}
	pr_debug("Loaded bl31.bin (size %u) to 0x%p\n", size, bl31);

	err = read_file_2("/boot/ti-dm.bin", &size, &ti_dm_buf, FILESIZE_MAX);
	if (err) {
		pr_err("Cannot load ti-dm.bin: %pe\n", ERR_PTR(err));
		return err;
	}
	pr_debug("Loaded ti-dm.bin (size %u)\n", size);

	elf = elf_open_binary(ti_dm_buf);
	if (IS_ERR(elf)) {
		pr_err("Cannot open ELF image %pe\n", elf);
		return PTR_ERR(elf);
	}

	err = elf_load(elf);
	if (err) {
		pr_err("Cannot load ELF image %pe\n", ERR_PTR(err));
		elf_close(elf);
	}

	free(ti_dm_buf);

	fw.data = bl31;

	/* Release all the exclusive devices held by SPL before starting ATF */
	pr_info("Starting TF-A on A53 core\n");

	sync_caches_for_execution();

	ti_sci->ops.dev_ops.release_exclusive_devices(ti_sci);
	arm64_rproc->ops->load(arm64_rproc, &fw);
	arm64_rproc->ops->start(arm64_rproc);

	pr_debug("Starting ti-dm at 0x%08llx\n", elf->entry);

	ti_dm = (void *)(unsigned long)elf->entry;

	ti_dm();
}

static int xload(void)
{
	int ret;

	ret = k3_r5_start_image();

	pr_crit("Starting image failed with: %pe\n", ERR_PTR(ret));
	pr_crit("Nothing left to do\n");

	hang();
}
postenvironment_initcall(xload);
