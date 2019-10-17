/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <bootm.h>
#include <fs.h>
#include <malloc.h>
#include <memory.h>
#include <libfile.h>
#include <image-fit.h>
#include <globalvar.h>
#include <init.h>
#include <environment.h>
#include <linux/stat.h>
#include <magicvar.h>

static LIST_HEAD(handler_list);

int register_image_handler(struct image_handler *handler)
{
	list_add_tail(&handler->list, &handler_list);

	return 0;
}

static struct image_handler *bootm_find_handler(enum filetype filetype,
		struct image_data *data)
{
	struct image_handler *handler;

	list_for_each_entry(handler, &handler_list, list) {
		if (filetype != filetype_uimage &&
				handler->filetype == filetype)
			return handler;
		if  (filetype == filetype_uimage &&
				handler->ih_os == data->os->header.ih_os)
			return handler;
	}

	return NULL;
}

static int bootm_appendroot;
static int bootm_provide_machine_id;
static int bootm_verbosity;

void bootm_data_init_defaults(struct bootm_data *data)
{
	data->initrd_address = UIMAGE_INVALID_ADDRESS;
	data->os_address = UIMAGE_SOME_ADDRESS;
	data->oftree_file = getenv_nonempty("global.bootm.oftree");
	data->tee_file = getenv_nonempty("global.bootm.tee");
	data->os_file = getenv_nonempty("global.bootm.image");
	getenv_ul("global.bootm.image.loadaddr", &data->os_address);
	getenv_ul("global.bootm.initrd.loadaddr", &data->initrd_address);
	data->initrd_file = getenv_nonempty("global.bootm.initrd");
	data->verify = bootm_get_verify_mode();
	data->appendroot = bootm_appendroot;
	data->provide_machine_id = bootm_provide_machine_id;
	data->verbose = bootm_verbosity;
}

static enum bootm_verify bootm_verify_mode = BOOTM_VERIFY_HASH;

enum bootm_verify bootm_get_verify_mode(void)
{
	return bootm_verify_mode;
}

static const char * const bootm_verify_names[] = {
#ifndef CONFIG_BOOTM_FORCE_SIGNED_IMAGES
	[BOOTM_VERIFY_NONE] = "none",
	[BOOTM_VERIFY_HASH] = "hash",
	[BOOTM_VERIFY_AVAILABLE] = "available",
#endif
	[BOOTM_VERIFY_SIGNATURE] = "signature",
};

static int uimage_part_num(const char *partname)
{
	if (!partname)
		return 0;
	return simple_strtoul(partname, NULL, 0);
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

	if (data->os_fit) {
		const void *kernel = data->fit_kernel;
		unsigned long kernel_size = data->fit_kernel_size;

		data->os_res = request_sdram_region("kernel",
				load_address, kernel_size);
		if (!data->os_res)
			return -ENOMEM;
		memcpy((void *)load_address, kernel, kernel_size);
		return 0;
	}

	if (data->os) {
		int num;

		num = uimage_part_num(data->os_part);

		data->os_res = uimage_load_to_sdram(data->os,
			num, load_address);
		if (!data->os_res)
			return -ENOMEM;

		return 0;
	}

	if (data->os_file) {
		data->os_res = file_to_sdram(data->os_file, load_address);
		if (!data->os_res)
			return -ENOMEM;

		return 0;
	}

	return -EINVAL;
}

