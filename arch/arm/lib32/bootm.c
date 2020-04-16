#include <bootm.h>
#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <init.h>
#include <fs.h>
#include <libfile.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/sizes.h>
#include <libbb.h>
#include <magicvar.h>
#include <binfmt.h>
#include <restart.h>
#include <globalvar.h>
#include <tee/optee.h>

#include <asm/byteorder.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

/* If true, ignore device tree and boot with ATAGs */
static int bootm_boot_atag;

/*
 * sdram_start_and_size() - determine place for putting the kernel/oftree/initrd
 *
 * @start:	returns the start address of the first RAM bank
 * @size:	returns the usable space at the beginning of the first RAM bank
 *
 * This function returns the base address of the first RAM bank and the free
 * space found there.
 *
 * return: 0 for success, negative error code otherwise
 */
static int sdram_start_and_size(unsigned long *start, unsigned long *size)
{
	struct memory_bank *bank;
	struct resource *res;

	/*
	 * We use the first memory bank for the kernel and other resources
	 */
	bank = list_first_entry_or_null(&memory_banks, struct memory_bank,
			list);
	if (!bank) {
		printf("cannot find first memory bank\n");
		return -EINVAL;
	}

	/*
	 * If the first memory bank has child resources we can use the bank up
	 * to the beginning of the first child resource, otherwise we can use
	 * the whole bank.
	 */
	res = list_first_entry_or_null(&bank->res->children, struct resource,
			sibling);
	if (res)
		*size = res->start - bank->start;
	else
		*size = bank->size;

	*start = bank->start;

	return 0;
}

static int get_kernel_addresses(size_t image_size,
				 int verbose, unsigned long *load_address,
				 unsigned long *mem_free)
{
	unsigned long mem_start, mem_size;
	int ret;
	size_t image_decomp_size;
	unsigned long spacing;

	ret = sdram_start_and_size(&mem_start, &mem_size);
	if (ret)
		return ret;

	/*
	 * The kernel documentation "Documentation/arm/Booting" advises
	 * to place the compressed image outside of the lowest 32 MiB to
	 * avoid relocation. We should do this if we have at least 64 MiB
	 * of ram. If we have less space, we assume a maximum
	 * compression factor of 5.
	 */
	image_decomp_size = PAGE_ALIGN(image_size * 5);
	if (mem_size >= SZ_64M)
		image_decomp_size = max_t(size_t, image_decomp_size, SZ_32M);

	/*
	 * By default put oftree/initrd close behind compressed kernel image to
	 * avoid placing it outside of the kernels lowmem region.
	 */
	spacing = SZ_1M;

	if (*load_address == UIMAGE_INVALID_ADDRESS) {
		unsigned long mem_end = mem_start + mem_size - 1;
		unsigned long kaddr;

		/*
		 * Place the kernel at an address where it does not need to
		 * relocate itself before decompression.
		 */
		kaddr = mem_start + image_decomp_size;

		/*
		 * Make sure we do not place the image past the end of the
		 * available memory.
		 */
		if (kaddr + image_size + spacing >= mem_end)
			kaddr = mem_end - image_size - spacing;

		*load_address = PAGE_ALIGN_DOWN(kaddr);

		if (verbose)
			printf("no OS load address, defaulting to 0x%08lx\n",
				*load_address);
	} else if (*load_address <= mem_start + image_decomp_size) {
		/*
		 * If the user/image specified an address where the kernel needs
		 * to relocate itself before decompression we need to extend the
		 * spacing to allow this relocation to happen without
		 * overwriting anything placed behind the kernel.
		 */
		spacing += image_decomp_size;
	}

	*mem_free = PAGE_ALIGN(*load_address + image_size + spacing);

	/*
	 * Place oftree/initrd outside of the first 128 MiB, if we have space
	 * for it. This avoids potential conflicts with the kernel decompressor.
	 */
	if (mem_size > SZ_256M)
		*mem_free = max(*mem_free, mem_start + SZ_128M);

	return 0;
}

