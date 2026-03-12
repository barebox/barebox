// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <bootargs.h>
#include <bootm.h>
#include <fs.h>
#include <fcntl.h>
#include <efi/mode.h>
#include <malloc.h>
#include <memory.h>
#include <block.h>
#include <libfile.h>
#include <bootm-fit.h>
#include <bootm-overrides.h>
#include <bootm-uimage.h>
#include <globalvar.h>
#include <init.h>
#include <environment.h>
#include <linux/stat.h>
#include <magicvar.h>
#include <zero_page.h>
#include <security/config.h>

static LIST_HEAD(handler_list);
static struct sconfig_notifier_block sconfig_notifier;

static bool uimage_check(struct image_handler *handler,
			 struct image_data *data,
			 enum filetype detected_filetype)
{
	return detected_filetype == filetype_uimage &&
		handler->ih_os == data->os_uimage->header.ih_os;
}

static bool filetype_check(struct image_handler *handler,
			   struct image_data *data,
			   enum filetype detected_filetype)
{
	return handler->filetype == detected_filetype;
}

int register_image_handler(struct image_handler *handler)
{
	if (!handler->check_image) {
		if (IS_ENABLED(CONFIG_BOOTM_UIMAGE) &&
		    handler->filetype == filetype_uimage)
			handler->check_image = uimage_check;
		else
			handler->check_image = filetype_check;
	}

	list_add_tail(&handler->list, &handler_list);
	return 0;
}

static struct image_handler *bootm_find_handler(enum filetype filetype,
		struct image_data *data)
{
	struct image_handler *handler;

	list_for_each_entry(handler, &handler_list, list) {
		if (handler->check_image(handler, data, filetype))
			return handler;
	}

	return NULL;
}

static int bootm_appendroot;
static int bootm_dryrun;
static int bootm_earlycon;
static int bootm_provide_machine_id;
static int bootm_provide_hostname;
static int bootm_verbosity;
static int bootm_efi_mode = BOOTM_EFI_AVAILABLE;

void bootm_data_init_defaults(struct bootm_data *data)
{
	data->initrd_address = UIMAGE_INVALID_ADDRESS;
	data->os_address = UIMAGE_SOME_ADDRESS;
	data->os_entry = UIMAGE_SOME_ADDRESS;
	data->oftree_file = getenv_nonempty("global.bootm.oftree");
	data->tee_file = getenv_nonempty("global.bootm.tee");
	data->os_file = getenv_nonempty("global.bootm.image");
	getenv_ul("global.bootm.image.loadaddr", &data->os_address);
	if (IS_ENABLED(CONFIG_BOOTM_INITRD)) {
		getenv_ul("global.bootm.initrd.loadaddr", &data->initrd_address);
		data->initrd_file = getenv_nonempty("global.bootm.initrd");
	}
	data->root_dev = getenv_nonempty("global.bootm.root_dev");
	data->root_param = getenv_nonempty("global.bootm.root_param");
	data->verify = bootm_get_verify_mode();
	data->appendroot = bootm_appendroot;
	data->provide_machine_id = bootm_provide_machine_id;
	data->provide_hostname = bootm_provide_hostname;
	data->verbose = bootm_verbosity;
	data->dryrun = bootm_dryrun;
	data->efi_boot = bootm_efi_mode;
}

void bootm_data_restore_defaults(const struct bootm_data *data)
{
	globalvar_set("bootm.oftree", data->oftree_file);
	globalvar_set("bootm.tee", data->tee_file);
	globalvar_set("bootm.image", data->os_file);
	pr_setenv("global.bootm.image.loadaddr", "0x%lx", data->os_address);
	if (IS_ENABLED(CONFIG_BOOTM_INITRD)) {
		pr_setenv("global.bootm.initrd.loadaddr", "0x%lx", data->initrd_address);
		globalvar_set("bootm.initrd", data->initrd_file);
	}
	globalvar_set("bootm.root_dev", data->root_dev);
	globalvar_set("bootm.root_param", data->root_param);
	bootm_set_verify_mode(data->verify);
	bootm_appendroot = data->appendroot;
	bootm_provide_machine_id = data->provide_machine_id;
	bootm_provide_hostname = data->provide_hostname;
	bootm_verbosity = data->verbose;
	bootm_dryrun = data->dryrun;
	bootm_efi_mode = data->efi_boot;
}