bool bootm_has_initrd(struct image_data *data)
{
	if (!IS_ENABLED(CONFIG_BOOTM_INITRD))
		return false;

	if (IS_ENABLED(CONFIG_FITIMAGE) && data->os_fit &&
	    fit_has_image(data->os_fit, data->fit_config, "ramdisk"))
		return true;

	if (data->initrd_file)
		return true;

	return false;
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
				printf("Checking data crc failed with %s\n",
					strerror(-ret));
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
int bootm_load_initrd(struct image_data *data, unsigned long load_address)
{
	enum filetype type;
	int ret;

	if (!IS_ENABLED(CONFIG_BOOTM_INITRD))
		return -ENOSYS;

	if (!bootm_has_initrd(data))
		return -EINVAL;

	if (data->initrd_res)
		return 0;

	if (IS_ENABLED(CONFIG_FITIMAGE) && data->os_fit &&
	    fit_has_image(data->os_fit, data->fit_config, "ramdisk")) {
		const void *initrd;
		unsigned long initrd_size;

		ret = fit_open_image(data->os_fit, data->fit_config, "ramdisk",
				     &initrd, &initrd_size);

		data->initrd_res = request_sdram_region("initrd",
				load_address,
				initrd_size);
		if (!data->initrd_res)
			return -ENOMEM;
		memcpy((void *)load_address, initrd, initrd_size);
		printf("Loaded initrd from FIT image\n");
		goto done1;
	}

	type = file_name_detect_type(data->initrd_file);

	if ((int)type < 0) {
		printf("could not open %s: %s\n", data->initrd_file,
				strerror(-type));
		return (int)type;
	}

	if (type == filetype_uimage) {
		int num;
		ret = bootm_open_initrd_uimage(data);
		if (ret) {
			printf("loading initrd failed with %s\n",
					strerror(-ret));
			return ret;
		}

		num = uimage_part_num(data->initrd_part);

		data->initrd_res = uimage_load_to_sdram(data->initrd,
			num, load_address);
		if (!data->initrd_res)
			return -ENOMEM;

		goto done;
	}

	data->initrd_res = file_to_sdram(data->initrd_file, load_address);
	if (!data->initrd_res)
		return -ENOMEM;

done:

	printf("Loaded initrd %s '%s'", file_type_to_string(type),
	       data->initrd_file);
	if (type == filetype_uimage && data->initrd->header.ih_type == IH_TYPE_MULTI)
		printf(", multifile image %s", data->initrd_part);
	printf("\n");
done1:
	printf("initrd is at %pa-%pa\n",
		&data->initrd_res->start,
		&data->initrd_res->end);

	return 0;
}

static int bootm_open_oftree_uimage(struct image_data *data, size_t *size,
				    struct fdt_header **fdt)
{
	enum filetype ft;
	const char *oftree = data->oftree_file;
	int num = uimage_part_num(data->oftree_part);
	struct uimage_handle *of_handle;
	int release = 0;

	printf("Loading devicetree from '%s'@%d\n", oftree, num);

	if (!IS_ENABLED(CONFIG_BOOTM_OFTREE_UIMAGE))
		return -EINVAL;

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
		printf("%s is not an oftree but %s\n",
			data->oftree_file, file_type_to_string(ft));
		free(*fdt);
		return -EINVAL;
	}

	return 0;
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
 * Return: pointer to the fixed devicetree or a ERR_PTR() on failure.
 */
void *bootm_get_devicetree(struct image_data *data)
{
	enum filetype type;
	struct fdt_header *oftree;
	int ret;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return ERR_PTR(-ENOSYS);

	if (IS_ENABLED(CONFIG_FITIMAGE) && data->os_fit &&
	    fit_has_image(data->os_fit, data->fit_config, "fdt")) {
		const void *of_tree;
		unsigned long of_size;

		ret = fit_open_image(data->os_fit, data->fit_config, "fdt",
				     &of_tree, &of_size);
		if (ret)
			return ERR_PTR(ret);

		data->of_root_node = of_unflatten_dtb(of_tree);
	} else if (data->oftree_file) {
		size_t size;

		type = file_name_detect_type(data->oftree_file);

		if ((int)type < 0) {
			printf("could not open %s: %s\n", data->oftree_file,
					strerror(-type));
			return ERR_PTR((int)type);
		}

		switch (type) {
		case filetype_uimage:
			ret = bootm_open_oftree_uimage(data, &size, &oftree);
			break;
		case filetype_oftree:
			printf("Loading devicetree from '%s'\n", data->oftree_file);
			ret = read_file_2(data->oftree_file, &size, (void *)&oftree,
					  FILESIZE_MAX);
			break;
		default:
			return ERR_PTR(-EINVAL);
		}

		if (ret)
			return ERR_PTR(ret);

		data->of_root_node = of_unflatten_dtb(oftree);

		free(oftree);

		if (IS_ERR(data->of_root_node)) {
			data->of_root_node = NULL;
			pr_err("unable to unflatten devicetree\n");
			return ERR_PTR(-EINVAL);
		}

	} else {
		data->of_root_node = of_get_root_node();
		if (!data->of_root_node)
			return NULL;

		if (bootm_verbose(data) > 1 && data->of_root_node)
			printf("using internal devicetree\n");
	}

	if (data->initrd_res) {
		of_add_initrd(data->of_root_node, data->initrd_res->start,
				data->initrd_res->end);
		of_add_reserve_entry(data->initrd_res->start, data->initrd_res->end);
	}

	oftree = of_get_fixed_tree(data->of_root_node);
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
			fdt_size);
	if (!data->oftree_res)
		return -ENOMEM;

	memcpy((void *)data->oftree_res->start, fdt, fdt_size);

	of_print_cmdline(data->of_root_node);
	if (bootm_verbose(data) > 1)
		of_print_nodes(data->of_root_node, 0);

	return 0;
}

