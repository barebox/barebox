// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: 2024 Marco Felsch, Pengutronix
/*
 * Based on Linux drivers/soc/imx/soc-imx8m.c:
 * Copyright 2019 NXP.
 */

#include <init.h>
#include <of.h>
#include <of_address.h>
#include <pm_domain.h>

#include <asm-generic/memory_layout.h>

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/arm-smccc.h>
#include <linux/clk.h>

#include <mach/imx/generic.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/reset-reason.h>
#include <mach/imx/revision.h>
#include <mach/imx/scratch.h>
#include <mach/imx/tzasc.h>

#include <tee/optee.h>

#define REV_B1				0x21

#define IMX8MQ_SW_INFO_B1		0x40
#define IMX8MQ_SW_MAGIC_B1		0xff0055aa

#define IMX_SIP_GET_SOC_INFO		0xc2000006

#define OCOTP_UID_LOW			0x410
#define OCOTP_UID_HIGH			0x420

#define IMX8MP_OCOTP_UID_OFFSET		0x10

/* Same as ANADIG_DIGPROG_IMX7D */
#define ANADIG_DIGPROG_IMX8MM	0x800

struct imx8_soc_data {
	char *name;
	u32 (*soc_revision)(void);
	void (*save_boot_loc)(void);
};

static u64 soc_uid;

#ifdef CONFIG_HAVE_ARM_SMCCC
static u32 imx8mq_soc_revision_from_atf(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(IMX_SIP_GET_SOC_INFO, 0, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0 == SMCCC_RET_NOT_SUPPORTED)
		return 0;
	else
		return res.a0 & 0xff;
}
#else
static inline u32 imx8mq_soc_revision_from_atf(void) { return 0; };
#endif

static u32 __init imx8mq_soc_revision(void)
{
	struct device_node *np;
	void __iomem *ocotp_base;
	u32 magic;
	u32 rev;
	struct clk *clk;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx8mq-ocotp");
	if (!np)
		return 0;

	ocotp_base = of_iomap(np, 0);
	WARN_ON(!ocotp_base);
	clk = of_clk_get_by_name(np, NULL);
	if (IS_ERR(clk)) {
		WARN_ON(IS_ERR(clk));
		return 0;
	}

	clk_prepare_enable(clk);

	/*
	 * SOC revision on older imx8mq is not available in fuses so query
	 * the value from ATF instead.
	 */
	rev = imx8mq_soc_revision_from_atf();
	if (!rev) {
		magic = readl_relaxed(ocotp_base + IMX8MQ_SW_INFO_B1);
		if (magic == IMX8MQ_SW_MAGIC_B1)
			rev = REV_B1;
	}

	soc_uid = readl_relaxed(ocotp_base + OCOTP_UID_HIGH);
	soc_uid <<= 32;
	soc_uid |= readl_relaxed(ocotp_base + OCOTP_UID_LOW);

	/* Keep the OCOTP clk on for the TF-A else the CPU stuck */
	of_node_put(np);

	return rev;
}

static void __init imx8mm_soc_uid(void)
{
	void __iomem *ocotp_base;
	struct device_node *np;
	struct clk *clk;
	u32 offset = of_machine_is_compatible("fsl,imx8mp") ?
		     IMX8MP_OCOTP_UID_OFFSET : 0;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx8mm-ocotp");
	if (!np)
		return;

	ocotp_base = of_iomap(np, 0);
	WARN_ON(!ocotp_base);
	clk = of_clk_get_by_name(np, NULL);
	if (IS_ERR(clk)) {
		WARN_ON(IS_ERR(clk));
		return;
	}

	clk_prepare_enable(clk);

	soc_uid = readl_relaxed(ocotp_base + OCOTP_UID_HIGH + offset);
	soc_uid <<= 32;
	soc_uid |= readl_relaxed(ocotp_base + OCOTP_UID_LOW + offset);

	/* Keep the OCOTP clk on for the TF-A else the CPU stuck */
	of_node_put(np);
}

static u32 __init imx8mm_soc_revision(void)
{
	struct device_node *np;
	void __iomem *anatop_base;
	u32 rev;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx8mm-anatop");
	if (!np)
		return 0;

	anatop_base = of_iomap(np, 0);
	WARN_ON(!anatop_base);

	rev = readl_relaxed(anatop_base + ANADIG_DIGPROG_IMX8MM);

	of_node_put(np);

	imx8mm_soc_uid();

	return rev;
}

