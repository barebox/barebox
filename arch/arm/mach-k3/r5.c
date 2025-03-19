// SPDX-License-Identifier: GPL-2.0-only
#include <io.h>
#include <linux/kernel.h>
#include <mach/k3/r5.h>
#include <asm/armv7r-mpu.h>
#include <init.h>
#include <libfile.h>
#include <fs.h>
#include <fip.h>
#include <firmware.h>
#include <linux/remoteproc.h>
#include <soc/ti/ti_sci_protocol.h>
#include <linux/clk.h>
#include <elf.h>
#include <asm/cache.h>
#include <linux/sizes.h>
#include <barebox.h>
#include <bootsource.h>
#include <linux/usb/gadget-multi.h>
#include <mach/k3/common.h>
#include <mci.h>
#include <fiptool.h>

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

#if IN_PROPER

/*
 * The bl31 and optee binaries are relocatable, but these addresses
 * are hardcoded as reserved mem regions in the upstream device trees.
 */
#define BL31_ADDRESS (void *)0x9e780000
#define BL32_ADDRESS (void *)0x9e800000
#define BAREBOX_ADDRESS (void *)0x80080000

static void *k3_ti_dm;

static bool have_bl31;
static bool have_bl32;
static bool have_bl33;

static uuid_t uuid_bl31 = UUID_EL3_RUNTIME_FIRMWARE_BL31;
static uuid_t uuid_ti_dm_fw = UUID_TI_DM_FW;
static uuid_t uuid_bl33 = UUID_NON_TRUSTED_FIRMWARE_BL33;
static uuid_t uuid_bl32 = UUID_SECURE_PAYLOAD_BL32;

static struct fip_state *fip_image_load_auth(const char *filename, size_t offset)
{
	struct fip_state *fip = NULL;
	int fd;
	unsigned int maxsize = SZ_4M;
	size_t size;
	void *buf = NULL;
	int ret;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return ERR_PTR(-errno);

	if (offset) {
		loff_t pos;
		pos = lseek(fd, offset, SEEK_SET);
		if (pos < 0) {
			ret = -errno;
			goto err;
		}
	}

	buf = xzalloc(maxsize);

	/*
	 * There is no easy way to determine the size of the certificates the ROM
	 * takes as images, so the best we can do here is to assume a maximum size
	 * and load this.
	 */
	ret = read_full(fd, buf, maxsize);
	if (ret < 0)
		goto err;

	size = maxsize;

	ret = k3_authenticate_image(&buf, &size);
	if (ret) {
		pr_err("Failed to authenticate %s\n", filename);
		goto err;
	}

	fip = fip_new();
	ret = fip_parse_buf(fip, buf, size, NULL);
	if (ret)
		goto err;

	close(fd);

	return fip;
err:
	if (fip)
		fip_free(fip);
	close(fd);
	free(buf);

	return ERR_PTR(ret);
}

static int load_fip(const char *filename, off_t offset)
{
	struct fip_state *fip;
	struct fip_image_desc *desc;
	unsigned char shasum[SHA256_DIGEST_SIZE];
	int ret;

	if (IS_ENABLED(CONFIG_ARCH_K3_AUTHENTICATE_IMAGE))
		fip = fip_image_load_auth(filename, offset);
	else
		fip = fip_image_open(filename, offset);

	if (IS_ERR(fip)) {
		pr_err("Cannot open FIP image: %pe\n", fip);
		return PTR_ERR(fip);
	}

	if (IS_ENABLED(CONFIG_FIRMWARE_VERIFY_NEXT_IMAGE)) {
		ret = fip_sha256(fip, shasum);
		if (ret) {
			pr_err("Cannot calc fip sha256: %pe\n", ERR_PTR(ret));
			return ret;
		}

		ret = firmware_next_image_check_sha256(shasum, true);
		if (ret)
			return ret;
	}

	fip_for_each_desc(fip, desc) {
		struct fip_toc_entry *toc_entry = &desc->image->toc_e;

		if (uuid_equal(&toc_entry->uuid, &uuid_bl31)) {
			memcpy(BL31_ADDRESS, desc->image->buffer, toc_entry->size);
			have_bl31 = true;
		}

		if (uuid_equal(&toc_entry->uuid, &uuid_bl33)) {
			memcpy(BAREBOX_ADDRESS, desc->image->buffer, toc_entry->size);
			have_bl33 = true;
		}

		if (uuid_equal(&toc_entry->uuid, &uuid_bl32)) {
			memcpy(BL32_ADDRESS, desc->image->buffer, toc_entry->size);
			have_bl32 = true;
		}

		if (uuid_equal(&toc_entry->uuid, &uuid_ti_dm_fw)) {
			k3_ti_dm = xmemdup(desc->image->buffer, toc_entry->size);
		}
	}

	fip_free(fip);

	return 0;
}