static int optee_verify_header_request_region(struct image_data *data, struct optee_header *hdr)
{
	int ret = 0;

	ret = optee_verify_header(hdr);
	if (ret < 0)
		return ret;

	data->tee_res = request_sdram_region("TEE", hdr->init_load_addr_lo, hdr->init_size);
	if (!data->tee_res) {
		pr_err("Cannot request SDRAM region 0x%08x-0x%08x: %s\n",
		       hdr->init_load_addr_lo, hdr->init_load_addr_lo + hdr->init_size - 1,
		       strerror(-EINVAL));
		return -EINVAL;
	}

	return ret;
}

static int bootm_load_tee_from_file(struct image_data *data)
{
	int fd, ret;
	struct optee_header hdr;

	fd = open(data->tee_file, O_RDONLY);
	if (fd < 0) {
		pr_err("%s", strerror(errno));
		return -errno;
	}

	if (read_full(fd, &hdr, sizeof(hdr)) < 0) {
		pr_err("%s", strerror(errno));
		ret = -errno;
		goto out;
	}

	if (optee_verify_header_request_region(data, &hdr) < 0) {
		pr_err("%s", strerror(errno));
		ret = -errno;
		goto out;
	}

	if (read_full(fd, (void *)data->tee_res->start, hdr.init_size) < 0) {
		pr_err("%s", strerror(errno));
		ret = -errno;
		release_region(data->tee_res);
		goto out;
	}

	printf("Read optee file to %pa, size 0x%08x\n", (void *)data->tee_res->start, hdr.init_size);

	ret = 0;
out:
	close(fd);

	return ret;
}

static int __do_bootm_linux(struct image_data *data, unsigned long free_mem,
			    int swap, void *fdt)
{
	unsigned long kernel;
	unsigned long initrd_start = 0, initrd_size = 0, initrd_end = 0;
	void *tee;
	enum arm_security_state state = bootm_arm_security_state();
	void *fdt_load_address = NULL;
	int ret;

	kernel = data->os_res->start + data->os_entry;

	initrd_start = data->initrd_address;

	if (initrd_start == UIMAGE_INVALID_ADDRESS) {
		initrd_start = PAGE_ALIGN(free_mem);

		if (bootm_verbose(data)) {
			printf("no initrd load address, defaulting to 0x%08lx\n",
				initrd_start);
		}
	}

	if (bootm_has_initrd(data)) {
		ret = bootm_load_initrd(data, initrd_start);
		if (ret)
			return ret;
	}

	if (data->initrd_res) {
		initrd_start = data->initrd_res->start;
		initrd_end = data->initrd_res->end;
		initrd_size = resource_size(data->initrd_res);
		free_mem = PAGE_ALIGN(initrd_end + 1);
	}

	if (fdt && bootm_boot_atag) {
		printf("Error: Boot with ATAGs forced, but kernel has an appended device tree\n");
		return -EINVAL;
	}

	if (!fdt && !bootm_boot_atag) {
		fdt = bootm_get_devicetree(data);
		if (IS_ERR(fdt))
			return PTR_ERR(fdt);
	}

	if (fdt) {
		fdt_load_address = (void *)free_mem;
		ret = bootm_load_devicetree(data, fdt, free_mem);

		free(fdt);

		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_BOOTM_OPTEE) && data->tee_file) {
		ret = bootm_load_tee_from_file(data);
		if (ret)
			return ret;
	}


	if (bootm_verbose(data)) {
		printf("\nStarting kernel at 0x%08lx", kernel);
		if (initrd_size)
			printf(", initrd at 0x%08lx", initrd_start);
		if (fdt_load_address)
			printf(", oftree at 0x%p", fdt_load_address);
		printf("...\n");
	}

	if (IS_ENABLED(CONFIG_ARM_SECURE_MONITOR)) {
		if (file_detect_type((void *)data->os_res->start, 0x100) ==
		    filetype_arm_barebox)
			state = ARM_STATE_SECURE;

		printf("Starting kernel in %s mode\n",
		       bootm_arm_security_state_name(state));
	}

	if (data->dryrun)
		return 0;

	if (data->tee_res)
		tee = (void *)data->tee_res->start;
	else
		tee = NULL;

	start_linux((void *)kernel, swap, initrd_start, initrd_size,
		    fdt_load_address, state, tee);

	restart_machine();

	return -ERESTARTSYS;
}