static const struct imx8_soc_data imx8mq_soc_data = {
	.name = "i.MX8MQ",
	.soc_revision = imx8mq_soc_revision,
	.save_boot_loc = imx8mq_boot_save_loc,
};

static const struct imx8_soc_data imx8mm_soc_data = {
	.name = "i.MX8MM",
	.soc_revision = imx8mm_soc_revision,
	.save_boot_loc = imx8mm_boot_save_loc,
};

static const struct imx8_soc_data imx8mn_soc_data = {
	.name = "i.MX8MN",
	.soc_revision = imx8mm_soc_revision,
	.save_boot_loc = imx8mn_boot_save_loc,
};

static const struct imx8_soc_data imx8mp_soc_data = {
	.name = "i.MX8MP",
	.soc_revision = imx8mm_soc_revision,
	.save_boot_loc = imx8mp_boot_save_loc,
};

static __maybe_unused const struct of_device_id imx8_soc_match[] = {
	{ .compatible = "fsl,imx8mq", .data = &imx8mq_soc_data, },
	{ .compatible = "fsl,imx8mm", .data = &imx8mm_soc_data, },
	{ .compatible = "fsl,imx8mn", .data = &imx8mn_soc_data, },
	{ .compatible = "fsl,imx8mp", .data = &imx8mp_soc_data, },
	{ }
};

static int imx8_soc_imx8m_init(struct soc_device_attribute *soc_dev_attr)
{
	void __iomem *src = IOMEM(MX8M_SRC_BASE_ADDR);
	const char *uid = soc_dev_attr->serial_number;
	const char *cputypestr = soc_dev_attr->soc_id;

	genpd_activate();

	/*
	 * Reset reasons seem to be identical to that of i.MX7
	 */
	imx_set_reset_reason(src + IMX7_SRC_SRSR, imx7_reset_reasons);
	pr_info("%s unique ID: %s\n", cputypestr, uid);

	if (IS_ENABLED(CONFIG_PBL_OPTEE) && imx8m_tzc380_is_enabled()) {
		static struct of_optee_fixup_data optee_fixup_data = {
			.shm_size = OPTEE_SHM_SIZE,
			.method = "smc",
		};

		optee_set_membase(imx_scratch_get_optee_hdr());
		of_optee_fixup(of_get_root_node(), &optee_fixup_data);
		of_register_fixup(of_optee_fixup, &optee_fixup_data);
	}

	return 0;
}

#define imx8_revision(soc_rev) \
	soc_rev ? \
	xasprintf("%d.%d", (soc_rev >> 4) & 0xf,  soc_rev & 0xf) : \
	"unknown"

static int __init imx8_soc_init(void)
{
	struct device_node *of_root = of_get_root_node();
	struct soc_device_attribute *soc_dev_attr;
	struct soc_device *soc_dev;
	const struct of_device_id *id;
	u32 soc_rev = 0;
	const struct imx8_soc_data *data;
	int ret;

	id = of_match_node(imx8_soc_match, of_root);
	if (!id)
		return 0;

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENOMEM;

	soc_dev_attr->family = "Freescale i.MX";

	ret = of_property_read_string(of_root, "model", &soc_dev_attr->machine);
	if (ret)
		goto free_soc;

	data = id->data;
	if (data) {
		soc_dev_attr->soc_id = data->name;
		if (data->soc_revision)
			soc_rev = data->soc_revision();
		if (data->save_boot_loc)
			data->save_boot_loc();
	}

	soc_dev_attr->revision = imx8_revision(soc_rev);
	if (!soc_dev_attr->revision) {
		ret = -ENOMEM;
		goto free_soc;
	}

	soc_dev_attr->serial_number = xasprintf("%016llX", soc_uid);
	if (!soc_dev_attr->serial_number) {
		ret = -ENOMEM;
		goto free_rev;
	}

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev)) {
		ret = PTR_ERR(soc_dev);
		goto free_serial_number;
	}

	imx_set_silicon_revision(soc_dev_attr->soc_id, soc_rev);

	return imx8_soc_imx8m_init(soc_dev_attr);

free_serial_number:
	kfree(soc_dev_attr->serial_number);
free_rev:
	if (strcmp(soc_dev_attr->revision, "unknown"))
		kfree(soc_dev_attr->revision);
free_soc:
	kfree(soc_dev_attr);
	return ret;
}
/* Aligned with imx_init() to not cause regressions */
postcore_initcall(imx8_soc_init);
MODULE_LICENSE("GPL");
