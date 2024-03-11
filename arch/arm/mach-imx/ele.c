// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2020-2022 NXP
 */
#define pr_fmt(fmt) "ele: " fmt

#include <common.h>
#include <io.h>
#include <mach/imx/ele.h>
#include <mach/imx/imx9-regs.h>
#include <linux/iopoll.h>
#include <firmware.h>
#include <linux/bitfield.h>

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

static int imx9_s3mua_call(struct ele_msg *msg)
{
	void __iomem *s3mua = IOMEM(MX9_S3MUA_BASE_ADDR);
	u32 result;
	int ret;

	ret = mu_write(s3mua, msg);
	if (ret)
		return ret;

	ret = mu_read(s3mua, msg);
	if (ret)
		return ret;

	result = msg->data[0];
	if ((result & 0xff) == 0xd6)
		return 0;

	return -EIO;
}

int ele_call(struct ele_msg *msg)
{
	return imx9_s3mua_call(msg);
}

int ele_get_info(struct ele_get_info_data *info)
{
        struct ele_msg msg = {
		.version = ELE_VERSION,
		.tag = ELE_CMD_TAG,
		.size = 4,
		.command = ELE_GET_INFO_REQ,
		.data = {
			upper_32_bits((unsigned long)info),
			lower_32_bits((unsigned long)info),
			sizeof(struct ele_get_info_data),
		},
	};
	int ret;

	ret = ele_call(&msg);
	if (ret)
		pr_err("Could not get ELE info: ret %d, response 0x%x\n",
			ret, msg.data[0]);

	return ret;
}

static int ele_get_start_trng(void)
{
	struct ele_msg msg = {
		.version = ELE_VERSION,
		.tag = ELE_CMD_TAG,
		.size = 1,
		.command = ELE_START_RNG,
	};
	int ret;

	ret = ele_call(&msg);
	if (ret)
		pr_err("Could not start TRNG, response 0x%x\n", msg.data[0]);

	return ret;
}

int imx93_ele_load_fw(void *bl33)
{
	struct ele_get_info_data info = {};
	struct ele_msg msg = {
		.version = ELE_VERSION,
		.tag = ELE_CMD_TAG,
		.size = 4,
		.command = ELE_FW_AUTH_REQ,
	};
	void *firmware;
	int size, ret;
	int rev = 0;

	ele_get_info(&info);

	rev = FIELD_GET(ELE_INFO_SOC_REV, info.soc);

	switch (rev) {
	case 0xa0:
		get_builtin_firmware_ext(mx93a0_ahab_container_img, bl33, &firmware, &size);
		break;
	case 0xa1:
		get_builtin_firmware_ext(mx93a1_ahab_container_img, bl33, &firmware, &size);
		break;
	default:
		pr_err("Unknown unhandled SoC revision %2x\n", rev);
		return -EINVAL;
	}

	/* Address of the container header */
	msg.data[0] = lower_32_bits((unsigned long)firmware);
	/* Actual address of the container header */
	msg.data[2] = lower_32_bits((unsigned long)firmware);

	ret = ele_call(&msg);
	if (ret)
		pr_err("Could not start ELE firmware: ret %d, response 0x%x\n",
			ret, msg.data[0]);

	if (rev >= 0xa1)
		ele_get_start_trng();

	return 0;
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

	ret = imx9_s3mua_call(&msg);

	if (response)
		*response = msg.data[0];

	*fuse_word = msg.data[1];

	return ret;
}

int ele_read_shadow_fuse(u16 fuse_id, u32 *fuse_word, u32 *response)
{
	struct ele_msg msg;
	int ret;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_READ_SHADOW_REQ;
	msg.data[0] = fuse_id;

	ret = imx9_s3mua_call(&msg);

	if (response)
		*response = msg.data[0];

	*fuse_word = msg.data[1];

	return ret;
}

