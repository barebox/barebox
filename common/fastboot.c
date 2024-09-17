// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * Copyright 2014 Sascha Hauer <s.hauer@pengutronix.de>
 * Ported to barebox
 *
 * Copyright 2020 Edmund Henniges <eh@emlix.com>
 * Copyright 2020 Daniel Glöckner <dg@emlix.com>
 * Split off of generic parts
 */

#define pr_fmt(fmt) "fastboot: " fmt

#include <common.h>
#include <command.h>
#include <ioctl.h>
#include <bbu.h>
#include <bootm.h>
#include <fs.h>
#include <init.h>
#include <libfile.h>
#include <ubiformat.h>
#include <unistd.h>
#include <magicvar.h>
#include <linux/log2.h>
#include <linux/sizes.h>
#include <memory.h>
#include <progress.h>
#include <environment.h>
#include <globalvar.h>
#include <restart.h>
#include <console_countdown.h>
#include <image-sparse.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/mtd/mtd.h>
#include <fastboot.h>
#include <system-partitions.h>

#define FASTBOOT_VERSION		"0.4"

static unsigned int fastboot_max_download_size;
static int fastboot_bbu;
static char *fastboot_partitions;

struct fb_variable {
	char *name;
	char *value;
	struct list_head list;
};

static void fb_setvar(struct fb_variable *var, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	var->value = bvasprintf(fmt, ap);
	va_end(ap);
}

static struct fb_variable *fb_addvar(struct fastboot *fb, struct list_head *list, const char *fmt, ...)
{
	struct fb_variable *var = xzalloc(sizeof(*var));
	va_list ap;

	va_start(ap, fmt);
	var->name = bvasprintf(fmt, ap);
	va_end(ap);

	list_add_tail(&var->list, list);

	return var;
}

static int fastboot_add_partition_variables(struct fastboot *fb, struct list_head *list,
		struct file_list_entry *fentry)
{
	struct stat s;
	size_t size = 0;
	int fd, ret;
	struct mtd_info_user mtdinfo;
	char *type = NULL;
	struct fb_variable *var;

	ret = stat(fentry->filename, &s);
	if (ret) {
		device_detect_by_name(devpath_to_name(fentry->filename));
		ret = stat(fentry->filename, &s);
	}

	if (ret) {
		if (fentry->flags & FILE_LIST_FLAG_OPTIONAL) {
			pr_info("skipping unavailable optional partition %s for fastboot gadget\n",
				fentry->filename);
			ret = 0;
			type = "unavailable";
			goto out;
		}

		if (fentry->flags & FILE_LIST_FLAG_CREATE) {
			ret = 0;
			type = "file";
			goto out;
		}

		goto out;
	}

	fd = open(fentry->filename, O_RDWR);
	if (fd < 0) {
		ret = -EINVAL;
		goto out;
	}

	size = s.st_size;

	ret = ioctl(fd, MEMGETINFO, &mtdinfo);

	close(fd);

	if (!ret) {
		switch (mtdinfo.type) {
		case MTD_NANDFLASH:
			type = "NAND-flash";
			break;
		case MTD_NORFLASH:
			type = "NOR-flash";
			break;
		case MTD_UBIVOLUME:
			type = "UBI";
			break;
		default:
			type = "flash";
			break;
		}

		goto out;
	}

	type = "basic";
	ret = 0;

out:
	if (ret)
		return ret;

	var = fb_addvar(fb, list, "partition-size:%s", fentry->name);
	fb_setvar(var, "%08zx", size);
	var = fb_addvar(fb, list, "partition-type:%s", fentry->name);
	fb_setvar(var, "%s", type);

	return ret;
}

int fastboot_generic_init(struct fastboot *fb, bool export_bbu)
{
	struct fb_variable *var;

	INIT_LIST_HEAD(&fb->variables);

	var = fb_addvar(fb, &fb->variables, "version");
	fb_setvar(var, "0.4");
	var = fb_addvar(fb, &fb->variables, "bootloader-version");
	fb_setvar(var, release_string);
	if (IS_ENABLED(CONFIG_FASTBOOT_SPARSE)) {
		var = fb_addvar(fb, &fb->variables, "max-download-size");
		fb_setvar(var, "%u", fastboot_max_download_size);
	}

	fb->tempname = make_temp("fastboot");
	if (!fb->tempname)
		return -ENOMEM;

	if (!fb->files)
		fb->files = file_list_new();
	if (export_bbu)
		bbu_append_handlers_to_file_list(fb->files);

	return 0;
}

