// SPDX-License-Identifier: GPL-2.0-or-later

#include <bootm.h>
#include <bootm-uimage.h>
#include <linux/kstrtox.h>

static int uimage_part_num(const char *partname)
{
	if (!partname)
		return 0;
	return simple_strtoul(partname, NULL, 0);
}

/*
 * bootm_load_uimage_os() - load uImage OS to RAM
 *
 * @data:		image data context
 * @load_address:	The address where the OS should be loaded to
 *
 * This loads the OS to a RAM location. load_address must be a valid
 * address. If the image_data doesn't have a OS specified it's considered
 * an error.
 *
 * Return: 0 on success, negative error code otherwise
 */
int bootm_load_uimage_os(struct image_data *data, unsigned long load_address)
{
	int num;

	num = uimage_part_num(data->os_part);

	data->os_res = uimage_load_to_sdram(data->os,
		num, load_address);
	if (!data->os_res)
		return -ENOMEM;

	return 0;
}

static int bootm_open_initrd_uimage(struct image_data *data)
{
	int ret;

	if (strcmp(data->os_file, data->initrd_file)) {
		data->initrd = uimage_open(data->initrd_file);
		if (!data->initrd)
			return -EINVAL;

		if (bootm_get_verify_mode() > BOOTM_VERIFY_NONE) {
			ret = uimage_verify(data->initrd);
			if (ret) {
				pr_err("Checking data crc failed with %pe\n",
					ERR_PTR(ret));
				return ret;
			}
		}
		uimage_print_contents(data->initrd);
	} else {
		data->initrd = data->os;
	}

	return 0;
}

/*
 * bootm_load_uimage_initrd() - load initrd from uimage to RAM
 *
 * @data:		image data context
 * @load_address:	The address where the initrd should be loaded to
 *
 * This loads the initrd to a RAM location. load_address must be a valid
 * address. If the image_data doesn't have a initrd specified this function
 * still returns successful as an initrd is optional.
 *
 * Return: initrd resource on success, NULL if no initrd is present or
 *         an error pointer if an error occurred.
 */
struct resource *
bootm_load_uimage_initrd(struct image_data *data, unsigned long load_address)
{
	struct resource *res;
	int ret;

	int num;
	ret = bootm_open_initrd_uimage(data);
	if (ret) {
		pr_err("loading initrd failed with %pe\n", ERR_PTR(ret));
		return ERR_PTR(ret);
	}

	num = uimage_part_num(data->initrd_part);

	res = uimage_load_to_sdram(data->initrd,
		num, load_address);
	if (!res)
		return ERR_PTR(-ENOMEM);

	return res;
}

int bootm_open_oftree_uimage(struct image_data *data, size_t *size,
			     struct fdt_header **fdt)
{
	enum filetype ft;
	const char *oftree = data->oftree_file;
	int num = uimage_part_num(data->oftree_part);
	struct uimage_handle *of_handle;
	int release = 0;

	pr_info("Loading devicetree from '%s'@%d\n", oftree, num);

	if (!strcmp(data->os_file, oftree)) {
		of_handle = data->os;
	} else if (!strcmp(data->initrd_file, oftree)) {
		of_handle = data->initrd;
	} else {
		of_handle = uimage_open(oftree);
		if (!of_handle)
			return -ENODEV;
		uimage_print_contents(of_handle);
		release = 1;
	}

	*fdt = uimage_load_to_buf(of_handle, num, size);

	if (release)
		uimage_close(of_handle);

	ft = file_detect_type(*fdt, *size);
	if (ft != filetype_oftree) {
		pr_err("%s is not an oftree but %s\n",
			data->oftree_file, file_type_to_string(ft));
		free(*fdt);
		return -EINVAL;
	}

	return 0;
}

int bootm_open_uimage(struct image_data *data)
{
	int ret;

	data->os = uimage_open(data->os_file);
	if (!data->os)
		return -EINVAL;

	if (bootm_get_verify_mode() > BOOTM_VERIFY_NONE) {
		ret = uimage_verify(data->os);
		if (ret) {
			pr_err("Checking data crc failed with %pe\n",
					ERR_PTR(ret));
			return ret;
		}
	}

	uimage_print_contents(data->os);

	if (IH_ARCH == IH_ARCH_INVALID || data->os->header.ih_arch != IH_ARCH) {
		pr_err("Unsupported Architecture 0x%x\n",
		       data->os->header.ih_arch);
		return -EINVAL;
	}

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = data->os->header.ih_load;

	return 0;
}

void bootm_close_uimage(struct image_data *data)
{
	if (data->initrd && data->initrd != data->os)
		uimage_close(data->initrd);
	uimage_close(data->os);
}