/*
 * ele_write_fuse - write a fuse
 * @fuse_id: The fuse to write to
 * @fuse_val: The value to write to the fuse
 * @lock: lock fuse after writing
 * @response: on return contains the response from ELE
 *
 * This writes the 32bit given in @fuse_val to the fuses at @fuse_id. This is
 * a permanent change, be careful.
 *
 * Return: 0 when the ELE call succeeds, negative error code otherwise
 */
int ele_write_fuse(u16 fuse_id, u32 fuse_val, bool lock, u32 *response)
{
	struct ele_msg msg;
	int ret;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 3;
	msg.command = ELE_WRITE_FUSE_REQ;
	msg.data[0] = (32 << 16) | (fuse_id << 5);

	if (lock)
		msg.data[0] |= (1 << 31);

	msg.data[1] = fuse_val;

	ret = imx9_s3mua_call(&msg);

	if (response)
		*response = msg.data[0];

	return ret;
}

/*
 * ele_forward_lifecycle - forward lifecycle
 * @lc: The lifecycle value to forward to
 * @response: on return contains the response from ELE
 *
 * This changes the chip's lifecycle value. Mainly useful to forward to
 * from ELE_LIFECYCLE_OEM_OPEN to ELE_LIFECYCLE_OEM_CLOSED. When doing
 * this the SoC will only boot authenticated images. Make sure the correct
 * SRK has been fused beforehand, otherwise you brick your board.
 *
 * Return: 0 when the ELE call succeeds, negative error code otherwise
 */
int ele_forward_lifecycle(enum ele_lifecycle lc, u32 *response)
{
	struct ele_msg msg;
	int ret;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 2;
	msg.command = ELE_FWD_LIFECYCLE_UP_REQ;
	msg.data[0] = lc;

	ret = imx9_s3mua_call(&msg);

	if (response)
		*response = msg.data[0];

	return ret;
}

/*
 * ele_authenticate_container - authenticate a container image
 * @addr: the address of the container
 * @response: on return contains the response from ELE
 *
 * This authenticates a container with the ELE. On return the result
 * of the authentication will be encoded in @response
 *
 * Return: 0 when the ELE call succeeds, negative error code otherwise
 */
int ele_authenticate_container(unsigned long addr, u32 *response)
{
	struct ele_msg msg;
	int ret;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 3;
	msg.command = ELE_OEM_CNTN_AUTH_REQ;
	msg.data[0] = upper_32_bits(addr);
	msg.data[1] = lower_32_bits(addr);

	ret = imx9_s3mua_call(&msg);

	if (response)
		*response = msg.data[0];

	return ret;
}

/*
 * ele_release_container - release a container image
 * @response: on return contains the response from ELE
 *
 * This releases a container image. Must be called when done with an
 * image previously authenticated with ele_authenticate_container()
 *
 * Return: 0 when the ELE call succeeds, negative error code otherwise
 */
int ele_release_container(u32 *response)
{
	struct ele_msg msg;
	int ret;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_RELEASE_CONTAINER_REQ;

	ret = imx9_s3mua_call(&msg);

	if (response)
		*response = msg.data[0];

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

	ret = ele_call(&msg);
	if (ret)
		pr_err("%s: ret %d, core id %u, response 0x%x\n",
			__func__, ret, core_id, msg.data[0]);

	if (response)
		*response = msg.data[0];

	return ret;
}

struct ele_str {
	u8 id;
	const char *str;
};