static void fastboot_free_variables(struct list_head *list)
{
	struct fb_variable *var, *tmp;

	list_for_each_entry_safe(var, tmp, list, list) {
		free(var->name);
		free(var->value);
		list_del(&var->list);
		free(var);
	}
}

void fastboot_generic_free(struct fastboot *fb)
{
	fastboot_free_variables(&fb->variables);

	free(fb->tempname);

	fb->active = false;
}

static struct fastboot *g_fb;

void fastboot_generic_close(struct fastboot *fb)
{
	if (g_fb == fb)
		g_fb = NULL;
}

/*
 * A "oem exec bootm" or similar commands will stop barebox. Tell the
 * fastboot command on the other side so that it doesn't run into a
 * timeout.
 */
static void fastboot_shutdown(void)
{
	struct fastboot *fb = g_fb;

	if (!fb || !fb->active)
		return;

	fastboot_tx_print(fb, FASTBOOT_MSG_INFO, "barebox shutting down");
	fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
}

early_exitcall(fastboot_shutdown);

static char *fastboot_msg[] = {
	[FASTBOOT_MSG_OKAY] = "OKAY",
	[FASTBOOT_MSG_FAIL] = "FAIL",
	[FASTBOOT_MSG_INFO] = "INFO",
	[FASTBOOT_MSG_DATA] = "DATA",
	[FASTBOOT_MSG_NONE] = "",
};

int fastboot_tx_print(struct fastboot *fb, enum fastboot_msg_type type,
		      const char *fmt, ...)
{
	struct va_format vaf;
	char buf[64];
	va_list ap;
	int n;
	const char *msg = fastboot_msg[type];

	va_start(ap, fmt);
	vaf.fmt = fmt;
	vaf.va = &ap;

	n = snprintf(buf, 64, "%s%pV", msg, &vaf);

	switch (type) {
	case FASTBOOT_MSG_OKAY:
		fb->active = false;
		break;
	case FASTBOOT_MSG_FAIL:
		fb->active = false;
		pr_err("%pV\n", &vaf);
		break;
	case FASTBOOT_MSG_INFO:
		pr_info("%pV\n", &vaf);
		break;
	case FASTBOOT_MSG_NONE:
	case FASTBOOT_MSG_DATA:
		break;
	}

	va_end(ap);

	if (n > 64)
		n = 64;

	return fb->write(fb, buf, n);
}

static void cb_reboot(struct fastboot *fb, const char *cmd)
{
	fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
	restart_machine();
}

static void cb_getvar(struct fastboot *fb, const char *cmd)
{
	struct fb_variable *var;
	LIST_HEAD(partition_list);
	struct file_list_entry *fentry;

	file_list_for_each_entry(fb->files, fentry) {
		int ret;

		ret = fastboot_add_partition_variables(fb, &partition_list, fentry);
		if (ret) {
			pr_warn("Failed to add partition variables: %pe", ERR_PTR(ret));
			return;
		}
	}

	pr_debug("getvar: \"%s\"\n", cmd);

	if (!strcmp(cmd, "all")) {
		list_for_each_entry(var, &fb->variables, list)
			fastboot_tx_print(fb, FASTBOOT_MSG_INFO, "%s: %s",
					  var->name, var->value);

		list_for_each_entry(var, &partition_list, list)
			fastboot_tx_print(fb, FASTBOOT_MSG_INFO, "%s: %s",
					  var->name, var->value);

		fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
		goto out;
	}

	list_for_each_entry(var, &fb->variables, list) {
		if (!strcmp(cmd, var->name)) {
			fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, var->value);
			goto out;
		}
	}

	list_for_each_entry(var, &partition_list, list) {
		if (!strcmp(cmd, var->name)) {
			fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, var->value);
			goto out;
		}
	}

	fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
out:
	fastboot_free_variables(&partition_list);
}

int fastboot_handle_download_data(struct fastboot *fb, const void *buffer,
				  unsigned int len)
{
	int ret;

	ret = write(fb->download_fd, buffer, len);
	if (ret < 0)
		return ret;

	fb->download_bytes += len;
	show_progress(fb->download_bytes);
	return 0;
}

