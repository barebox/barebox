/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */
#define pr_fmt(fmt) "HAB: " fmt

#include <common.h>
#include <fcntl.h>
#include <environment.h>
#include <libfile.h>
#include <mach/generic.h>
#include <hab.h>
#include <regmap.h>
#include <fs.h>
#include <mach/iim.h>
#include <mach/imx25-fusemap.h>
#include <mach/ocotp.h>
#include <mach/imx6-fusemap.h>

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

static int imx_hab_lockdown_device_iim(void)
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

static int imx_hab_write_srk_hash_ocotp(const u8 *__newsrk, unsigned flags)
{
	u32 *newsrk = (u32 *)__newsrk;
	int ret, i;

	for (i = 0; i < SRK_HASH_SIZE / sizeof(uint32_t); i++) {
		ret = imx_ocotp_write_field(OCOTP_SRK_HASH(i), newsrk[i]);
		if (ret < 0)
			return ret;
	}

	if (flags & IMX_SRK_HASH_WRITE_LOCK) {
		ret = imx_ocotp_write_field(OCOTP_SRK_LOCK, 1);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx_hab_permanent_write_enable_ocotp(int enable)
{
	return imx_ocotp_permanent_write(enable);
}

static int imx_hab_lockdown_device_ocotp(void)
{
	return imx_ocotp_write_field(OCOTP_SEC_CONFIG_1, 1);
}

static int imx_hab_device_locked_down_ocotp(void)
{
	int ret;
	unsigned int v;

	ret = imx_ocotp_read_field(OCOTP_SEC_CONFIG_1, &v);
	if (ret < 0)
		return ret;

	return v;
}

struct imx_hab_ops {
	int (*init)(void);
	int (*write_srk_hash)(const u8 *srk, unsigned flags);
	int (*read_srk_hash)(u8 *srk);
	int (*permanent_write_enable)(int enable);
	int (*lockdown_device)(void);
	int (*device_locked_down)(void);
};

static struct imx_hab_ops imx_hab_ops_iim = {
	.write_srk_hash = imx_hab_write_srk_hash_iim,
	.read_srk_hash =  imx_hab_read_srk_hash_iim,
	.lockdown_device = imx_hab_lockdown_device_iim,
	.device_locked_down = imx_hab_device_locked_down_iim,
	.permanent_write_enable = imx_hab_permanent_write_enable_iim,
};

static struct imx_hab_ops imx_hab_ops_ocotp = {
	.write_srk_hash = imx_hab_write_srk_hash_ocotp,
	.read_srk_hash =  imx_hab_read_srk_hash_ocotp,
	.lockdown_device = imx_hab_lockdown_device_ocotp,
	.device_locked_down = imx_hab_device_locked_down_ocotp,
	.permanent_write_enable = imx_hab_permanent_write_enable_ocotp,
};

static struct imx_hab_ops *imx_get_hab_ops(void)
{
	static struct imx_hab_ops *ops, *tmp;
	int ret;

	if (ops)
		return ops;

	if (IS_ENABLED(CONFIG_HABV3) && (cpu_is_mx25() || cpu_is_mx35()))
		tmp = &imx_hab_ops_iim;
	else if (IS_ENABLED(CONFIG_HABV4) && cpu_is_mx6())
		tmp = &imx_hab_ops_ocotp;
	else
		return NULL;

	if (tmp->init) {
		ret = tmp->init();
		if (ret)
			return NULL;
	}

	ops = tmp;

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
		pr_err("Cannot read current SRK hash: %s\n", strerror(-ret));
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
	u8 srk[SRK_HASH_SIZE];
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

	ret = ops->lockdown_device();

	if (flags & IMX_SRK_HASH_WRITE_PERMANENT)
		ops->permanent_write_enable(0);

	return ret;
}

int imx_hab_device_locked_down(void)
{
	struct imx_hab_ops *ops = imx_get_hab_ops();

	return ops->device_locked_down();
}