static struct ele_str ele_ind[] = {
	{ .id = ELE_ROM_PING_FAILURE_IND, .str = "ELE_ROM_PING_FAILURE" },
	{ .id = ELE_FW_PING_FAILURE_IND, .str = "ELE_FW_PING_FAILURE" },
	{ .id = ELE_BAD_SIGNATURE_FAILURE_IND, .str = "ELE_BAD_SIGNATURE_FAILURE" },
	{ .id = ELE_BAD_HASH_FAILURE_IND, .str = "ELE_BAD_HASH_FAILURE" },
	{ .id = ELE_INVALID_LIFECYCLE_IND, .str = "ELE_INVALID_LIFECYCLE" },
	{ .id = ELE_PERMISSION_DENIED_FAILURE_IND, .str = "ELE_PERMISSION_DENIED_FAILURE" },
	{ .id = ELE_INVALID_MESSAGE_FAILURE_IND, .str = "ELE_INVALID_MESSAGE_FAILURE" },
	{ .id = ELE_BAD_VALUE_FAILURE_IND, .str = "ELE_BAD_VALUE_FAILURE" },
	{ .id = ELE_BAD_FUSE_ID_FAILURE_IND, .str = "ELE_BAD_FUSE_ID_FAILURE" },
	{ .id = ELE_BAD_CONTAINER_FAILURE_IND, .str = "ELE_BAD_CONTAINER_FAILURE" },
	{ .id = ELE_BAD_VERSION_FAILURE_IND, .str = "ELE_BAD_VERSION_FAILURE" },
	{ .id = ELE_INVALID_KEY_FAILURE_IND, .str = "ELE_INVALID_KEY_FAILURE" },
	{ .id = ELE_BAD_KEY_HASH_FAILURE_IND, .str = "ELE_BAD_KEY_HASH_FAILURE" },
	{ .id = ELE_NO_VALID_CONTAINER_FAILURE_IND, .str = "ELE_NO_VALID_CONTAINER_FAILURE" },
	{ .id = ELE_BAD_CERTIFICATE_FAILURE_IND, .str = "ELE_BAD_CERTIFICATE_FAILURE" },
	{ .id = ELE_BAD_UID_FAILURE_IND, .str = "ELE_BAD_UID_FAILURE" },
	{ .id = ELE_BAD_MONOTONIC_COUNTER_FAILURE_IND, .str = "ELE_BAD_MONOTONIC_COUNTER_FAILURE" },
	{ .id = ELE_MUST_SIGNED_FAILURE_IND, .str = "ELE_MUST_SIGNED_FAILURE" },
	{ .id = ELE_NO_AUTHENTICATION_FAILURE_IND, .str = "ELE_NO_AUTHENTICATION_FAILURE" },
	{ .id = ELE_BAD_SRK_SET_FAILURE_IND, .str = "ELE_BAD_SRK_SET_FAILURE" },
	{ .id = ELE_UNALIGNED_PAYLOAD_FAILURE_IND, .str = "ELE_UNALIGNED_PAYLOAD_FAILURE" },
	{ .id = ELE_WRONG_SIZE_FAILURE_IND, .str = "ELE_WRONG_SIZE_FAILURE" },
	{ .id = ELE_ENCRYPTION_FAILURE_IND, .str = "ELE_ENCRYPTION_FAILURE" },
	{ .id = ELE_DECRYPTION_FAILURE_IND, .str = "ELE_DECRYPTION_FAILURE" },
	{ .id = ELE_OTP_PROGFAIL_FAILURE_IND, .str = "ELE_OTP_PROGFAIL_FAILURE" },
	{ .id = ELE_OTP_LOCKED_FAILURE_IND, .str = "ELE_OTP_LOCKED_FAILURE" },
	{ .id = ELE_OTP_INVALID_IDX_FAILURE_IND, .str = "ELE_OTP_INVALID_IDX_FAILURE" },
	{ .id = ELE_TIME_OUT_FAILURE_IND, .str = "ELE_TIME_OUT_FAILURE" },
	{ .id = ELE_BAD_PAYLOAD_FAILURE_IND, .str = "ELE_BAD_PAYLOAD_FAILURE" },
	{ .id = ELE_WRONG_ADDRESS_FAILURE_IND, .str = "ELE_WRONG_ADDRESS_FAILURE" },
	{ .id = ELE_DMA_FAILURE_IND, .str = "ELE_DMA_FAILURE" },
	{ .id = ELE_DISABLED_FEATURE_FAILURE_IND, .str = "ELE_DISABLED_FEATURE_FAILURE" },
	{ .id = ELE_MUST_ATTEST_FAILURE_IND, .str = "ELE_MUST_ATTEST_FAILURE" },
	{ .id = ELE_RNG_NOT_STARTED_FAILURE_IND, .str = "ELE_RNG_NOT_STARTED_FAILURE" },
	{ .id = ELE_CRC_ERROR_IND, .str = "ELE_CRC_ERROR" },
	{ .id = ELE_AUTH_SKIPPED_OR_FAILED_FAILURE_IND, .str = "ELE_AUTH_SKIPPED_OR_FAILED_FAILURE" },
	{ .id = ELE_INCONSISTENT_PAR_FAILURE_IND, .str = "ELE_INCONSISTENT_PAR_FAILURE" },
	{ .id = ELE_RNG_INST_FAILURE_FAILURE_IND, .str = "ELE_RNG_INST_FAILURE_FAILURE" },
	{ .id = ELE_LOCKED_REG_FAILURE_IND, .str = "ELE_LOCKED_REG_FAILURE" },
	{ .id = ELE_BAD_ID_FAILURE_IND, .str = "ELE_BAD_ID_FAILURE" },
	{ .id = ELE_INVALID_OPERATION_FAILURE_IND, .str = "ELE_INVALID_OPERATION_FAILURE" },
	{ .id = ELE_NON_SECURE_STATE_FAILURE_IND, .str = "ELE_NON_SECURE_STATE_FAILURE" },
	{ .id = ELE_MSG_TRUNCATED_IND, .str = "ELE_MSG_TRUNCATED" },
	{ .id = ELE_BAD_IMAGE_NUM_FAILURE_IND, .str = "ELE_BAD_IMAGE_NUM_FAILURE" },
	{ .id = ELE_BAD_IMAGE_ADDR_FAILURE_IND, .str = "ELE_BAD_IMAGE_ADDR_FAILURE" },
	{ .id = ELE_BAD_IMAGE_PARAM_FAILURE_IND, .str = "ELE_BAD_IMAGE_PARAM_FAILURE" },
	{ .id = ELE_BAD_IMAGE_TYPE_FAILURE_IND, .str = "ELE_BAD_IMAGE_TYPE_FAILURE" },
	{ .id = ELE_CORRUPTED_SRK_FAILURE_IND, .str = "ELE_CORRUPTED_SRK_FAILURE" },
	{ .id = ELE_OUT_OF_MEMORY_IND, .str = "ELE_OUT_OF_MEMORY" },
	{ .id = ELE_CSTM_FAILURE_IND, .str = "ELE_CSTM_FAILURE" },
	{ .id = ELE_OLD_VERSION_FAILURE_IND, .str = "ELE_OLD_VERSION_FAILURE" },
	{ .id = ELE_WRONG_BOOT_MODE_FAILURE_IND, .str = "ELE_WRONG_BOOT_MODE_FAILURE" },
	{ .id = ELE_APC_ALREADY_ENABLED_FAILURE_IND, .str = "ELE_APC_ALREADY_ENABLED_FAILURE" },
	{ .id = ELE_RTC_ALREADY_ENABLED_FAILURE_IND, .str = "ELE_RTC_ALREADY_ENABLED_FAILURE" },
	{ .id = ELE_ABORT_IND, .str = "ELE_ABORT" },
};