void fastboot_download_finished(struct fastboot *fb)
{
	close(fb->download_fd);
	fb->download_fd = 0;

	printf("\n");

	fastboot_tx_print(fb, FASTBOOT_MSG_INFO, "Downloading %d bytes finished",
			  fb->download_bytes);

	fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
}

void fastboot_abort(struct fastboot *fb)
{
	if (fb->download_fd > 0) {
		close(fb->download_fd);
		fb->download_fd = 0;
	}

	fb->active = false;

	unlink(fb->tempname);
}

static void cb_download(struct fastboot *fb, const char *cmd)
{
	fb->download_size = simple_strtoul(cmd, NULL, 16);
	fb->download_bytes = 0;

	fastboot_tx_print(fb, FASTBOOT_MSG_INFO, "Downloading %d bytes...",
			  fb->download_size);

	init_progression_bar(fb->download_size);

	if (fb->download_fd > 0) {
		pr_err("%s called and %s is still opened\n", __func__, fb->tempname);
		close(fb->download_fd);
	}

	fb->download_fd = open(fb->tempname, O_WRONLY | O_CREAT | O_TRUNC);
	if (fb->download_fd < 0) {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, "internal error");
			return;
	}

	if (!fb->download_size)
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
					  "data invalid size");
	else
		fb->start_download(fb);
}

void fastboot_start_download_generic(struct fastboot *fb)
{
	fastboot_tx_print(fb, FASTBOOT_MSG_DATA, "%08x", fb->download_size);
}

static void __maybe_unused cb_boot(struct fastboot *fb, const char *opt)
{
	int ret;
	struct bootm_data data = {
		.initrd_address = UIMAGE_INVALID_ADDRESS,
		.os_address = UIMAGE_SOME_ADDRESS,
	};

	fastboot_tx_print(fb, FASTBOOT_MSG_INFO, "Booting kernel..\n");

	globalvar_set_match("linux.bootargs.dyn.", "");
	globalvar_set("bootm.image", "");

	data.os_file = fb->tempname;

	ret = bootm_boot(&data);

	if (ret)
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, "Booting failed: %s",
				   strerror(-ret));
	else
		fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
}

static struct mtd_info *get_mtd(struct fastboot *fb, const char *filename)
{
	int fd, ret;
	struct mtd_info_user meminfo;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return ERR_PTR(-errno);

	ret = ioctl(fd, MEMGETINFO, &meminfo);

	close(fd);

	if (ret)
		return ERR_PTR(ret);

	return meminfo.mtd;
}

static int do_ubiformat(struct fastboot *fb, struct mtd_info *mtd,
			const char *file, size_t len)
{
	struct ubiformat_args args = {
		.yes = 1,
		.image = file,
		.image_size = len,
	};

	if (!file)
		args.novtbl = 1;

	if (!IS_ENABLED(CONFIG_UBIFORMAT)) {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
					  "ubiformat is not available");
		return -ENODEV;
	}

	return ubiformat(mtd, &args);
}


static int check_ubi(struct fastboot *fb, struct file_list_entry *fentry,
		     enum filetype filetype)
{
	struct mtd_info *mtd;

	mtd = get_mtd(fb, fentry->filename);

	/*
	 * Issue a warning when we are about to write a UBI image to a MTD device
	 * and the FILE_LIST_FLAG_UBI is not given as this means we loose all
	 * erase counters.
	 */
	if (!IS_ERR(mtd) && filetype == filetype_ubi &&
	    !(fentry->flags & FILE_LIST_FLAG_UBI)) {
		fastboot_tx_print(fb, FASTBOOT_MSG_INFO,
				  "writing UBI image to MTD device, "
				  "add the 'u' ");
		fastboot_tx_print(fb, FASTBOOT_MSG_INFO,
				  "flag to the partition description");
		return 0;
	}

	if (!(fentry->flags & FILE_LIST_FLAG_UBI))
		return 0;