static enum bootm_verify bootm_verify_mode = BOOTM_VERIFY_AVAILABLE;

enum bootm_verify bootm_get_verify_mode(void)
{
	return bootm_verify_mode;
}

void bootm_set_verify_mode(enum bootm_verify mode)
{
	bootm_verify_mode = mode;
}

static const char * const bootm_verify_names[] = {
#ifndef CONFIG_BOOTM_FORCE_SIGNED_IMAGES
	[BOOTM_VERIFY_NONE] = "none",
	[BOOTM_VERIFY_HASH] = "hash",
	[BOOTM_VERIFY_AVAILABLE] = "available",
#endif
	[BOOTM_VERIFY_SIGNATURE] = "signature",
};

/*
 * There's three ways to influence whether signed images are forced:
 * 1) CONFIG_BOOTM_FORCE_SIGNED_IMAGES: forced at compile time
 * 2) SCONFIG_BOOT_UNSIGNED_IMAGES: determined by the active security policy
 * 3) bootm_force_signed_images(): forced dynamically by board code.
 *                                 will be deprecated in favor of 2)
 */
static bool force_signed_images = IS_ENABLED(CONFIG_BOOTM_FORCE_SIGNED_IMAGES);

static void bootm_optional_signed_images(void)
{
	/* This function should not be exported */
	BUG_ON(force_signed_images);

	globalvar_remove("bootm.verify");
	/* recreate bootm.verify with a single enumeration as option */
	globalvar_add_simple_enum("bootm.verify", (unsigned int *)&bootm_verify_mode,
				  bootm_verify_names, ARRAY_SIZE(bootm_verify_names));

	bootm_verify_mode = BOOTM_VERIFY_AVAILABLE;
}

static void bootm_require_signed_images(void)
{
	static unsigned int verify_mode = 0;

	/* recreate bootm.verify with a single enumeration as option */
	globalvar_remove("bootm.verify");
	globalvar_add_simple_enum("bootm.verify", &verify_mode,
				  &bootm_verify_names[BOOTM_VERIFY_SIGNATURE], 1);

	bootm_verify_mode = BOOTM_VERIFY_SIGNATURE;
}

static void bootm_unsigned_sconfig_update(struct sconfig_notifier_block *nb,
					  enum security_config_option opt,
					  bool allowed)
{
	if (!allowed)
		bootm_require_signed_images();
	else
		bootm_optional_signed_images();
}

void bootm_force_signed_images(void)
{
	bootm_require_signed_images();
	force_signed_images = true;
}

bool bootm_signed_images_are_forced(void)
{
	return force_signed_images || !IS_ALLOWED(SCONFIG_BOOT_UNSIGNED_IMAGES);
}

static int uimage_part_num(const char *partname)
{
	if (!partname)
		return 0;
	return simple_strtoul(partname, NULL, 0);
}

/**
 * bootm_load_os() - load OS to RAM
 * @data:		image data context
 * @load_address:	The address where the OS should be loaded to
 * @end_address:	The end address of the load buffer (inclusive)
 *
 * This loads the OS to a RAM location. load_address must be a valid
 * address. If the image_data doesn't have an OS specified it's considered
 * an error.
 *
 * Return: the OS resource on success, or an error pointer on failure
 */