static struct ele_str ele_ipc[] = {
	{ .id = ELE_IPC_MU_RTD, .str = "MU RTD" },
	{ .id = ELE_IPC_MU_APD, .str = "MU APD" },
};

static struct ele_str ele_command[] = {
	{ .id = ELE_PING_REQ, .str = "ELE_PING" },
	{ .id = ELE_FW_AUTH_REQ, .str = "ELE_FW_AUTH" },
	{ .id = ELE_RESTART_RST_TIMER_REQ, .str = "ELE_RESTART_RST_TIMER" },
	{ .id = ELE_DUMP_DEBUG_BUFFER_REQ, .str = "ELE_DUMP_DEBUG_BUFFER" },
	{ .id = ELE_OEM_CNTN_AUTH_REQ, .str = "ELE_OEM_CNTN_AUTH" },
	{ .id = ELE_VERIFY_IMAGE_REQ, .str = "ELE_VERIFY_IMAGE" },
	{ .id = ELE_RELEASE_CONTAINER_REQ, .str = "ELE_RELEASE_CONTAINER" },
	{ .id = ELE_WRITE_SECURE_FUSE_REQ, .str = "ELE_WRITE_SECURE_FUSE" },
	{ .id = ELE_FWD_LIFECYCLE_UP_REQ, .str = "ELE_FWD_LIFECYCLE_UP" },
	{ .id = ELE_READ_FUSE_REQ, .str = "ELE_READ_FUSE" },
	{ .id = ELE_GET_FW_VERSION_REQ, .str = "ELE_GET_FW_VERSION" },
	{ .id = ELE_RET_LIFECYCLE_UP_REQ, .str = "ELE_RET_LIFECYCLE_UP" },
	{ .id = ELE_GET_EVENTS_REQ, .str = "ELE_GET_EVENTS" },
	{ .id = ELE_ENABLE_PATCH_REQ, .str = "ELE_ENABLE_PATCH" },
	{ .id = ELE_RELEASE_RDC_REQ, .str = "ELE_RELEASE_RDC" },
	{ .id = ELE_GET_FW_STATUS_REQ, .str = "ELE_GET_FW_STATUS" },
	{ .id = ELE_ENABLE_OTFAD_REQ, .str = "ELE_ENABLE_OTFAD" },
	{ .id = ELE_RESET_REQ, .str = "ELE_RESET" },
	{ .id = ELE_UPDATE_OTP_CLKDIV_REQ, .str = "ELE_UPDATE_OTP_CLKDIV" },
	{ .id = ELE_POWER_DOWN_REQ, .str = "ELE_POWER_DOWN" },
	{ .id = ELE_ENABLE_APC_REQ, .str = "ELE_ENABLE_APC" },
	{ .id = ELE_ENABLE_RTC_REQ, .str = "ELE_ENABLE_RTC" },
	{ .id = ELE_DEEP_POWER_DOWN_REQ, .str = "ELE_DEEP_POWER_DOWN" },
	{ .id = ELE_STOP_RST_TIMER_REQ, .str = "ELE_STOP_RST_TIMER" },
	{ .id = ELE_WRITE_FUSE_REQ, .str = "ELE_WRITE_FUSE" },
	{ .id = ELE_RELEASE_CAAM_REQ, .str = "ELE_RELEASE_CAAM" },
	{ .id = ELE_RESET_A35_CTX_REQ, .str = "ELE_RESET_A35_CTX" },
	{ .id = ELE_MOVE_TO_UNSECURED_REQ, .str = "ELE_MOVE_TO_UNSECURED" },
	{ .id = ELE_GET_INFO_REQ, .str = "ELE_GET_INFO" },
	{ .id = ELE_ATTEST_REQ, .str = "ELE_ATTEST" },
	{ .id = ELE_RELEASE_PATCH_REQ, .str = "ELE_RELEASE_PATCH" },
	{ .id = ELE_OTP_SEQ_SWITH_REQ, .str = "ELE_OTP_SEQ_SWITH" },
};