	if (!IS_ENABLED(CONFIG_UBIFORMAT)) {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
				  "ubiformat not available");
		return -ENOSYS;
	}

	if (IS_ERR(mtd)) {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
				  "UBI flag given on non-MTD device");
		return -EINVAL;
	}

	if (filetype == filetype_ubi) {
		fastboot_tx_print(fb, FASTBOOT_MSG_INFO,
				  "This is a UBI image...");
		return 1;
	} else {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
				  "This is no UBI image but %s",
				 file_type_to_string(filetype));
		return -EINVAL;
	}
}

static int fastboot_handle_sparse(struct fastboot *fb,
				  struct file_list_entry *fentry)
{
	struct sparse_image_ctx *sparse;
	void *buf = NULL;
	int ret, fd;
	unsigned int flags = O_RDWR;
	int bufsiz = SZ_128K;
	struct stat s;
	struct mtd_info *mtd = NULL;

	ret = stat(fentry->filename, &s);
	if (ret) {
		if (fentry->flags & FILE_LIST_FLAG_CREATE)
			flags |= O_CREAT;
		else
			return ret;
	}

	fd = open(fentry->filename, flags);
	if (fd < 0)
		return -errno;

	ret = fstat(fd, &s);
	if (ret)
		goto out_close_fd;

	sparse = sparse_image_open(fb->tempname);
	if (IS_ERR(sparse)) {
		pr_err("Cannot open sparse image\n");
		ret = PTR_ERR(sparse);
		goto out_close_fd;
	}

	if (S_ISREG(s.st_mode)) {
		ret = ftruncate(fd, sparse_image_size(sparse));
		if (ret)
			goto out;
	}

	buf = malloc(bufsiz);
	if (!buf) {
		ret = -ENOMEM;
		goto out;
	}

	if (fentry->flags & FILE_LIST_FLAG_UBI) {
		mtd = get_mtd(fb, fentry->filename);
		if (IS_ERR(mtd)) {
			ret = PTR_ERR(mtd);
			goto out;
		}
	}

	while (1) {
		size_t retlen;
		loff_t pos;

		ret = sparse_image_read(sparse, buf, &pos, bufsiz, &retlen);
		if (ret)
			goto out;
		if (!retlen)
			break;

		if (pos == 0) {
			ret = check_ubi(fb, fentry, file_detect_type(buf, retlen));
			if (ret < 0)
				goto out;
		}

		if (fentry->flags & FILE_LIST_FLAG_UBI) {
			if (!IS_ENABLED(CONFIG_UBIFORMAT)) {
				ret = -ENOSYS;
				goto out;
			}

			if (pos == 0) {
				ret = do_ubiformat(fb, mtd, NULL, 0);
				if (ret)
					goto out;
			}

			ret = ubiformat_write(mtd, buf, retlen, pos);
			if (ret)
				goto out;
		} else {
			discard_range(fd, retlen, pos);

			pos = lseek(fd, pos, SEEK_SET);
			if (pos == -1) {
				ret = errno == EINVAL ? -ENOSPC : -errno;
				goto out;
			}

			ret = write_full(fd, buf, retlen);
			if (ret < 0)
				goto out;
		}
	}

	ret = 0;

out:
	free(buf);
	sparse_image_close(sparse);
out_close_fd:
	close(fd);

	return ret;
}