const struct resource *bootm_load_os(struct image_data *data,
		ulong load_address, ulong end_address)
{
	struct resource *res;

	if (data->os_res)
		return data->os_res;

	if (load_address == UIMAGE_INVALID_ADDRESS ||
	    end_address <= load_address || !data->os)
		return ERR_PTR(-EINVAL);

	res = loadable_extract_into_sdram_all(data->os, load_address, end_address);
	if (!IS_ERR(res))
		data->os_res = res;

	return res;
}

/**
 * bootm_load_initrd() - load initrd to RAM
 * @data:		image data context
 * @load_address:	The address where the initrd should be loaded to
 * @end_address:	The end address of the load buffer (inclusive)
 *
 * This loads the initrd to a RAM location. load_address must be a valid
 * address. If the image_data doesn't have an initrd specified this function
 * still returns successful as an initrd is optional.
 *
 * Return: the initrd resource if one was loaded, NULL if no initrd was
 *         specified, or an error pointer on failure
 */
const struct resource *
bootm_load_initrd(struct image_data *data, ulong load_address, ulong end_address)
{
	struct resource *res = NULL;

	if (!IS_ENABLED(CONFIG_BOOTM_INITRD))
		return NULL;

	/* TODO: This should not be set anywhere, but in case it is, let's print
	 * a warning to find out if we need this
	 */
	if (WARN_ON(data->initrd_res))
		return data->initrd_res;

	if (!data->initrd)
		return NULL;

	if (end_address <= load_address)
		return ERR_PTR(-EINVAL);

	res = loadable_extract_into_sdram_all(data->initrd, load_address, end_address);
	if (!IS_ERR(res))
		data->initrd_res = res;
	return res;
}

/*
 * bootm_get_devicetree() - get devicetree
 *
 * @data:		image data context
 *
 * This gets the fixed devicetree from the various image sources or the internal
 * devicetree. It returns a pointer to the allocated devicetree which must be
 * freed after use.
 *
 * Return: pointer to the fixed devicetree, NULL if image_data has an empty DT
 *         or a ERR_PTR() on failure.
 */
void *bootm_get_devicetree(struct image_data *data)
{
	struct fdt_header *oftree;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return ERR_PTR(-ENOSYS);

	if (data->oftree) {
		const struct fdt_header *oftree_view;
		size_t size;

		oftree_view = loadable_view(data->oftree, &size);
		if (IS_ERR(oftree_view))
			pr_err("could not open device tree \"%s\": %pe\n",
			       data->oftree_file, oftree_view);
		if (IS_ERR_OR_NULL(oftree_view))
			return ERR_CAST(oftree_view);

		data->of_root_node = of_unflatten_dtb(oftree_view, size);
		loadable_view_free(data->oftree, oftree_view, size);

		if (IS_ERR(data->of_root_node)) {
			data->of_root_node = NULL;
			pr_err("unable to unflatten devicetree\n");
			return ERR_PTR(-EINVAL);
		}

	} else {
		data->of_root_node = of_dup_root_node_for_boot();
		if (!data->of_root_node)
			return NULL;

		if (bootm_verbose(data) > 1)
			printf("using internal devicetree\n");
	}

	if (data->initrd_res) {
		of_add_initrd(data->of_root_node, data->initrd_res->start,
				data->initrd_res->end);
		of_add_reserve_entry(data->initrd_res->start, data->initrd_res->end);
	}

	bootm_set_pending_oftree_overlays(data->oftree);
	of_fix_tree(data->of_root_node);
	bootm_clear_pending_oftree_overlays();

	oftree = of_flatten_dtb(data->of_root_node);
	if (!oftree)
		return ERR_PTR(-EINVAL);

	fdt_add_reserve_map(oftree);

	return oftree;
}

/**
 * bootm_load_devicetree() - load devicetree into specified memory range
 * @data:		image data context
 * @fdt:		The flat device tree to load
 * @load_address:	The address where the devicetree should be loaded to
 * @end_address:	The end address of the load buffer (inclusive)
 *
 * This loads the devicetree to a RAM location. load_address must be a valid
 * address which is requested with request_sdram_region. The associated region
 * is released automatically in the bootm error path.
 *
 * Return: the devicetree resource on success, or an error pointer on failure
 */