static struct ele_str ele_status[] = {
	{ .id = ELE_SUCCESS_IND, .str = "ELE_SUCCESS" },
	{ .id = ELE_FAILURE_IND, .str = "ELE_FAILURE" },
};

static const struct ele_str *get_idx(struct ele_str *str, int size, int id)
{
	u32 i;

	for (i = 0; i < size; i++) {
		if (str[i].id == id)
			return &str[i];
	}

	return NULL;
}

#define ELE_EVENT_IPC		GENMASK(31, 24)
#define ELE_EVENT_COMMAND	GENMASK(23, 16)
#define ELE_EVENT_IND		GENMASK(15, 8)
#define ELE_EVENT_STATUS	GENMASK(7, 0)

static void display_event(u32 event)
{
	int ipc = FIELD_GET(ELE_EVENT_IPC, event);
	int command = FIELD_GET(ELE_EVENT_COMMAND, event);
	int ind = FIELD_GET(ELE_EVENT_IND, event);
	int status = FIELD_GET(ELE_EVENT_STATUS, event);
	const struct ele_str *ipc_str = get_idx(ARRAY_AND_SIZE(ele_ipc), ipc);
	const struct ele_str *command_str = get_idx(ARRAY_AND_SIZE(ele_command), command);
	const struct ele_str *ind_str = get_idx(ARRAY_AND_SIZE(ele_ind), ind);
	const struct ele_str *status_str = get_idx(ARRAY_AND_SIZE(ele_status), status);

	pr_info("Event 0x%08x:\n", event);
	pr_info("  IPC = %s (0x%02x)\n", ipc_str ? ipc_str->str : "INVALID", ipc);
	pr_info("  CMD = %s (0x%02x)\n", command_str ? command_str->str : "INVALID", command);
	pr_info("  IND = %s (0x%02x)\n", ind_str ? ind_str->str : "INVALID", ind);
	pr_info("  STA = %s (0x%02x)\n", status_str ? status_str->str : "INVALID", status);
}