static void cb_flash(struct fastboot *fb, const char *cmd)
{
	struct file_list_entry *fentry;
	int ret;
	const char *filename = NULL;
	enum filetype filetype;

	ret = file_name_detect_type(fb->tempname, &filetype);
	if (ret) {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, "internal error");
		goto out;
	}

	fastboot_tx_print(fb, FASTBOOT_MSG_INFO, "Copying file to %s...",
			  cmd);

	fentry = file_list_entry_by_name(fb->files, cmd);

	if (!fentry) {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, "No such partition: %s",
				  cmd);
		ret = -ENOENT;
		goto out;
	}

	/* Check if board-code registered a vendor-specific handler */
	if (fb->cmd_flash) {
		ret = fb->cmd_flash(fb, fentry, fb->tempname, fb->download_size);
		if (ret != FASTBOOT_CMD_FALLTHROUGH)
			goto out;
	}

	filename = fentry->filename;

	if (filetype == filetype_android_sparse) {
		if (!IS_ENABLED(CONFIG_FASTBOOT_SPARSE)) {
			fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
					  "sparse image not supported");
			ret = -EOPNOTSUPP;
			goto out;
		}

		ret = fastboot_handle_sparse(fb, fentry);
		if (ret)
			fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
					  "writing sparse image: %s",
					  strerror(-ret));

		goto out;
	}

	ret = check_ubi(fb, fentry, filetype);
	if (ret < 0)
		goto out;

	if (ret > 0) {
		struct mtd_info *mtd;

		mtd = get_mtd(fb, fentry->filename);

		ret = do_ubiformat(fb, mtd, fb->tempname, fb->download_size);
		if (ret) {
			fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
					  "write partition: %s",
					  strerror(-ret));
			goto out;
		}

		goto out;
	}

	if (IS_ENABLED(CONFIG_BAREBOX_UPDATE) &&
	    (filetype_is_barebox_image(filetype) || strstarts(fentry->name, "bbu-"))) {
		void *buf;
		struct bbu_handler *handler;
		struct bbu_data data = {
			.devicefile = filename,
			.flags = BBU_FLAG_YES,
		};

		handler = bbu_find_handler_by_device(data.devicefile);
		if (!handler)
			goto copy;

		fastboot_tx_print(fb, FASTBOOT_MSG_INFO,
				  "This is a barebox image...");

		ret = read_file_2(fb->tempname, &data.len, &buf,
				fb->download_size);
		if (ret) {
			fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
					  "reading barebox");
			goto out;
		}

		data.image = buf;
		data.imagefile = fb->tempname;

		ret = barebox_update(&data, handler);

		if (ret)
			fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
				  "update barebox: %s", strerror(-ret));

		free(buf);

		goto out;
	}

copy:
	ret = copy_file(fb->tempname, filename, 1);
	if (ret)
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
				  "write partition: %s", strerror(-ret));

out:
	if (!ret)
		fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");

	unlink(fb->tempname);
}

static void cb_erase(struct fastboot *fb, const char *cmd)
{
	struct file_list_entry *fentry;
	int ret;
	const char *filename = NULL;
	int fd;

	fastboot_tx_print(fb, FASTBOOT_MSG_INFO, "Erasing %s...", cmd);

	file_list_for_each_entry(fb->files, fentry) {
		if (!strcmp(cmd, fentry->name)) {
			filename = fentry->filename;
			break;
		}
	}

	if (!filename) {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
				  "No such partition: %s", cmd);
		return;
	}

	fd = open(filename, O_RDWR);
	if (fd < 0)
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, strerror(-fd));

	ret = erase(fd, ERASE_SIZE_ALL, 0, ERASE_TO_CLEAR);

	close(fd);

	if (ret)
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
				  "cannot erase partition %s: %s",
				  filename, strerror(-ret));
	else
		fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
}

static bool fastboot_cmd_should_abort(const char *cmdbuf)
{
	return !strstarts(cmdbuf, "getvar:");
}

struct cmd_dispatch_info {
	char *cmd;
	void (*cb)(struct fastboot *fb, const char *opt);
};

static void fb_run_command(struct fastboot *fb, const char *cmdbuf,
		const struct cmd_dispatch_info *cmds, int num_commands)
{
	const struct cmd_dispatch_info *cmd;
	int i;

	if (fastboot_cmd_should_abort(cmdbuf))
		console_countdown_abort("fastboot");

	for (i = 0; i < num_commands; i++) {
		size_t cmdlen;

		cmd = &cmds[i];

		cmdlen = str_has_prefix(cmdbuf, cmd->cmd);
		if (cmdlen) {
			cmd->cb(fb, cmdbuf + cmdlen);

			return;
		}
	}

	fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, "unknown command %s",
			  cmdbuf);
}

static void cb_oem_getenv(struct fastboot *fb, const char *cmd)
{
	const char *value;

	pr_debug("%s: \"%s\"\n", __func__, cmd);

	value = getenv(cmd);

	fastboot_tx_print(fb, FASTBOOT_MSG_INFO, value ? value : "");
	fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
}

static void cb_oem_setenv(struct fastboot *fb, const char *cmd)
{
	char *var = xstrdup(cmd);
	char *value;
	int ret;

	pr_debug("%s: \"%s\"\n", __func__, cmd);

	value = parse_assignment(var);
	if (!value) {
		ret = -EINVAL;
		goto out;
	}

	ret = setenv(var, value);
	if (ret)
		goto out;

	fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
out:
	free(var);

	if (ret)
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, strerror(-ret));
}