const struct resource *
bootm_load_devicetree(struct image_data *data, void *fdt,
		      ulong load_address, ulong end_address)
{
	int fdt_size;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return ERR_PTR(-ENOSYS);
	if (end_address <= load_address)
		return ERR_PTR(-EINVAL);

	fdt_size = be32_to_cpu(((struct fdt_header *)fdt)->totalsize);
	if (load_address + fdt_size - 1 > end_address)
		return ERR_PTR(-ENOSPC);

	data->oftree_res = request_sdram_region("oftree", load_address,
			fdt_size, MEMTYPE_LOADER_DATA, MEMATTRS_RW);
	if (!data->oftree_res)
		return ERR_PTR(-EBUSY);

	memcpy((void *)data->oftree_res->start, fdt, fdt_size);

	of_print_cmdline(data->of_root_node);
	if (bootm_verbose(data) > 1) {
		of_print_nodes(data->of_root_node, 0, ~0);
		fdt_print_reserve_map(fdt);
	}

	return data->oftree_res;
}

loff_t bootm_get_os_size(struct image_data *data)
{
	loff_t size;

	if (!data->os)
		return -EINVAL;

	return loadable_get_size(data->os, &size) ?: size;
}

static void bootm_print_info(struct image_data *data)
{
	if (data->os_res)
		printf("OS image is at %pa-%pa\n",
				&data->os_res->start,
				&data->os_res->end);
	else
		printf("OS image not yet relocated\n");
}

int bootm_image_name_and_part(const char *name, char **filename, char **part)
{
	char *at, *ret;

	if (!name || !*name)
		return -EINVAL;

	ret = xstrdup(name);

	*filename = ret;
	*part = NULL;

	at = strchr(ret, '@');
	if (!at)
		return 0;

	*at++ = 0;

	*part = at;

	return 0;
}

/**
 * file_read_and_detect_boot_image_type - read file header and detect image type
 * @os_file: path to the boot image file
 * @os_header: returns a PAGE_SIZE buffer with the file header (caller frees)
 *
 * Return: detected filetype (enum filetype) on success, negative error code
 *         on failure. On error, *os_header is set to NULL.
 */
int file_read_and_detect_boot_image_type(const char *os_file, void **os_header)
{
	size_t size;
	int ret;

	ret = read_file_2(os_file, &size, os_header, PAGE_SIZE);
	if (ret < 0 && ret != -EFBIG) {
		pr_err("could not open %s: %pe\n", os_file, ERR_PTR(ret));
		return ret;
	}
	if (size < PAGE_SIZE) {
		free(*os_header);
		*os_header = NULL;
		return -ENODATA;
	}

	return file_detect_boot_image_type(*os_header, PAGE_SIZE);
}

static int bootm_open_files(struct image_data *data)
{
	data->os = loadable_from_file(data->os_file, LOADABLE_KERNEL);
	if (IS_ERR(data->os))
		return PTR_ERR(data->os);

	if (data->oftree_file) {
		data->oftree = loadable_from_file(data->oftree_file, LOADABLE_FDT);
		if (IS_ERR(data->oftree))
			return PTR_ERR(data->oftree);
	}

	if (data->initrd_file) {
		int ret = loadables_from_files(&data->initrd, data->initrd_file, ":",
					       LOADABLE_INITRD);
		if (ret)
			return ret;
	}

	if (data->tee_file) {
		data->tee = loadable_from_file(data->tee_file, LOADABLE_TEE);
		if (IS_ERR(data->tee))
			return PTR_ERR(data->tee);
	}

	return 0;
}

