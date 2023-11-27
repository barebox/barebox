// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2020-2022 NXP
 */
#define pr_fmt(fmt) "s4mu: " fmt

#include <common.h>
#include <io.h>
#include <mach/imx/ele.h>
#include <mach/imx/imx9-regs.h>
#include <linux/iopoll.h>

#define MU_SR_TE0_MASK		BIT(0)
#define MU_SR_RF0_MASK		BIT(0)
#define MU_TR_COUNT		8
#define MU_RR_COUNT		4

struct mu_type {
	u32 ver;
	u32 par;
	u32 cr;
	u32 sr;
	u32 reserved0[60];
	u32 fcr;
	u32 fsr;
	u32 reserved1[2];
	u32 gier;
	u32 gcr;
	u32 gsr;
	u32 reserved2;
	u32 tcr;
	u32 tsr;
	u32 rcr;
	u32 rsr;
	u32 reserved3[52];
	u32 tr[16];
	u32 reserved4[16];
	u32 rr[16];
	u32 reserved5[14];
	u32 mu_attr;
};

static int mu_hal_sendmsg(void __iomem *base, u32 reg_index, u32 msg)
{
	struct mu_type *mu_base = base;
	u32 mask = MU_SR_TE0_MASK << reg_index;
	u32 val;
	int ret;

	/* Wait TX register to be empty. */
	ret = readl_poll_timeout(&mu_base->tsr, val, val & mask, 10000);
	if (ret < 0) {
		pr_debug("%s timeout\n", __func__);
		return -ETIMEDOUT;
	}

	writel(msg, &mu_base->tr[reg_index]);

	return 0;
}

static int mu_hal_receivemsg(void __iomem *base, u32 reg_index, u32 *msg)
{
	struct mu_type *mu_base = base;
	u32 mask = MU_SR_RF0_MASK << reg_index;
	u32 val;
	int ret;

	/* Wait RX register to be full. */
	ret = readl_poll_timeout(&mu_base->rsr, val, val & mask, 10000000);
	if (ret < 0)
		return -ETIMEDOUT;

	*msg = readl(&mu_base->rr[reg_index]);

	return 0;
}

static int mu_read(void __iomem *base, struct ele_msg *msg)
{
	int ret, i;

	/* Read first word */
	ret = mu_hal_receivemsg(base, 0, (u32 *)msg);
	if (ret)
		return ret;

	/* Read remaining words */
	for (i = 1; i < msg->size; i++) {
		ret = mu_hal_receivemsg(base, i % MU_RR_COUNT, &msg->data[i - 1]);
		if (ret)
			return ret;
	}

	return 0;
}

static int mu_write(void __iomem *base, struct ele_msg *msg)
{
	int ret, i;

	/* Write first word */
	ret = mu_hal_sendmsg(base, 0, *((u32 *)msg));
	if (ret)
		return ret;

	/* Write remaining words */
	for (i = 1; i < msg->size; i++) {
		ret = mu_hal_sendmsg(base, i % MU_TR_COUNT, msg->data[i - 1]);
		if (ret)
			return ret;
	}

	return 0;
}

static int imx9_s3mua_call(struct ele_msg *msg, bool get_response)
{
	void __iomem *s3mua = IOMEM(MX9_S3MUA_BASE_ADDR);
	u32 result;
	int ret;

	ret = mu_write(s3mua, msg);
	if (ret)
		return ret;

	if (get_response) {
		ret = mu_read(s3mua, msg);
		if (ret)
			return ret;
	}

	result = msg->data[0];
	if ((result & 0xff) == 0xd6)
		return 0;

	return -EIO;
}

int ele_call(struct ele_msg *msg, bool get_response)
{
	return imx9_s3mua_call(msg, get_response);
}

int ele_read_common_fuse(u16 fuse_id, u32 *fuse_word, u32 *response)
{
	struct ele_msg msg;
	int ret;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_READ_FUSE_REQ;
	msg.data[0] = fuse_id;

	ret = imx9_s3mua_call(&msg, true);

	if (response)
		*response = msg.data[0];

	*fuse_word = msg.data[1];

	return ret;
}

#define ELE_READ_SHADOW_REQ 0xf3

int ele_read_shadow_fuse(u16 fuse_id, u32 *fuse_word, u32 *response)
{
	struct ele_msg msg;
	int ret;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_READ_SHADOW_REQ;
	msg.data[0] = fuse_id;

	ret = imx9_s3mua_call(&msg, true);

	if (response)
		*response = msg.data[0];

	*fuse_word = msg.data[1];

	return ret;
}

int ele_release_rdc(u8 core_id, u8 xrdc, u32 *response)
{
	struct ele_msg msg;
	int ret;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_RELEASE_RDC_REQ;

	switch (xrdc) {
	case 0:
		msg.data[0] = (0x74 << 8) | core_id;
		break;
	case 1:
		msg.data[0] = (0x78 << 8) | core_id;
		break;
	case 2:
		msg.data[0] = (0x82 << 8) | core_id;
		break;
	case 3:
		msg.data[0] = (0x86 << 8) | core_id;
		break;
	default:
		return -EINVAL;
	}

	ret = ele_call(&msg, true);
	if (ret)
		pr_err("%s: ret %d, core id %u, response 0x%x\n",
			__func__, ret, core_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}
