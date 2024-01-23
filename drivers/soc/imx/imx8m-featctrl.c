// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2022 Ahmad Fatoum, Pengutronix

#include <common.h>
#include <linux/bitmap.h>
#include <featctrl.h>
#include <soc/imx8m/featctrl.h>

#include <dt-bindings/features/imx8m.h>

struct imx_feat {
	struct feature_controller feat;
	unsigned long features[BITS_TO_LONGS(IMX8M_FEAT_END)];
};

static struct imx_feat *to_imx_feat(struct feature_controller *feat)
{
	return container_of(feat, struct imx_feat, feat);
}

static int imx8m_feat_check(struct feature_controller *feat, int idx)
{
	struct imx_feat *priv = to_imx_feat(feat);

	if (idx > IMX8M_FEAT_END)
		return -EINVAL;

	return test_bit(idx, priv->features) ? FEATCTRL_OKAY : FEATCTRL_GATED;
}

static inline bool is_fused(u32 val, u32 bitmask)
{
	return bitmask && (val & bitmask) == bitmask;
}

#define field_get(_mask, _reg) (((_reg) & (_mask)) >> (ffs(_mask) - 1))

static void check_cpus(u32 mask, u32 reg, unsigned long *features)
{
	switch (field_get(mask, reg)) {
	case 0b11:
		clear_bit(IMX8M_FEAT_CPU_DUAL, features);
		fallthrough;
	case 0b10:
		clear_bit(IMX8M_FEAT_CPU_QUAD, features);
	}
}

int imx8m_feat_ctrl_init(struct device *dev, u32 tester3, u32 tester4,
			 const struct imx8m_featctrl_data *data)
{
	unsigned long *features;
	struct imx_feat *priv;

	if (!dev || !data)
		return -ENODEV;

	dev_dbg(dev, "tester3 = 0x%08x, tester4 = 0x%08x\n", tester3, tester4);

	priv = xzalloc(sizeof(*priv));
	features = priv->features;

	bitmap_fill(features, IMX8M_FEAT_END);

	if (is_fused(tester3, data->tester3.vpu_bitmask) ||
	    is_fused(tester4, data->tester4.vpu_bitmask))
		clear_bit(IMX8M_FEAT_VPU, features);
	if (is_fused(tester4, data->tester4.gpu_bitmask))
		clear_bit(IMX8M_FEAT_GPU, features);
	if (is_fused(tester4, data->tester4.mipi_dsi_bitmask))
		clear_bit(IMX8M_FEAT_MIPI_DSI, features);
	if (is_fused(tester4, data->tester4.isp_bitmask))
		clear_bit(IMX8M_FEAT_ISP, features);
	if (is_fused(tester4, data->tester4.npu_bitmask))
		clear_bit(IMX8M_FEAT_NPU, features);
	if (is_fused(tester4, data->tester4.lvds_bitmask))
		clear_bit(IMX8M_FEAT_LVDS, features);
	if (is_fused(tester4, data->tester4.dsp_bitmask))
		clear_bit(IMX8M_FEAT_DSP, features);

	if (data->tester3.cpu_bitmask)
		check_cpus(data->tester3.cpu_bitmask, tester3, features);
	else if (data->tester4.cpu_bitmask)
		check_cpus(data->tester4.cpu_bitmask, tester4, features);

	priv->feat.dev = dev;
	priv->feat.check = imx8m_feat_check;

	return feature_controller_register(&priv->feat);
}