int bootm_open_os_compressed(struct image_data *data)
{
	void *header;
	ssize_t ret;

	if (!IS_ENABLED(CONFIG_BOOTM_COMPRESSED))
		return -ENOSYS;

	/* Wrap the OS loadable with transparent decompression */
	data->os = loadable_decompress(data->os);

	/* Read decompressed header to detect actual kernel type */
	header = xzalloc(PAGE_SIZE);
	ret = loadable_extract_into_buf(data->os, header, PAGE_SIZE, 0,
					LOADABLE_EXTRACT_PARTIAL);
	if (ret < 0) {
		free(header);
		return ret;
	}

	/* Detect actual image type from decompressed content */
	data->kernel_type = file_detect_boot_image_type(header, ret);

	/* Replace the os_header with the decompressed header */
	free(data->os_header);
	data->os_header = header;

	return 0;
}

struct image_data *bootm_boot_prep(const struct bootm_data *bootm_data)
{
	struct image_data *data;
	int ret;
	const char *image_type_str;

	if (!bootm_data->os_file) {
		pr_err("no image given\n");
		return ERR_PTR(-ENOENT);
	}

	data = xzalloc(sizeof(*data));

	bootm_image_name_and_part(bootm_data->os_file, &data->os_file, &data->os_part);
	bootm_image_name_and_part(bootm_data->oftree_file, &data->oftree_file, &data->oftree_part);
	bootm_image_name_and_part(bootm_data->initrd_file, &data->initrd_file, &data->initrd_part);
	if (bootm_data->tee_file)
		data->tee_file = xstrdup(bootm_data->tee_file);
	data->verbose = bootm_data->verbose;
	data->verify = bootm_data->verify;
	data->force = bootm_data->force;
	data->dryrun = bootm_data->dryrun;
	data->initrd_address = bootm_data->initrd_address;
	data->os_address = data->os_address_hint = bootm_data->os_address;
	data->os_entry = data->os_entry_hint = bootm_data->os_entry;
	data->efi_boot = bootm_data->efi_boot;

	ret = file_read_and_detect_boot_image_type(data->os_file, &data->os_header);
	if (ret < 0)
		goto err_out;
	data->image_type = ret;

	if (!data->force && data->image_type == filetype_unknown) {
		pr_err("Unknown OS filetype (try -f)\n");
		ret = -EINVAL;
		goto err_out;
	}

	if (bootm_signed_images_are_forced()) {
		data->verify = BOOTM_VERIFY_SIGNATURE;

		/*
		 * When we only allow booting signed images make sure everything
		 * we boot is in the OS image and not given separately.
		 */
		data->oftree_file = NULL;
		data->initrd_file = NULL;
		data->tee_file = NULL;
		if (data->image_type != filetype_fit) {
			pr_err("Signed boot and image is no FIT image, aborting\n");
			ret = -EINVAL;
			goto err_out;
		}
	}

	image_type_str = file_type_to_short_string(data->image_type);

	/* May be updated by below container-specific handlers */
	data->kernel_type = data->image_type;

	switch (data->image_type) {
	case filetype_fit:
		ret = bootm_open_fit(data, false);
		break;
	case filetype_uimage:
		ret = bootm_open_uimage(data);
		break;
	case filetype_gzip:
	case filetype_bzip2:
	case filetype_lzo_compressed:
	case filetype_lz4_compressed:
	case filetype_xz_compressed:
	case filetype_zstd_compressed:
		ret = bootm_open_files(data);
		if (!ret)
			ret = bootm_open_os_compressed(data);
		image_type_str = "compressed boot";
		break;
	default:
		ret = bootm_open_files(data);
		image_type_str = "boot";
		break;
	}

	if (ret) {
		pr_err("Loading %s image failed with: %pe\n", image_type_str, ERR_PTR(ret));
		goto err_out;
	}

	if (IS_ENABLED(CONFIG_BOOTM_UIMAGE)) {
		ret = bootm_collect_uimage_loadables(data);
		if (ret)
			goto err_out;
	}

