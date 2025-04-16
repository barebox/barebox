// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt) "HAB: " fmt

#include <common.h>
#include <fcntl.h>
#include <environment.h>
#include <libfile.h>
#include <mach/imx/generic.h>
#include <hab.h>
#include <linux/regmap.h>
#include <fs.h>
#include <mach/imx/iim.h>
#include <mach/imx/imx25-fusemap.h>
#include <mach/imx/ocotp.h>
#include <mach/imx/imx6-fusemap.h>
#include <mach/imx/ele.h>

#include "hab.h"

bool imx_hab_srk_hash_valid(const void *buf)
{
	const u8 *srk = buf;
	int all_zero = 1, all_ff = 1;
	int i;

	for (i = 0; i < SRK_HASH_SIZE; i++) {
		if (srk[i] != 0x0)
			all_zero = 0;
		if (srk[i] != 0xff)
			all_ff = 0;
	}

	return !all_zero && !all_ff;
}

static int imx_hab_read_srk_hash_iim(u8 *srk)
{
	int ret, i;
	unsigned int val;

	ret = imx_iim_read_field(IMX25_IIM_SRK0_HASH_0, &val);
	if (ret < 0)
		return ret;
	srk[0] = val;

	for (i = 1; i < SRK_HASH_SIZE; i++) {
		ret = imx_iim_read_field(IMX25_IIM_SRK0_HASH_1_31(i), &val);
		if (ret < 0)
			return ret;
		srk[i] = val;
	}

	return 0;
}