static int do_bootm_linux(struct image_data *data)
{
	unsigned long load_address, mem_free;
	int ret;

	load_address = data->os_address;

	ret = get_kernel_addresses(bootm_get_os_size(data),
			     bootm_verbose(data), &load_address, &mem_free);
	if (ret)
		return ret;

	ret = bootm_load_os(data, load_address);
	if (ret)
		return ret;

	return __do_bootm_linux(data, mem_free, 0, NULL);
}

static struct image_handler uimage_handler = {
	.name = "ARM Linux uImage",
	.bootm = do_bootm_linux,
	.filetype = filetype_uimage,
	.ih_os = IH_OS_LINUX,
};

static struct image_handler rawimage_handler = {
	.name = "ARM raw image",
	.bootm = do_bootm_linux,
	.filetype = filetype_unknown,
};

struct zimage_header {
	u32	unused[9];
	u32	magic;
	u32	start;
	u32	end;
};

#define ZIMAGE_MAGIC 0x016F2818

static int do_bootz_linux_fdt(int fd, struct image_data *data, void **outfdt)
{
	struct fdt_header __header, *header;
	void *oftree;
	int ret;

	u32 end;

	header = &__header;
	ret = read(fd, header, sizeof(*header));
	if (ret < 0)
		return ret;
	if (ret < sizeof(*header))
		return -ENXIO;

	if (file_detect_type(header, sizeof(*header)) != filetype_oftree)
		return -ENXIO;

	end = be32_to_cpu(header->totalsize);

	oftree = malloc(end + 0x8000);
	if (!oftree) {
		perror("zImage: oftree malloc");
		return -ENOMEM;
	}

	memcpy(oftree, header, sizeof(*header));

	end -= sizeof(*header);

	ret = read_full(fd, oftree + sizeof(*header), end);
	if (ret < 0)
		goto err_free;
	if (ret < end) {
		printf("premature end of image\n");
		ret = -EIO;
		goto err_free;
	}

	if (IS_BUILTIN(CONFIG_OFTREE)) {
		struct device_node *root;

		root = of_unflatten_dtb(oftree);
		if (IS_ERR(root)) {
			pr_err("unable to unflatten devicetree\n");
			goto err_free;
		}
		*outfdt = of_get_fixed_tree(root);
		if (!*outfdt) {
			pr_err("Unable to get fixed tree\n");
			ret = -EINVAL;
			goto err_free;
		}

		free(oftree);
	} else {
		*outfdt = oftree;
	}

	pr_info("zImage: concatenated oftree detected\n");

	return 0;

err_free:
	free(oftree);

	return ret;
}