	if (bootm_data->appendroot) {
		const char *root = NULL;
		const char *rootopts = NULL;

		if (bootm_data->root_dev) {
			const char *root_dev_name = devpath_to_name(bootm_data->root_dev);
			struct cdev *root_cdev = cdev_open_by_name(root_dev_name, O_RDONLY);

			ret = cdev_get_linux_root_and_opts(root_cdev, &root, &rootopts);

			if (ret) {
				if (!root_cdev)
					pr_err("no cdev found for %s, cannot set %s= option\n",
						root_dev_name, bootm_data->root_param);
				else if (!root_cdev->partuuid[0])
					pr_err("%s doesn't have a PARTUUID, cannot set %s= option\n",
						root_dev_name, bootm_data->root_param);
				else
					pr_err("could not determine %s= from %s\n",
						bootm_data->root_param, root_dev_name);
			}

			if (root_cdev)
				cdev_close(root_cdev);
		} else {
			struct fs_device *fsdev = get_fsdevice_by_path(AT_FDCWD, data->os_file);
			if (fsdev)
				fsdev_get_linux_root_options(fsdev, &root, &rootopts);
			else
				pr_err("no fsdevice under path: %s\n", data->os_file);
		}

		if (!root) {
			pr_err("Failed to append kernel cmdline parameter '%s='\n",
			       bootm_data->root_param);
		} else {
			char *rootarg;

			rootarg = format_root_bootarg(bootm_data->root_param, root, rootopts);
			pr_info("Adding \"%s\" to Kernel commandline\n", rootarg);
			globalvar_add_simple("linux.bootargs.bootm.appendroot",
					     rootarg);
			free(rootarg);
		}
	}

	if (bootm_earlycon) {
		struct console_device *console;
		const char *earlycon = NULL;

		for_each_console(console) {
			if (!(console->f_active & (CONSOLE_STDOUT | CONSOLE_STDERR)))
				continue;

			earlycon = dev_get_param(&console->class_dev, "linux.bootargs.earlycon");
			if (earlycon)
				break;
		}

		if (!earlycon)
			earlycon = "earlycon";

		pr_info("Adding \"%s\" to Kernel commandline\n", earlycon);
		globalvar_add_simple("linux.bootargs.bootm.earlycon", earlycon);
	}

	if (bootm_data->provide_machine_id) {
		const char *machine_id = getenv_nonempty("global.machine_id");
		char *machine_id_bootarg;

		if (!machine_id) {
			pr_err("Providing machine id is enabled but no machine id set\n");
			ret = -EINVAL;
			goto err_out;
		}

		machine_id_bootarg = basprintf("systemd.machine_id=%s", machine_id);
		globalvar_add_simple("linux.bootargs.machine_id", machine_id_bootarg);
		free(machine_id_bootarg);
	}

	if (bootm_data->provide_hostname) {
		const char *hostname = getenv_nonempty("global.hostname");
		const char *suffix = NULL;
		char *hostname_bootarg;

		if (!hostname) {
			pr_err("Providing hostname is enabled but no hostname is set\n");
			ret = -EINVAL;
			goto err_out;
		}

		if (!barebox_hostname_is_valid(hostname)) {
			pr_err("Provided hostname is not compatible to systemd hostname requirements\n");
			ret = -EINVAL;
			goto err_out;
		}

		if (IS_ENABLED(CONFIG_SERIAL_NUMBER_FIXUP_SYSTEMD_HOSTNAME))
			suffix = barebox_get_serial_number();

		hostname_bootarg = basprintf("systemd.hostname=%s%s%s",
					     hostname, suffix ? "-" : "",
					     suffix ?: "");

		globalvar_add_simple("linux.bootargs.hostname", hostname_bootarg);
		free(hostname_bootarg);
	}

	return data;
err_out:
	bootm_boot_cleanup(data);
	return ERR_PTR(ret);
}