#define AHAB_MAX_EVENTS 8

static int ahab_get_events(u32 *events)
{
	struct ele_msg msg;
	int ret, i = 0;
	u32 n_events;

	msg.version = ELE_VERSION;
	msg.tag = ELE_CMD_TAG;
	msg.size = 1;
	msg.command = ELE_GET_EVENTS_REQ;

	ret = imx9_s3mua_call(&msg);
	if (ret) {
		pr_err("%s: ret %d, response 0x%x\n", __func__, ret, msg.data[0]);

		return ret;
	}

	n_events = msg.data[1] & 0xffff;

	if (n_events > AHAB_MAX_EVENTS)
		n_events = AHAB_MAX_EVENTS;

	for (; i < n_events; i++)
		events[i] = msg.data[i + 2];

	return n_events;
}

unsigned int imx93_ahab_read_lifecycle(void)
{
	return readl(MX9_OCOTP_BASE_ADDR + 0x41c) & 0x3ff;
}

static const char *ele_life_cycle(u32 lc)
{
	switch (lc) {
	case ELE_LIFECYCLE_BLANK: return "BLANK";
	case ELE_LIFECYCLE_FAB: return "FAB";
	case ELE_LIFECYCLE_NXP_PROVISIONED: return "NXP Provisioned";
	case ELE_LIFECYCLE_OEM_OPEN: return "OEM Open";
	case ELE_LIFECYCLE_OEM_CLOSED: return "OEM closed";
	case ELE_LIFECYCLE_FIELD_RETURN_OEM: return "Field Return OEM";
	case ELE_LIFECYCLE_FIELD_RETURN_NXP: return "Field Return NXP";
	case ELE_LIFECYCLE_OEM_LOCKED: return "OEM Locked";
	case ELE_LIFECYCLE_BRICKED: return "BRICKED";
	default: return "Unknown";
	}
}

int ele_print_events(void)
{
	unsigned int lc;
	u32 events[AHAB_MAX_EVENTS];
	int i, ret;

	lc = imx93_ahab_read_lifecycle();
	pr_info("Current lifecycle: %s\n", ele_life_cycle(lc));

	ret = ahab_get_events(events);
	if (ret < 0)
		return ret;

	if (!ret) {
		pr_info("No Events Found!\n");
		return 0;
	}

	for (i = 0; i < ret; i++)
		display_event(events[i]);

	return 0;
}