static int do_bootz_linux(struct image_data *data)
{
	int fd, ret, swap = 0;
	struct zimage_header __header, *header;
	void *zimage;
	u32 end, start;
	size_t image_size;
	unsigned long load_address = data->os_address;
	unsigned long mem_free;
	void *fdt = NULL;

	fd = open(data->os_file, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	header = &__header;
	ret = read(fd, header, sizeof(*header));
	if (ret < sizeof(*header)) {
		printf("could not read %s\n", data->os_file);
		goto err_out;
	}

	switch (header->magic) {
	case swab32(ZIMAGE_MAGIC):
		swap = 1;
		/* fall through */
	case ZIMAGE_MAGIC:
		break;
	default:
		printf("invalid magic 0x%08x\n", header->magic);
		ret = -EINVAL;
		goto err_out;
	}

	end = header->end;
	start = header->start;

	if (swap) {
		end = swab32(end);
		start = swab32(start);
	}

	image_size = end - start;
	load_address = data->os_address;

	ret = get_kernel_addresses(image_size, bootm_verbose(data),
			     &load_address, &mem_free);
	if (ret)
		return ret;

	data->os_res = request_sdram_region("zimage", load_address, image_size);
	if (!data->os_res) {
		pr_err("bootm/zImage: failed to request memory at 0x%lx to 0x%lx (%zu).\n",
		       load_address, load_address + image_size, image_size);
		ret = -ENOMEM;
		goto err_out;
	}

	zimage = (void *)data->os_res->start;

	memcpy(zimage, header, sizeof(*header));

	ret = read_full(fd, zimage + sizeof(*header),
			image_size - sizeof(*header));
	if (ret < 0)
		goto err_out;
	if (ret < image_size - sizeof(*header)) {
		printf("premature end of image\n");
		ret = -EIO;
		goto err_out;
	}

	if (swap) {
		void *ptr;
		for (ptr = zimage; ptr < zimage + end; ptr += 4)
			*(u32 *)ptr = swab32(*(u32 *)ptr);
	}

	ret = do_bootz_linux_fdt(fd, data, &fdt);
	if (ret && ret != -ENXIO)
		goto err_out;

	close(fd);

	return __do_bootm_linux(data, mem_free, swap, fdt);

err_out:
	close(fd);

	return ret;
}

static struct image_handler zimage_handler = {
	.name = "ARM zImage",
	.bootm = do_bootz_linux,
	.filetype = filetype_arm_zimage,
};

static struct image_handler barebox_handler = {
	.name = "ARM barebox",
	.bootm = do_bootm_linux,
	.filetype = filetype_arm_barebox,
};

static struct image_handler socfpga_xload_handler = {
	.name = "SoCFPGA prebootloader image",
	.bootm = do_bootm_linux,
	.filetype = filetype_socfpga_xload,
};

#include <aimage.h>

static int aimage_load_resource(int fd, struct resource *r, void* buf, int ps)
{
	int ret;
	void *image = (void *)r->start;
	unsigned to_read = ps - resource_size(r) % ps;

	ret = read_full(fd, image, resource_size(r));
	if (ret < 0)
		return ret;

	ret = read_full(fd, buf, to_read);
	if (ret < 0)
		printf("could not read dummy %u\n", to_read);

	return ret;
}

static int do_bootm_aimage(struct image_data *data)
{
	struct resource *snd_stage_res;
	int fd, ret;
	struct android_header __header, *header;
	void *buf;
	int to_read;
	struct android_header_comp *cmp;
	unsigned long mem_free;
	unsigned long mem_start, mem_size;

	ret = sdram_start_and_size(&mem_start, &mem_size);
	if (ret)
		return ret;

	fd = open(data->os_file, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	header = &__header;
	ret = read(fd, header, sizeof(*header));
	if (ret < sizeof(*header)) {
		printf("could not read %s\n", data->os_file);
		goto err_out;
	}

	printf("Android Image for '%s'\n", header->name);

	/*
	 * As on tftp we do not support lseek and we will just have to seek
	 * for the size of a page - 1 max just buffer instead to read to dummy
	 * data
	 */
	buf = xmalloc(header->page_size);

	to_read = header->page_size - sizeof(*header);
	ret = read_full(fd, buf, to_read);
	if (ret < 0) {
		printf("could not read dummy %d from %s\n", to_read, data->os_file);
		goto err_out;
	}

	cmp = &header->kernel;
	data->os_res = request_sdram_region("akernel", cmp->load_addr, cmp->size);
	if (!data->os_res) {
		pr_err("Cannot request region 0x%08x - 0x%08x, using default load address\n",
				cmp->load_addr, cmp->size);

		data->os_address = mem_start + PAGE_ALIGN(cmp->size * 4);
		data->os_res = request_sdram_region("akernel", data->os_address, cmp->size);
		if (!data->os_res) {
			pr_err("Cannot request region 0x%08x - 0x%08x\n",
					cmp->load_addr, cmp->size);
			ret = -ENOMEM;
			goto err_out;
		}
	}

	ret = aimage_load_resource(fd, data->os_res, buf, header->page_size);
	if (ret < 0) {
		perror("could not read kernel");
		goto err_out;
	}

	/*
	 * fastboot always expect a ramdisk
	 * in barebox we can be less restrictive
	 */
	cmp = &header->ramdisk;
	if (cmp->size) {
		data->initrd_res = request_sdram_region("ainitrd", cmp->load_addr, cmp->size);
		if (!data->initrd_res) {
			ret = -ENOMEM;
			goto err_out;
		}

		ret = aimage_load_resource(fd, data->initrd_res, buf, header->page_size);
		if (ret < 0) {
			perror("could not read initrd");
			goto err_out;
		}
	}

	if (!getenv("aimage_noverwrite_bootargs"))
		linux_bootargs_overwrite(header->cmdline);

	if (!getenv("aimage_noverwrite_tags"))
		armlinux_set_bootparams((void *)(unsigned long)header->tags_addr);

	cmp = &header->second_stage;
	if (cmp->size) {
		void (*second)(void);

		snd_stage_res = request_sdram_region("asecond", cmp->load_addr, cmp->size);
		if (!snd_stage_res) {
			ret = -ENOMEM;
			goto err_out;
		}

		ret = aimage_load_resource(fd, snd_stage_res, buf, header->page_size);
		if (ret < 0) {
			perror("could not read initrd");
			goto err_out;
		}

		second = (void*)snd_stage_res->start;
		shutdown_barebox();

		second();

		restart_machine();
	}

	close(fd);

	/*
	 * Put devicetree right after initrd if present or after the kernel
	 * if not.
	 */
	if (data->initrd_res)
		mem_free = PAGE_ALIGN(data->initrd_res->end);
	else
		mem_free = PAGE_ALIGN(data->os_res->end + SZ_1M);

	return __do_bootm_linux(data, mem_free, 0, NULL);

err_out:
	linux_bootargs_overwrite(NULL);
	close(fd);

	return ret;
}

static struct image_handler aimage_handler = {
	.name = "ARM Android Image",
	.bootm = do_bootm_aimage,
	.filetype = filetype_aimage,
};

#ifdef CONFIG_BOOTM_AIMAGE
BAREBOX_MAGICVAR(aimage_noverwrite_bootargs, "Disable overwrite of the bootargs with the one present in aimage");
BAREBOX_MAGICVAR(aimage_noverwrite_tags, "Disable overwrite of the tags addr with the one present in aimage");
#endif

static struct image_handler arm_fit_handler = {
        .name = "FIT image",
        .bootm = do_bootm_linux,
        .filetype = filetype_oftree,
};

static struct binfmt_hook binfmt_aimage_hook = {
	.type = filetype_aimage,
	.exec = "bootm",
};

static struct binfmt_hook binfmt_arm_zimage_hook = {
	.type = filetype_arm_zimage,
	.exec = "bootm",
};

static struct binfmt_hook binfmt_barebox_hook = {
	.type = filetype_arm_barebox,
	.exec = "bootm",
};

BAREBOX_MAGICVAR_NAMED(global_bootm_boot_atag, global.bootm.boot_atag,
		       "If true, ignore device tree and boot using ATAGs");

static int armlinux_register_image_handler(void)
{
	globalvar_add_simple_bool("bootm.boot_atag", &bootm_boot_atag);

	register_image_handler(&barebox_handler);
	register_image_handler(&socfpga_xload_handler);
	register_image_handler(&uimage_handler);
	register_image_handler(&rawimage_handler);
	register_image_handler(&zimage_handler);
	if (IS_BUILTIN(CONFIG_BOOTM_AIMAGE)) {
		register_image_handler(&aimage_handler);
		binfmt_register(&binfmt_aimage_hook);
	}
	if (IS_BUILTIN(CONFIG_FITIMAGE))
	        register_image_handler(&arm_fit_handler);
	binfmt_register(&binfmt_arm_zimage_hook);
	binfmt_register(&binfmt_barebox_hook);

	return 0;
}
late_initcall(armlinux_register_image_handler);