int bootm_boot_handler(struct image_data *data)
{
	struct image_handler *handler;
	int ret;

	pr_info("\nLoading %s '%s'", file_type_to_string(data->kernel_type),
		data->os_file);
	if (data->kernel_type == filetype_uimage &&
			data->os_uimage->header.ih_type == IH_TYPE_MULTI)
		pr_info(", multifile image %d", uimage_part_num(data->os_part));
	pr_info("\n");

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = UIMAGE_INVALID_ADDRESS;
	if (data->os_entry == UIMAGE_SOME_ADDRESS)
		data->os_entry = 0;

	handler = bootm_find_handler(data->kernel_type, data);
	if (!handler) {
		pr_err("no image handler found for image type %s\n",
		       file_type_to_string(data->kernel_type));
		if (data->kernel_type == filetype_uimage)
			pr_err("and OS type: %d\n", data->os_uimage->header.ih_os);
		return -ENODEV;
	}

	if (bootm_verbose(data)) {
		bootm_print_info(data);
		printf("Passing control to %s handler\n", handler->name);
	}

	ret = handler->bootm(data);
	if (data->dryrun)
		pr_info("Dryrun. Aborted\n");

	return ret;
}

void bootm_boot_cleanup(struct image_data *data)
{
	release_sdram_region(data->os_res);
	if (data->initrd_res)
		of_del_reserve_entry(data->initrd_res->start, data->initrd_res->end);
	release_sdram_region(data->initrd_res);
	release_sdram_region(data->oftree_res);
	release_sdram_region(data->tee_res);
	loadable_release(&data->oftree);
	loadable_release(&data->initrd);
	loadable_release(&data->os);
	loadable_release(&data->tee);
	if (data->of_root_node)
		of_delete_node(data->of_root_node);

	globalvar_remove("linux.bootargs.bootm.earlycon");
	globalvar_remove("linux.bootargs.bootm.appendroot");
	free(data->os_header);
	free(data->os_file);
	free(data->oftree_file);
	free(data->initrd_file);
	free(data->tee_file);
	free(data);
}

/*
 * bootm_boot - Boot an application image described by bootm_data
 */
int bootm_boot(const struct bootm_data *bootm_data)
{
	struct image_data *data;
	int ret;

	data = bootm_boot_prep(bootm_data);
	if (IS_ERR(data))
		return PTR_ERR(data);

	ret = bootm_boot_handler(data);

	bootm_boot_cleanup(data);
	return ret;
}

bool bootm_efi_check_image(struct image_handler *handler,
			   struct image_data *data,
			   enum filetype detected_filetype)
{
	/* This is our default: Use EFI when support is compiled in,
	 * and fallback to normal boot otherwise.
	 */
	if (data->efi_boot == BOOTM_EFI_AVAILABLE) {
		if (IS_ENABLED(CONFIG_EFI_LOADER) || efi_is_payload())
			data->efi_boot = BOOTM_EFI_REQUIRED;
		else
			data->efi_boot = BOOTM_EFI_DISABLED;
	}

	/* If EFI is disabled, we assume EFI-stubbed images are
	 * just normal non-EFI stubbed ones
	 */
	if (data->efi_boot == BOOTM_EFI_DISABLED)
		return handler->filetype == filetype_no_efistub(detected_filetype);

	/* If EFI is required, we enforce handlers to match exactly */
	return detected_filetype == handler->filetype;
}

int linux_rootwait_secs = 10;

static const char * const bootm_efi_loader_mode_names[] = {
	[BOOTM_EFI_DISABLED] = "disabled",
	[BOOTM_EFI_AVAILABLE] = "available",
	[BOOTM_EFI_REQUIRED] = "required",
};

