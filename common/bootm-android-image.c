// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "android-image: " fmt

#include <bootm.h>
#include <aimage.h>
#include <init.h>
#include <fcntl.h>
#include <libfile.h>
#include <linux/fs.h>
#include <linux/sizes.h>
#include <linux/align.h>
#include <unistd.h>
#include <filetype.h>

static char *aimage_copy_component(int from, size_t ofs, size_t size)
{
	char *path;
	int to, ret;
	loff_t pos;

	path = make_temp("aimage");
	if (!path) {
		ret = -ENOMEM;
		goto err;
	}

	pos = lseek(from, ofs, SEEK_SET);
	if (pos < 0) {
		ret = -errno;
		goto err;
	}

	to = open(path, O_CREAT | O_WRONLY);
	if (to < 0) {
		ret = to;
		goto err;
	}

	ret = copy_fd(from, to, size);

	close(to);

	if (!ret)
		return path;
err:
	free(path);

	return ERR_PTR(ret);
}

static int do_bootm_aimage(struct image_data *img_data)
{
	struct bootm_data bootm_data = {
		.oftree_file = img_data->oftree_file,
		.initrd_file = img_data->initrd_file,
		.tee_file = img_data->tee_file,
		.verbose = img_data->verbose,
		.verify = img_data->verify,
		.force = img_data->force,
		.dryrun = img_data->dryrun,
		.initrd_address = img_data->initrd_address,
		.os_address = img_data->os_address,
		.os_entry = img_data->os_entry,
	};
	struct android_header *hdr;
	int fd, ret;
	char *kernel = NULL, *initrd = NULL;
	size_t ofs;

	hdr = xmalloc(sizeof(*hdr));

	fd = open(img_data->os_file, O_RDONLY);
	if (fd < 0) {
		ret = fd;
		goto err;
	}

	ret = read_full(fd, hdr, sizeof(*hdr));
	if (ret < 0)
		goto err_close;

	if (ret < sizeof(*hdr)) {
		ret = -EINVAL;
		goto err_close;
	}

	if (file_detect_type(hdr, sizeof(*hdr)) != filetype_aimage) {
		pr_err("Image is not an Android image\n");
		ret = -EINVAL;
		goto err;
	}

	if (hdr->page_size < sizeof(*hdr) || hdr->page_size > SZ_64K) {
		pr_err("Invalid page_size 0x%08x\n", hdr->page_size);
		ret = -EINVAL;
		goto err_close;
	}

	ofs = hdr->page_size;
	if (hdr->kernel.size) {
		kernel = aimage_copy_component(fd, ofs, hdr->kernel.size);
		if (IS_ERR(kernel)) {
			kernel = NULL;
			ret = PTR_ERR(kernel);
			goto err_close;
		}
	}

	ofs += ALIGN(hdr->kernel.size, hdr->page_size);

	if (hdr->ramdisk.size) {
		initrd = aimage_copy_component(fd, ofs, hdr->ramdisk.size);
		if (IS_ERR(initrd)) {
			initrd = NULL;
			ret = PTR_ERR(initrd);
			goto err_close;
		}
	}

	if (kernel)
		bootm_data.os_file = kernel;

	if (initrd)
		bootm_data.initrd_file = initrd;

	close(fd);

	ret = bootm_boot(&bootm_data);

err_close:
	close(fd);

	if (kernel)
		unlink(kernel);
	if (initrd)
		unlink(initrd);
err:
	free(hdr);

	return ret;
}

static struct image_handler aimage_bootm_handler = {
	.name = "Android boot image",
	.bootm = do_bootm_aimage,
	.filetype = filetype_aimage,
};

static int bootm_aimage_register(void)
{
	return register_image_handler(&aimage_bootm_handler);
}
late_initcall(bootm_aimage_register);