int bootm_get_os_size(struct image_data *data)
{
	int ret;

	if (data->os)
		return uimage_get_size(data->os, uimage_part_num(data->os_part));
	if (data->os_fit)
		return data->fit_kernel_size;

	if (data->os_file) {
		struct stat s;
		ret = stat(data->os_file, &s);
		if (ret)
			return ret;
		return s.st_size;
	}

	return -EINVAL;
}

static int bootm_open_os_uimage(struct image_data *data)
{
	int ret;

	data->os = uimage_open(data->os_file);
	if (!data->os)
		return -EINVAL;

	if (bootm_get_verify_mode() > BOOTM_VERIFY_NONE) {
		ret = uimage_verify(data->os);
		if (ret) {
			printf("Checking data crc failed with %s\n",
					strerror(-ret));
			return ret;
		}
	}

	uimage_print_contents(data->os);

	if (data->os->header.ih_arch != IH_ARCH) {
		printf("Unsupported Architecture 0x%x\n",
		       data->os->header.ih_arch);
		return -EINVAL;
	}

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = data->os->header.ih_load;

	return 0;
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

	if (!bootm_data->os_file) {
		printf("no image given\n");
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

	ret = read_file_2(data->os_file, &size, &data->os_header, PAGE_SIZE);
	if (ret < 0 && ret != -EFBIG) {
		printf("could not open %s: %s\n", data->os_file,
				strerror(-ret));
		goto err_out;
	}
	if (size < PAGE_SIZE)
		goto err_out;

	os_type = file_detect_type(data->os_header, PAGE_SIZE);

	if (!data->force && os_type == filetype_unknown) {
		printf("Unknown OS filetype (try -f)\n");
		ret = -EINVAL;
		goto err_out;
	}

	if (IS_ENABLED(CONFIG_BOOTM_FORCE_SIGNED_IMAGES)) {
		data->verify = BOOTM_VERIFY_SIGNATURE;

		/*
		 * When we only allow booting signed images make sure everything
		 * we boot is in the OS image and not given separately.
		 */
		data->oftree_file = NULL;
		data->initrd_file = NULL;
		if (os_type != filetype_oftree) {
			printf("Signed boot and image is no FIT image, aborting\n");
			ret = -EINVAL;
			goto err_out;
		}
	}

	if (IS_ENABLED(CONFIG_FITIMAGE) && os_type == filetype_oftree) {
		struct fit_handle *fit;

		fit = fit_open(data->os_file, data->verbose, data->verify);
		if (IS_ERR(fit)) {
			printf("Loading FIT image %s failed with: %s\n", data->os_file,
			       strerrorp(fit));
			ret = PTR_ERR(fit);
			goto err_out;
		}

		data->os_fit = fit;

		data->fit_config = fit_open_configuration(data->os_fit,
							  data->os_part);
		if (IS_ERR(data->fit_config)) {
			printf("Cannot open FIT image configuration '%s'\n",
			       data->os_part ? data->os_part : "default");
			ret = PTR_ERR(data->fit_config);
			goto err_out;
		}

		ret = fit_open_image(data->os_fit, data->fit_config, "kernel",
				     &data->fit_kernel, &data->fit_kernel_size);
		if (ret)
			goto err_out;
	}

	if (os_type == filetype_uimage) {
		ret = bootm_open_os_uimage(data);
		if (ret) {
			printf("Loading OS image failed with: %s\n",
					strerror(-ret));
			goto err_out;
		}
	}

	if (bootm_data->appendroot) {
		char *rootarg;

		rootarg = path_get_linux_rootarg(data->os_file);
		if (!IS_ERR(rootarg)) {
			printf("Adding \"%s\" to Kernel commandline\n", rootarg);
			globalvar_add_simple("linux.bootargs.bootm.appendroot",
					     rootarg);
			free(rootarg);
		}
	}

	if (bootm_data->provide_machine_id) {
		const char *machine_id = getenv_nonempty("global.machine_id");
		char *machine_id_bootarg;

		if (!machine_id) {
			printf("Providing machine id is enabled but no machine id set\n");
			ret = -EINVAL;
			goto err_out;
		}

		machine_id_bootarg = basprintf("systemd.machine_id=%s", machine_id);
		globalvar_add_simple("linux.bootargs.machine_id", machine_id_bootarg);
		free(machine_id_bootarg);
	}

	printf("\nLoading %s '%s'", file_type_to_string(os_type),
			data->os_file);
	if (os_type == filetype_uimage &&
			data->os->header.ih_type == IH_TYPE_MULTI)
		printf(", multifile image %d", uimage_part_num(data->os_part));
	printf("\n");

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = UIMAGE_INVALID_ADDRESS;

	handler = bootm_find_handler(os_type, data);
	if (!handler) {
		printf("no image handler found for image type %s\n",
			file_type_to_string(os_type));
		if (os_type == filetype_uimage)
			printf("and OS type: %d\n", data->os->header.ih_os);
		ret = -ENODEV;
		goto err_out;
	}

	if (bootm_verbose(data)) {
		bootm_print_info(data);
		printf("Passing control to %s handler\n", handler->name);
	}

	ret = handler->bootm(data);
	if (data->dryrun)
		printf("Dryrun. Aborted\n");

err_out:
	if (data->os_res)
		release_sdram_region(data->os_res);
	if (data->initrd_res)
		release_sdram_region(data->initrd_res);
	if (data->oftree_res)
		release_sdram_region(data->oftree_res);
	if (data->tee_res)
		release_sdram_region(data->tee_res);
	if (data->initrd && data->initrd != data->os)
		uimage_close(data->initrd);
	if (data->os)
		uimage_close(data->os);
	if (IS_ENABLED(CONFIG_FITIMAGE) && data->os_fit)
		fit_close(data->os_fit);
	if (data->of_root_node && data->of_root_node != of_get_root_node())
		of_delete_node(data->of_root_node);

	globalvar_remove("linux.bootargs.bootm.appendroot");
	free(data->os_header);
	free(data->os_file);
	free(data->oftree_file);
	free(data->initrd_file);
	free(data->tee_file);
	free(data);

	return ret;
}

static int bootm_init(void)
{
	globalvar_add_simple("bootm.image", NULL);
	globalvar_add_simple("bootm.image.loadaddr", NULL);
	globalvar_add_simple("bootm.oftree", NULL);
	globalvar_add_simple("bootm.tee", NULL);
	globalvar_add_simple_bool("bootm.appendroot", &bootm_appendroot);
	globalvar_add_simple_bool("bootm.provide_machine_id", &bootm_provide_machine_id);
	if (IS_ENABLED(CONFIG_BOOTM_INITRD)) {
		globalvar_add_simple("bootm.initrd", NULL);
		globalvar_add_simple("bootm.initrd.loadaddr", NULL);
	}

	if (IS_ENABLED(CONFIG_BOOTM_FORCE_SIGNED_IMAGES))
		bootm_verify_mode = BOOTM_VERIFY_SIGNATURE;

	globalvar_add_simple_int("bootm.verbose", &bootm_verbosity, "%u");

	globalvar_add_simple_enum("bootm.verify", (unsigned int *)&bootm_verify_mode,
				  bootm_verify_names, ARRAY_SIZE(bootm_verify_names));

	return 0;
}
late_initcall(bootm_init);

BAREBOX_MAGICVAR(bootargs, "Linux kernel parameters");
BAREBOX_MAGICVAR_NAMED(global_bootm_image, global.bootm.image, "bootm default boot image");
BAREBOX_MAGICVAR_NAMED(global_bootm_image_loadaddr, global.bootm.image.loadaddr, "bootm default boot image loadaddr");
BAREBOX_MAGICVAR_NAMED(global_bootm_initrd, global.bootm.initrd, "bootm default initrd");
BAREBOX_MAGICVAR_NAMED(global_bootm_initrd_loadaddr, global.bootm.initrd.loadaddr, "bootm default initrd loadaddr");
BAREBOX_MAGICVAR_NAMED(global_bootm_oftree, global.bootm.oftree, "bootm default oftree");
BAREBOX_MAGICVAR_NAMED(global_bootm_tee, global.bootm.tee, "bootm default tee image");
BAREBOX_MAGICVAR_NAMED(global_bootm_verify, global.bootm.verify, "bootm default verify level");
BAREBOX_MAGICVAR_NAMED(global_bootm_verbose, global.bootm.verbose, "bootm default verbosity level (0=quiet)");
BAREBOX_MAGICVAR_NAMED(global_bootm_appendroot, global.bootm.appendroot, "Add root= option to Kernel to mount rootfs from the device the Kernel comes from");
BAREBOX_MAGICVAR_NAMED(global_bootm_provide_machine_id, global.bootm.provide_machine_id, "If true, add systemd.machine_id= with value of global.machine_id to Kernel");