static int bootm_init(void)
{
	globalvar_add_simple("bootm.image", NULL);
	globalvar_add_simple("bootm.image.loadaddr", NULL);
	globalvar_add_simple("bootm.oftree", NULL);
	globalvar_add_simple("bootm.root_dev", NULL);
	globalvar_add_simple("bootm.root_param", "root");
	globalvar_add_simple("bootm.tee", NULL);
	globalvar_add_simple_bool("bootm.appendroot", &bootm_appendroot);
	globalvar_add_simple_bool("bootm.earlycon", &bootm_earlycon);
	globalvar_add_simple_bool("bootm.provide_machine_id", &bootm_provide_machine_id);
	globalvar_add_simple_bool("bootm.provide_hostname", &bootm_provide_hostname);
	if (IS_ENABLED(CONFIG_BOOTM_INITRD)) {
		globalvar_add_simple("bootm.initrd", NULL);
		globalvar_add_simple("bootm.initrd.loadaddr", NULL);
	}

	globalvar_add_simple_bool("bootm.dryrun", &bootm_dryrun);
	globalvar_add_simple_int("bootm.verbose", &bootm_verbosity, "%u");

	if (bootm_signed_images_are_forced())
		bootm_require_signed_images();
	else
		bootm_optional_signed_images();

	sconfig_register_handler_filtered(&sconfig_notifier,
					  bootm_unsigned_sconfig_update,
					  SCONFIG_BOOT_UNSIGNED_IMAGES);


	if (IS_ENABLED(CONFIG_ROOTWAIT_BOOTARG))
		globalvar_add_simple_int("linux.rootwait",
					 &linux_rootwait_secs, "%d");

	if (IS_ENABLED(CONFIG_EFI_LOADER) && !efi_is_payload())
		globalvar_add_simple_enum("bootm.efi", &bootm_efi_mode,
					  bootm_efi_loader_mode_names,
					  ARRAY_SIZE(bootm_efi_loader_mode_names));

	return 0;
}
late_initcall(bootm_init);

BAREBOX_MAGICVAR(bootargs, "Linux kernel parameters");
BAREBOX_MAGICVAR(global.bootm.image, "bootm default boot image");
BAREBOX_MAGICVAR(global.bootm.image.loadaddr, "bootm default boot image loadaddr");
BAREBOX_MAGICVAR(global.bootm.initrd, "bootm default initrd");
BAREBOX_MAGICVAR(global.bootm.initrd.loadaddr, "bootm default initrd loadaddr");
BAREBOX_MAGICVAR(global.bootm.oftree, "bootm default oftree");
BAREBOX_MAGICVAR(global.bootm.tee, "bootm default tee image");
BAREBOX_MAGICVAR(global.bootm.dryrun, "bootm default dryrun level");
BAREBOX_MAGICVAR(global.bootm.verify, "bootm default verify level");
#ifdef CONFIG_EFI_LOADER
BAREBOX_MAGICVAR(global.bootm.efi, "efiloader: Behavior for EFI-stubbable boot images: EFI \"disabled\", EFI if \"available\" (default), EFI is \"required\"");
#endif
BAREBOX_MAGICVAR(global.bootm.verbose, "bootm default verbosity level (0=quiet)");
BAREBOX_MAGICVAR(global.bootm.earlycon, "Add earlycon option to Kernel for early log output");
BAREBOX_MAGICVAR(global.bootm.appendroot, "Add root= option to Kernel to mount rootfs from the device the Kernel comes from (default, device can be overridden via global.bootm.root_dev)");
BAREBOX_MAGICVAR(global.bootm.root_dev, "bootm default root device (overrides default device in global.bootm.appendroot)");
BAREBOX_MAGICVAR(global.bootm.root_param, "bootm root parameter name (normally 'root' for root=/dev/...)");
BAREBOX_MAGICVAR(global.bootm.provide_machine_id, "If true, append systemd.machine_id=$global.machine_id to Kernel command line");
BAREBOX_MAGICVAR(global.bootm.provide_hostname, "If true, append systemd.hostname=$global.hostname to Kernel command line");
