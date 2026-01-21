// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <bootargs.h>
#include <bootm.h>
#include <bootm-overrides.h>
#include <fs.h>
#include <fcntl.h>
#include <efi/mode.h>
#include <malloc.h>
#include <memory.h>
#include <block.h>
#include <libfile.h>
#include <bootm-fit.h>
#include <image-fit.h>
#include <bootm-uimage.h>
#include <globalvar.h>
#include <init.h>
#include <environment.h>
#include <linux/stat.h>
#include <magicvar.h>
#include <uncompress.h>
#include <zero_page.h>
#include <security/config.h>

static LIST_HEAD(handler_list);
static struct sconfig_notifier_block sconfig_notifier;

static __maybe_unused struct bootm_overrides bootm_overrides;

static bool uimage_check(struct image_handler *handler,
			 struct image_data *data,
			 enum filetype detected_filetype)
{
	return detected_filetype == filetype_uimage &&
		handler->ih_os == data->os->header.ih_os;
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

static inline bool image_is_uimage(struct image_data *data)
{
	return IS_ENABLED(CONFIG_BOOTM_UIMAGE) && data->os;
}

static bool bootm_get_override(char **oldpath, const char *newpath)
{
	if (!IS_ENABLED(CONFIG_BOOT_OVERRIDE))
		return false;
	if (bootm_signed_images_are_forced())
		return false;
	if (!newpath)
		return false;

	if (oldpath && !streq_ptr(*oldpath, newpath)) {
		free(*oldpath);
		*oldpath = *newpath ? xstrdup(newpath) : NULL;
	}

	return true;
}

/*
 * bootm_load_os() - load OS to RAM
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
int bootm_load_os(struct image_data *data, unsigned long load_address)
{
	if (data->os_res)
		return 0;

	if (load_address == UIMAGE_INVALID_ADDRESS)
		return -EINVAL;

	if (data->os_fit)
		return bootm_load_fit_os(data, load_address);

	if (image_is_uimage(data))
		return bootm_load_uimage_os(data, load_address);

	if (!data->os_file)
		return -EINVAL;

	data->os_res = file_to_sdram(data->os_file, load_address, MEMTYPE_LOADER_CODE);
	if (!data->os_res)
		return -ENOMEM;

	return 0;
}

/*
 * bootm_load_initrd() - load initrd to RAM
 *
 * @data:		image data context
 * @load_address:	The address where the initrd should be loaded to
 *
 * This loads the initrd to a RAM location. load_address must be a valid
 * address. If the image_data doesn't have a initrd specified this function
 * still returns successful as an initrd is optional. Check data->initrd_res
 * to see if an initrd has been loaded.
 *
 * Return: 0 on success, negative error code otherwise
 */
const struct resource *
bootm_load_initrd(struct image_data *data, unsigned long load_address)
{
	struct resource *res = NULL;
	const char *initrd, *initrd_part = NULL;
	enum filetype type = filetype_unknown;
	int ret;

	if (!IS_ENABLED(CONFIG_BOOTM_INITRD))
		return NULL;

	/* TODO: This should not be set anywhere, but in case it is, let's print
	 * a warning to find out if we need this
	 */
	if (WARN_ON(data->initrd_res))
		return data->initrd_res;

	bootm_get_override(&data->initrd_file, bootm_overrides.initrd_file);

	initrd = data->initrd_file;
	if (initrd) {
		ret = file_name_detect_type(initrd, &type);
		if (ret) {
			pr_err("could not open initrd \"%s\": %pe\n",
			       initrd, ERR_PTR(ret));
			return ERR_PTR(ret);
		}
	}

	if (type == filetype_uimage) {
		res = bootm_load_uimage_initrd(data, load_address);
		if (data->initrd->header.ih_type == IH_TYPE_MULTI)
			initrd_part = data->initrd_part;

	} else if (initrd) {
		res = file_to_sdram(initrd, load_address, MEMTYPE_LOADER_DATA)
			?: ERR_PTR(-ENOMEM);

	} else if (data->os_fit) {
		res = bootm_load_fit_initrd(data, load_address);
		type = filetype_fit;

	}

	if (IS_ERR_OR_NULL(res))
		return res;

	pr_info("Loaded initrd from %s %s%s%s to %pa-%pa\n",
		file_type_to_string(type), initrd,
		initrd_part ? "@" : "", initrd_part ?: "",
		&res->start, &res->end);

	data->initrd_res = res;
	return data->initrd_res;
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
	enum filetype type;
	struct fdt_header *oftree;
	bool from_fit = false;
	int ret;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return ERR_PTR(-ENOSYS);

	from_fit = bootm_fit_has_fdt(data);
	if (bootm_get_override(&data->oftree_file, bootm_overrides.oftree_file))
		from_fit = false;

	if (from_fit) {
		data->of_root_node = bootm_get_fit_devicetree(data);
	} else if (data->oftree_file) {
		size_t size;

		ret = file_name_detect_type(data->oftree_file, &type);
		if (ret) {
			pr_err("could not open device tree \"%s\": %pe\n", data->oftree_file,
			       ERR_PTR(ret));
			return ERR_PTR(ret);
		}

		switch (type) {
		case filetype_uimage:
			ret = bootm_open_oftree_uimage(data, &size, &oftree);
			break;
		case filetype_oftree:
			pr_info("Loading devicetree from '%s'\n", data->oftree_file);
			ret = read_file_2(data->oftree_file, &size, (void *)&oftree,
					  FILESIZE_MAX);
			break;
		case filetype_empty:
			return NULL;
		default:
			return ERR_PTR(-EINVAL);
		}

		if (ret)
			return ERR_PTR(ret);

		data->of_root_node = of_unflatten_dtb(oftree, size);

		free(oftree);

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

	of_fix_tree(data->of_root_node);

	oftree = of_flatten_dtb(data->of_root_node);
	if (!oftree)
		return ERR_PTR(-EINVAL);

	fdt_add_reserve_map(oftree);

	return oftree;
}

/*
 * bootm_load_devicetree() - load devicetree
 *
 * @data:		image data context
 * @fdt:		The flat device tree to load
 * @load_address:	The address where the devicetree should be loaded to
 *
 * This loads the devicetree to a RAM location. load_address must be a valid
 * address which is requested with request_sdram_region. The associated region
 * is released automatically in the bootm error path.
 *
 * Return: 0 on success, negative error code otherwise
 */
int bootm_load_devicetree(struct image_data *data, void *fdt,
			    unsigned long load_address)
{
	int fdt_size;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return -ENOSYS;

	fdt_size = be32_to_cpu(((struct fdt_header *)fdt)->totalsize);

	data->oftree_res = request_sdram_region("oftree", load_address,
			fdt_size, MEMTYPE_LOADER_DATA, MEMATTRS_RW);
	if (!data->oftree_res)
		return -ENOMEM;

	memcpy((void *)data->oftree_res->start, fdt, fdt_size);

	of_print_cmdline(data->of_root_node);
	if (bootm_verbose(data) > 1) {
		of_print_nodes(data->of_root_node, 0, ~0);
		fdt_print_reserve_map(fdt);
	}

	return 0;
}

int bootm_get_os_size(struct image_data *data)
{
	const char *os_file;
	struct stat s;
	int ret;

	if (image_is_uimage(data))
		return uimage_get_size(data->os, uimage_part_num(data->os_part));
	if (data->os_fit)
		return data->fit_kernel_size;
	if (!data->os_file)
		return -EINVAL;
	os_file = data->os_file;

	ret = stat(os_file, &s);
	if (ret)
		return ret;

	return s.st_size;
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

static int bootm_image_name_and_part(const char *name, char **filename, char **part)
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

/*
 * bootm_boot - Boot an application image described by bootm_data
 */
int bootm_boot(struct bootm_data *bootm_data)
{
	struct image_data *data;
	struct image_handler *handler;
	int ret;
	enum filetype os_type;
	size_t size;
	const char *os_type_str;

	if (!bootm_data->os_file) {
		pr_err("no image given\n");
		return -ENOENT;
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
	data->os_address = bootm_data->os_address;
	data->os_entry = bootm_data->os_entry;
	data->efi_boot = bootm_data->efi_boot;

	ret = read_file_2(data->os_file, &size, &data->os_header, PAGE_SIZE);
	if (ret < 0 && ret != -EFBIG) {
		pr_err("could not open %s: %pe\n", data->os_file, ERR_PTR(ret));
		goto err_out;
	}
	if (size < PAGE_SIZE)
		goto err_out;

	os_type = data->os_type = file_detect_boot_image_type(data->os_header, PAGE_SIZE);

	if (!data->force && os_type == filetype_unknown) {
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
		if (os_type != filetype_fit) {
			pr_err("Signed boot and image is no FIT image, aborting\n");
			ret = -EINVAL;
			goto err_out;
		}
	}

	os_type_str = file_type_to_short_string(os_type);

	switch (os_type) {
	case filetype_fit:
		ret = bootm_open_fit(data);
		os_type = file_detect_type(data->fit_kernel, data->fit_kernel_size);
		break;
	case filetype_uimage:
		ret = bootm_open_uimage(data);
		break;
	default:
		ret = 0;
		break;
	}

	if (ret) {
		pr_err("Loading %s image failed with: %pe\n", os_type_str, ERR_PTR(ret));
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

	pr_info("\nLoading %s '%s'", file_type_to_string(os_type),
		data->os_file);
	if (os_type == filetype_uimage &&
			data->os->header.ih_type == IH_TYPE_MULTI)
		pr_info(", multifile image %d", uimage_part_num(data->os_part));
	pr_info("\n");

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = UIMAGE_INVALID_ADDRESS;
	if (data->os_entry == UIMAGE_SOME_ADDRESS)
		data->os_entry = 0;

	handler = bootm_find_handler(os_type, data);
	if (!handler) {
		pr_err("no image handler found for image type %s\n",
		       file_type_to_string(os_type));
		if (os_type == filetype_uimage)
			pr_err("and OS type: %d\n", data->os->header.ih_os);
		ret = -ENODEV;
		goto err_out;
	}

	if (bootm_verbose(data)) {
		bootm_print_info(data);
		printf("Passing control to %s handler\n", handler->name);
	}

	bootm_get_override(&data->oftree_file, bootm_overrides.oftree_file);

	if (bootm_get_override(&data->initrd_file, bootm_overrides.initrd_file)) {
		release_sdram_region(data->initrd_res);
		data->initrd_res = NULL;
	}

	ret = handler->bootm(data);
	if (data->dryrun)
		pr_info("Dryrun. Aborted\n");

err_out:
	release_sdram_region(data->os_res);
	release_sdram_region(data->initrd_res);
	release_sdram_region(data->oftree_res);
	release_sdram_region(data->tee_res);
	if (image_is_uimage(data))
		bootm_close_uimage(data);
	if (data->os_fit)
		bootm_close_fit(data);
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

	return ret;
}

#ifdef CONFIG_BOOT_OVERRIDE
struct bootm_overrides bootm_set_overrides(const struct bootm_overrides overrides)
{
	struct bootm_overrides old = bootm_overrides;
	/* bootm_merge_overrides copies only actual (non-NULL) overrides */
	bootm_merge_overrides(&bootm_overrides, &overrides);
	return old;
}
#endif

bool bootm_efi_check_image(struct image_handler *handler,
			   struct image_data *data,
			   enum filetype detected_filetype)
{
	/* This is our default: Use EFI when support is compiled in,
	 * and fallback to normal boot otherwise.
	 */
	if (data->efi_boot == BOOTM_EFI_AVAILABLE) {
		if (IS_ENABLED(CONFIG_EFI_LOADER))
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

static int do_bootm_compressed(struct image_data *img_data)
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
	int from, to, ret;
	char *dstpath;

	from = open(img_data->os_file, O_RDONLY);
	if (from < 0)
		return -ENODEV;

	dstpath = make_temp("bootm-compressed");
	if (!dstpath) {
		ret = -ENOMEM;
		goto fail_from;
	}

	to = open(dstpath, O_CREAT | O_WRONLY);
	if (to < 0) {
		ret = -ENODEV;
		goto fail_make_temp;
	}

	ret = uncompress_fd_to_fd(from, to, uncompress_err_stdout);
	if (ret)
		goto fail_to;

	bootm_data.os_file = dstpath;
	ret = bootm_boot(&bootm_data);

fail_to:
	close(to);
	unlink(dstpath);
fail_make_temp:
	free(dstpath);
fail_from:
	close(from);
	return ret;
}

static struct image_handler bzip2_bootm_handler = {
	.name = "BZIP2 compressed file",
	.bootm = do_bootm_compressed,
	.filetype = filetype_bzip2,
};

static struct image_handler gzip_bootm_handler = {
	.name = "GZIP compressed file",
	.bootm = do_bootm_compressed,
	.filetype = filetype_gzip,
};

static struct image_handler lzo_bootm_handler = {
	.name = "LZO compressed file",
	.bootm = do_bootm_compressed,
	.filetype = filetype_lzo_compressed,
};

static struct image_handler lz4_bootm_handler = {
	.name = "LZ4 compressed file",
	.bootm = do_bootm_compressed,
	.filetype = filetype_lz4_compressed,
};

static struct image_handler xz_bootm_handler = {
	.name = "XZ compressed file",
	.bootm = do_bootm_compressed,
	.filetype = filetype_xz_compressed,
};

static struct image_handler zstd_bootm_handler = {
	.name = "ZSTD compressed file",
	.bootm = do_bootm_compressed,
	.filetype = filetype_zstd_compressed,
};

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

	if (IS_ENABLED(CONFIG_BZLIB))
		register_image_handler(&bzip2_bootm_handler);
	if (IS_ENABLED(CONFIG_ZLIB))
		register_image_handler(&gzip_bootm_handler);
	if (IS_ENABLED(CONFIG_LZO_DECOMPRESS))
		register_image_handler(&lzo_bootm_handler);
	if (IS_ENABLED(CONFIG_LZ4_DECOMPRESS))
		register_image_handler(&lz4_bootm_handler);
	if (IS_ENABLED(CONFIG_XZ_DECOMPRESS))
		register_image_handler(&xz_bootm_handler);
	if (IS_ENABLED(CONFIG_ZSTD_DECOMPRESS))
		register_image_handler(&zstd_bootm_handler);

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