static int imx_hab_write_srk_hash_iim(const u8 *srk, unsigned flags)
{
	int ret, i;

	ret = imx_iim_write_field(IMX25_IIM_SRK0_HASH_0, srk[0]);
	if (ret < 0)
		return ret;

	for (i = 1; i < SRK_HASH_SIZE; i++) {
		ret = imx_iim_write_field(IMX25_IIM_SRK0_HASH_1_31(i), srk[i]);
		if (ret < 0)
			return ret;
	}

	if (flags & IMX_SRK_HASH_WRITE_PERMANENT) {
		u8 verify[SRK_HASH_SIZE];

		setenv("iim.explicit_sense_enable", "1");
		ret = imx_hab_read_srk_hash_iim(verify);
		if (ret)
			return ret;
		setenv("iim.explicit_sense_enable", "0");

		if (memcmp(srk, verify, SRK_HASH_SIZE))
			return -EIO;
	}

	if (flags & IMX_SRK_HASH_WRITE_LOCK) {
		ret = imx_iim_write_field(IMX25_IIM_SRK0_LOCK96, 1);
		if (ret < 0)
			return ret;
		ret = imx_iim_write_field(IMX25_IIM_SRK0_LOCK160, 1);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx_hab_permanent_write_enable_iim(int enable)
{
	return imx_iim_permanent_write(enable);
}

static int imx_hab_lockdown_device_iim(unsigned flags)
{
	return imx_iim_write_field(IMX25_IIM_HAB_TYPE, 3);
}

static int imx_hab_device_locked_down_iim(void)
{
	int ret;
	unsigned int v;

	ret = imx_iim_read_field(IMX25_IIM_HAB_TYPE, &v);
	if (ret < 0)
		return ret;

	return v == 1 ? false : true;
}

static int imx_hab_read_srk_hash_ocotp(u8 *__srk)
{
	u32 *srk = (u32 *)__srk;
	int ret, i;

	for (i = 0; i < SRK_HASH_SIZE / sizeof(uint32_t); i++) {
		ret = imx_ocotp_read_field(OCOTP_SRK_HASH(i), &srk[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx_hab_write_srk_hash_ocotp(const u8 *__newsrk)
{
	u32 *newsrk = (u32 *)__newsrk;
	int ret, i;

	for (i = 0; i < SRK_HASH_SIZE / sizeof(uint32_t); i++) {
		ret = imx_ocotp_write_field(OCOTP_SRK_HASH(i), newsrk[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx6_hab_write_srk_hash_ocotp(const u8 *newsrk, unsigned flags)
{
	int ret;

	ret = imx_hab_write_srk_hash_ocotp(newsrk);
	if (ret)
		return ret;

	if (flags & IMX_SRK_HASH_WRITE_LOCK) {
		ret = imx_ocotp_write_field(OCOTP_SRK_LOCK, 1);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx8m_hab_write_srk_hash_ocotp(const u8 *newsrk, unsigned flags)
{
	int ret;

	ret = imx_hab_write_srk_hash_ocotp(newsrk);
	if (ret)
		return ret;

	if (flags & IMX_SRK_HASH_WRITE_LOCK) {
		ret = imx_ocotp_write_field(MX8M_OCOTP_SRK_LOCK, 1);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx_hab_permanent_write_enable_ocotp(int enable)
{
	return imx_ocotp_permanent_write(enable);
}

static int imx6_hab_lockdown_device_ocotp(unsigned flags)
{
	int ret;

	ret = imx_ocotp_write_field(OCOTP_DIR_BT_DIS, 1);
	if (ret < 0)
		return ret;

	return imx_ocotp_write_field(OCOTP_SEC_CONFIG_1, 1);
}

static int imx8m_hab_lockdown_device_ocotp(unsigned flags)
{
	int ret;

	ret = imx_ocotp_write_field(MX8M_OCOTP_SEC_CONFIG_1, 1);
	if (ret < 0)
		return ret;

	/* Lock upper 64bit of the 128bit UNIQUE_ID eFuse field on i.MX8MP */
	if (cpu_is_mx8mp())
		return imx_ocotp_write_field(MX8MP_OCOTP_GP5_LOCK, 0b11);

	/* Only i.MX8MQ requires fusing of DIR_BT_DIS */
	if (cpu_is_mx8mq())
		return imx_ocotp_write_field(MX8MQ_OCOTP_DIR_BT_DIS, 1);

	return 0;
}

static int imx6_hab_device_locked_down_ocotp(void)
{
	int ret;
	unsigned int v;

	ret = imx_ocotp_read_field(OCOTP_SEC_CONFIG_1, &v);
	if (ret < 0)
		return ret;

	return v;
}

static int imx8m_hab_device_locked_down_ocotp(void)
{
	int ret;
	unsigned int v;

	ret = imx_ocotp_read_field(MX8M_OCOTP_SEC_CONFIG_1, &v);
	if (ret < 0)
		return ret;

	return v;
}

static int imx8m_hab_revoke_key_ocotp(unsigned key_idx)
{
	int ret;

	/* Prohibit revocation of last possible key */
	if (key_idx >= 4)
		return -EINVAL;

	ret = imx_ocotp_srk_revoke_locked();
	if (ret < 0)
		return ret;

	/* Return -EINVAL in case the SRK_REVOKE write is locked */
	if (ret == 1)
		return -EINVAL;

	ret = imx_ocotp_write_field(MX8M_OCOTP_SRK_REVOKE, BIT(key_idx));

	return ret;
}

/*
 * The fuse pattern for i.MX8M Plus is 0x28001401, but bit 2 is already set from factory.
 * This means when field return is set, the fuse word value reads 0x28001405
 */
#define MX8MP_FIELD_RETURN_PATTERN	0x28001401

static int imx8m_hab_field_return_ocotp(void)
{
	int ret;

	ret = imx_ocotp_field_return_locked();
	if (ret < 0)
		return ret;

	/* Return -EINVAL in case the FIELD_RETURN write is locked */
	if (ret == 1)
		return -EINVAL;

	if (cpu_is_mx8mp())
		ret = imx_ocotp_write_field(MX8MP_OCOTP_FIELD_RETURN,
					    MX8MP_FIELD_RETURN_PATTERN);
	else
		ret = imx_ocotp_write_field(MX8M_OCOTP_FIELD_RETURN, 1);

	return ret;
}

struct imx_hab_ops {
	int (*write_srk_hash)(const u8 *srk, unsigned flags);
	int (*read_srk_hash)(u8 *srk);
	int (*permanent_write_enable)(int enable);
	int (*lockdown_device)(unsigned flags);
	int (*device_locked_down)(void);
	int (*print_status)(void);
	int (*revoke_key)(unsigned key_idx);
	int (*field_return)(void);
};

static struct imx_hab_ops imx_hab_ops_iim = {
	.write_srk_hash = imx_hab_write_srk_hash_iim,
	.read_srk_hash =  imx_hab_read_srk_hash_iim,
	.lockdown_device = imx_hab_lockdown_device_iim,
	.device_locked_down = imx_hab_device_locked_down_iim,
	.permanent_write_enable = imx_hab_permanent_write_enable_iim,
	.print_status = imx25_hab_print_status,
};

static struct imx_hab_ops imx6_hab_ops_ocotp = {
	.write_srk_hash = imx6_hab_write_srk_hash_ocotp,
	.read_srk_hash =  imx_hab_read_srk_hash_ocotp,
	.lockdown_device = imx6_hab_lockdown_device_ocotp,
	.device_locked_down = imx6_hab_device_locked_down_ocotp,
	.permanent_write_enable = imx_hab_permanent_write_enable_ocotp,
	.print_status = imx6_hab_print_status,
};

static struct imx_hab_ops imx8m_hab_ops_ocotp = {
	.write_srk_hash = imx8m_hab_write_srk_hash_ocotp,
	.read_srk_hash =  imx_hab_read_srk_hash_ocotp,
	.lockdown_device = imx8m_hab_lockdown_device_ocotp,
	.device_locked_down = imx8m_hab_device_locked_down_ocotp,
	.permanent_write_enable = imx_hab_permanent_write_enable_ocotp,
	.print_status = imx8m_hab_print_status,
	.revoke_key = imx8m_hab_revoke_key_ocotp,
	.field_return = imx8m_hab_field_return_ocotp,
};

static int imx_ahab_write_srk_hash(const u8 *__newsrk, unsigned flags)
{
	u32 *newsrk = (u32 *)__newsrk;
	u32 resp;
	int ret, i;

	if (!(flags & IMX_SRK_HASH_WRITE_PERMANENT)) {
		pr_err("Cannot write fuses temporarily\n");
		return -EPERM;
	}

	for (i = 0; i < SRK_HASH_SIZE / sizeof(u32); i++) {
		ret = ele_write_fuse(0x80 + i, newsrk[i], false, &resp);
		if (ret) {
			pr_err("Writing fuse index 0x%02x failed with %d, response 0x%08x\n",
			       i, ret, resp);
			return ret;
		}
	}

	return 0;
}

static int imx_ahab_read_srk_hash(u8 *__srk)
{
	u32 *srk = (u32 *)__srk;
	u32 resp;
	int ret, i;

	for (i = 0; i < SRK_HASH_SIZE / sizeof(u32); i++) {
		ret = ele_read_common_fuse(0x80 + i, &srk[i], &resp);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx_ahab_permanent_write_enable(int enable)
{
	return 0;
}

static int imx_ahab_lockdown_device(unsigned flags)
{
	unsigned int lc;
	int ret;

	if (!(flags & IMX_SRK_HASH_WRITE_PERMANENT)) {
		pr_err("Cannot write fuses temporarily\n");
		return -EPERM;
	}

	lc = imx93_ahab_read_lifecycle();
	if (lc == ELE_LIFECYCLE_OEM_CLOSED) {
		pr_info("already OEM closed\n");
		return 0;
	}

	if (lc != ELE_LIFECYCLE_OEM_OPEN) {
		pr_err("Current lifecycle is NOT OEM open, can't move to OEM closed\n");
		return -EPERM;
	}

	ret = ele_forward_lifecycle(0x8, NULL);
	if (ret) {
		pr_err("failed to forward lifecycle to OEM closed: %pe\n", ERR_PTR(ret));
		return ret;
	}

	pr_info("Change to OEM closed successfully\n");

	return 0;
}

static int imx_ahab_device_locked_down(void)
{
	return imx93_ahab_read_lifecycle() != ELE_LIFECYCLE_OEM_OPEN;
}

static int imx_ahab_print_status(void)
{
	int ret;

	ret = ele_print_events();
	if (ret)
		pr_err("Cannot read ELE events: %pe\n", ERR_PTR(ret));

	return ret;
}

static struct imx_hab_ops imx93_ahab_ops = {
	.write_srk_hash = imx_ahab_write_srk_hash,
	.read_srk_hash =  imx_ahab_read_srk_hash,
	.lockdown_device = imx_ahab_lockdown_device,
	.device_locked_down = imx_ahab_device_locked_down,
	.permanent_write_enable = imx_ahab_permanent_write_enable,
	.print_status = imx_ahab_print_status,
};

static struct imx_hab_ops *imx_get_hab_ops(void)
{
	static struct imx_hab_ops *ops;

	if (ops)
		return ops;

	if (IS_ENABLED(CONFIG_HABV3) && cpu_is_mx25())
		ops = &imx_hab_ops_iim;
	else if (IS_ENABLED(CONFIG_HABV4) && cpu_is_mx6())
		ops = &imx6_hab_ops_ocotp;
	else if (IS_ENABLED(CONFIG_HABV4) && cpu_is_mx8m())
		ops = &imx8m_hab_ops_ocotp;
	else if (IS_ENABLED(CONFIG_AHAB) && cpu_is_mx93())
		ops = &imx93_ahab_ops;
	else
		return NULL;

	return ops;
}

int imx_hab_read_srk_hash(void *buf)
{
	struct imx_hab_ops *ops = imx_get_hab_ops();

	if (!ops)
		return -ENOSYS;

	return ops->read_srk_hash(buf);
}

int imx_hab_write_srk_hash(const void *buf, unsigned flags)
{
	u8 cursrk[SRK_HASH_SIZE];
	int ret;
	struct imx_hab_ops *ops = imx_get_hab_ops();

	if (!ops)
		return -ENOSYS;

	ret = ops->read_srk_hash(cursrk);
	if (ret) {
		pr_err("Cannot read current SRK hash: %pe\n", ERR_PTR(ret));
		return ret;
	}

	if (imx_hab_srk_hash_valid(cursrk)) {
		char *str = "Current SRK hash is valid";

		if (flags & IMX_SRK_HASH_FORCE) {
			pr_warn("%s, ignoring\n", str);
		} else {
			pr_err("%s, refusing to burn again\n", str);
			return -EEXIST;
		}
	}

	if (flags & IMX_SRK_HASH_WRITE_PERMANENT) {
		ret = ops->permanent_write_enable(1);
		if (ret)
			return ret;
	}

	ret = ops->write_srk_hash(buf, flags);

	if (flags & IMX_SRK_HASH_WRITE_PERMANENT)
		ops->permanent_write_enable(0);

	return ret;
}

int imx_hab_write_srk_hash_file(const char *filename, unsigned flags)
{
	int ret;
	size_t size;
	void *srk;

	ret = read_file_2(filename, &size, &srk, FILESIZE_MAX);
	if (ret)
		return ret;

	if (size != SRK_HASH_SIZE) {
		pr_err("File has wrong size, must be %d bytes\n", SRK_HASH_SIZE);
		return -EINVAL;
	}

	ret = imx_hab_write_srk_hash(srk, flags);

	free(srk);

	return ret;
}

int imx_hab_write_srk_hash_hex(const char *srkhash, unsigned flags)
{
	int ret;
	u8 srk[SRK_HASH_SIZE];

	if (strlen(srkhash) != SRK_HASH_SIZE * 2) {
		pr_err("Invalid srk hash %s\n", srkhash);
		return -EINVAL;
	}

	ret = hex2bin(srk, srkhash, SRK_HASH_SIZE);
	if (ret < 0) {
		pr_err("Invalid srk hash %s\n", srkhash);
		return -EINVAL;
	}

	return imx_hab_write_srk_hash(srk, flags);
}

int imx_hab_lockdown_device(unsigned flags)
{
	struct imx_hab_ops *ops = imx_get_hab_ops();
	u8 srk[SRK_HASH_SIZE] = {};
	int ret;

	ret = imx_hab_read_srk_hash(srk);
	if (ret)
		return ret;

	if (!imx_hab_srk_hash_valid(srk)) {
		pr_err("No SRK hash burnt into fuses. Refusing to lock device\n");
		return -EINVAL;
	}

	if (!ops)
		return -ENOSYS;

	if (flags & IMX_SRK_HASH_WRITE_PERMANENT) {
		ret = ops->permanent_write_enable(1);
		if (ret)
			return ret;
	}

	ret = ops->lockdown_device(flags);

	if (flags & IMX_SRK_HASH_WRITE_PERMANENT)
		ops->permanent_write_enable(0);

	return ret;
}

int imx_hab_device_locked_down(void)
{
	struct imx_hab_ops *ops = imx_get_hab_ops();

	if (!ops)
		return -ENOSYS;

	return ops->device_locked_down();
}

int imx_hab_print_status(void)
{
	struct imx_hab_ops *ops = imx_get_hab_ops();

	if (!ops)
		return -ENOSYS;

	return ops->print_status();
}

static int init_imx_hab_print_status(void)
{
	imx_hab_print_status();

	return 0;
}
postmmu_initcall(init_imx_hab_print_status);

int imx_hab_revoke_key(unsigned key_idx, bool permanent)
{
	struct imx_hab_ops *ops = imx_get_hab_ops();
	int ret;

	if (!ops || !ops->revoke_key)
		return -ENOSYS;

	if (permanent) {
		ret = ops->permanent_write_enable(1);
		if (ret)
			return ret;
	}

	ret = ops->revoke_key(key_idx);

	if (permanent)
		ops->permanent_write_enable(0);

	return ret;
}

int imx_hab_field_return(bool permanent)
{
	struct imx_hab_ops *ops = imx_get_hab_ops();
	int ret;

	if (!ops || !ops->field_return)
		return -ENOSYS;

	if (permanent) {
		ret = ops->permanent_write_enable(1);
		if (ret)
			return ret;
	}

	ret = ops->field_return();

	if (permanent)
		ops->permanent_write_enable(0);

	return ret;
}