static void cb_oem_exec(struct fastboot *fb, const char *cmd)
{
	int ret;

	if (!IS_ENABLED(CONFIG_COMMAND_SUPPORT)) {
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL,
				  "no command support available");
		return;
	}

	ret = run_command(cmd);
	if (ret < 0)
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, strerror(-ret));
	else if (ret > 0)
		fastboot_tx_print(fb, FASTBOOT_MSG_FAIL, "");
	else
		fastboot_tx_print(fb, FASTBOOT_MSG_OKAY, "");
}

static const struct cmd_dispatch_info cmd_oem_dispatch_info[] = {
	{
		.cmd = "getenv ",
		.cb = cb_oem_getenv,
	}, {
		.cmd = "setenv ",
		.cb = cb_oem_setenv,
	}, {
		.cmd = "exec ",
		.cb = cb_oem_exec,
	},
};

static void __maybe_unused cb_oem(struct fastboot *fb, const char *cmd)
{
	pr_debug("%s: \"%s\"\n", __func__, cmd);

	fb_run_command(fb, cmd, cmd_oem_dispatch_info, ARRAY_SIZE(cmd_oem_dispatch_info));
}

static const struct cmd_dispatch_info cmd_dispatch_info[] = {
	{
		.cmd = "reboot",
		.cb = cb_reboot,
	}, {
		.cmd = "getvar:",
		.cb = cb_getvar,
	}, {
		.cmd = "download:",
		.cb = cb_download,
#if defined(CONFIG_BOOTM)
	}, {
		.cmd = "boot",
		.cb = cb_boot,
#endif
	}, {
		.cmd = "flash:",
		.cb = cb_flash,
	}, {
		.cmd = "erase:",
		.cb = cb_erase,
#if defined(CONFIG_FASTBOOT_CMD_OEM)
	}, {
		.cmd = "oem ",
		.cb = cb_oem,
#endif
	},
};

void fastboot_exec_cmd(struct fastboot *fb, const char *cmdbuf)
{
	int ret;

	g_fb = fb;
	fb->active = true;

	/* Check if board-code registered a vendor-specific handler */
	if (fb->cmd_exec) {
		ret = fb->cmd_exec(fb, cmdbuf);
		if (ret != FASTBOOT_CMD_FALLTHROUGH)
			return;
	}

	fb_run_command(fb, cmdbuf, cmd_dispatch_info,
				ARRAY_SIZE(cmd_dispatch_info));
}

bool get_fastboot_bbu(void)
{
	return fastboot_bbu;
}

void set_fastboot_bbu(unsigned int enable)
{
	fastboot_bbu = enable;
}

struct file_list *get_fastboot_partitions(void)
{
	if (fastboot_partitions && *fastboot_partitions)
		return file_list_parse_null(fastboot_partitions);
	return system_partitions_get_null();
}

static int fastboot_globalvars_init(void)
{
	if (IS_ENABLED(CONFIG_FASTBOOT_SPARSE)) {
		fastboot_max_download_size
			= roundup_pow_of_two(clamp_t(ulong, mem_malloc_size() / 8,
						     SZ_8M, SZ_128M));
		globalvar_add_simple_int("fastboot.max_download_size",
				 &fastboot_max_download_size, "%u");
	}

	globalvar_add_simple_bool("fastboot.bbu", &fastboot_bbu);
	globalvar_add_simple_string("fastboot.partitions",
				    &fastboot_partitions);

	globalvar_alias_deprecated("usbgadget.fastboot_function",
				   "fastboot.partitions");
	globalvar_alias_deprecated("usbgadget.fastboot_bbu",
				   "fastboot.bbu");
	globalvar_alias_deprecated("usbgadget.fastboot_max_download_size",
				   "fastboot.max_download_size");

	return 0;
}

device_initcall(fastboot_globalvars_init);

BAREBOX_MAGICVAR(global.fastboot.max_download_size,
		 "Fastboot maximum download size");
BAREBOX_MAGICVAR(global.fastboot.partitions,
		       "Partitions exported for update via fastboot");
BAREBOX_MAGICVAR(global.fastboot.bbu,
		       "Export barebox update handlers via fastboot");