static int do_dfu(void)
{
	struct usbgadget_funcs funcs = {};
	int ret;
	struct stat s;

	funcs.flags |= USBGADGET_DFU;
	funcs.dfu_opts = "/fip.img(fip)cs";

	ret = usbgadget_prepare_register(&funcs);
	if (ret) {
		pr_err("DFU failed with: %pe\n", ERR_PTR(ret));
		return ret;
	}

	while (1) {
		ret = stat("/fip.img", &s);
		if (!ret) {
			printf("Downloaded FIP image, DFU done\n");
			ret = load_fip("/fip.img", 0);
			if (!ret)
				return 0;
			unlink("/fip.img");
		}

		command_slice_release();
		mdelay(50);
		command_slice_acquire();
	};
}

static int load_fip_emmc(void)
{
	int bootpart;
	struct mci *mci;
	char *fname;
	const char *mmcdev = "mmc0";
	int ret;

	device_detect_by_name(mmcdev);

	mci = mci_get_device_by_name(mmcdev);
	if (!mci)
		return -EINVAL;

	bootpart = mci->bootpart;

	if (bootpart != 1 && bootpart != 2) {
		pr_err("Unexpected bootpart value %d\n", bootpart);
		bootpart = 1;
	}

	fname = xasprintf("/dev/%s.boot%d", mmcdev, bootpart - 1);

	ret = load_fip(fname, K3_EMMC_BOOTPART_TIBOOT3_BIN_SIZE);

	free(fname);

	return ret;
}

static int k3_r5_start_image(void)
{
	int ret;
	struct firmware fw;
	const struct ti_sci_handle *ti_sci;
	struct elf_image *elf;
	void __noreturn (*ti_dm)(void);
	struct rproc *arm64_rproc;

	if (IS_ENABLED(CONFIG_USB_GADGET_DFU) && bootsource_get() == BOOTSOURCE_SERIAL)
		ret = do_dfu();
	else if (k3_boot_is_emmc())
		ret = load_fip_emmc();
	else
		ret = load_fip("/boot/k3.fip", 0);

	if (ret) {
		pr_crit("Unable to load FIP image\n");
		panic("Stop booting\n");
	}

	if (!have_bl31)
		panic("No TFA found in FIP image\n");

	if (!have_bl32)
		pr_info("No OP-TEE found. Continuing without\n");

	if (!have_bl33)
		panic("No bl33 found in FIP image\n");

	if (!k3_ti_dm)
		panic("No ti-dm binary found\n");

	ti_sci = ti_sci_get_handle(NULL);
	if (IS_ERR(ti_sci))
		return -EINVAL;

	arm64_rproc = ti_k3_am64_get_handle();
	if (!arm64_rproc) {
		pr_err("Cannot get rproc handle\n");
		return -EINVAL;
	}

	elf = elf_open_binary(k3_ti_dm);
	if (IS_ERR(elf)) {
		pr_err("Cannot open ELF image %pe\n", elf);
		return PTR_ERR(elf);
	}

	ret = elf_load(elf);
	if (ret) {
		pr_err("Cannot load ELF image %pe\n", ERR_PTR(ret));
		elf_close(elf);
	}

	free(k3_ti_dm);

	fw.data = BL31_ADDRESS;

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
#endif /* IN_PROPER */
