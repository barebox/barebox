#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <init.h>
#include <fs.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <sizes.h>
#include <libbb.h>
#include <magicvar.h>
#include <binfmt.h>

#include <asm/byteorder.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

static int __do_bootm_linux(struct image_data *data, int swap)
{
	unsigned long kernel;
	unsigned long initrd_start = 0, initrd_size = 0, initrd_end = 0;
	struct memory_bank *bank;
	unsigned long load_address;

	if (data->os_res) {
		load_address = data->os_res->start;
	} else if (data->os_address != UIMAGE_INVALID_ADDRESS) {
		load_address = data->os_address;
	} else {
		bank = list_first_entry(&memory_banks,
				struct memory_bank, list);
		load_address = bank->start + SZ_32K;
		if (bootm_verbose(data))
			printf("no os load address, defaulting to 0x%08lx\n",
				load_address);
	}

	if (!data->os_res && data->os) {
		data->os_res = uimage_load_to_sdram(data->os,
			data->os_num, load_address);
		if (!data->os_res)
			return -ENOMEM;
	}

	if (!data->os_res) {
		data->os_res = file_to_sdram(data->os_file, load_address);
		if (!data->os_res)
			return -ENOMEM;
	}

	kernel = data->os_res->start + data->os_entry;

	initrd_start = data->initrd_address;

	if (data->initrd_file && initrd_start == UIMAGE_INVALID_ADDRESS) {
		initrd_start = data->os_res->start + SZ_8M;

		if (bootm_verbose(data)) {
			printf("no initrd load address, defaulting to 0x%08lx\n",
				initrd_start);
		}
	}

	if (data->initrd) {
		data->initrd_res = uimage_load_to_sdram(data->initrd,
			data->initrd_num, initrd_start);
		if (!data->initrd_res)
			return -ENOMEM;
	} else if (data->initrd_file) {
		data->initrd_res = file_to_sdram(data->initrd_file, initrd_start);
		if (!data->initrd_res)
			return -ENOMEM;
	}

	if (data->initrd_res) {
		initrd_start = data->initrd_res->start;
		initrd_end = data->initrd_res->end;
		initrd_size = resource_size(data->initrd_res);
	}

	if (IS_ENABLED(CONFIG_OFTREE) && data->of_root_node) {
		of_add_initrd(data->of_root_node, initrd_start, initrd_end);
		if (initrd_end)
			of_add_reserve_entry(initrd_start, initrd_end);
		data->oftree = of_get_fixed_tree(data->of_root_node);
		fdt_add_reserve_map(data->oftree);
		of_print_cmdline(data->of_root_node);
		if (bootm_verbose(data) > 1)
			of_print_nodes(data->of_root_node, 0);
	}

	if (bootm_verbose(data)) {
		printf("\nStarting kernel at 0x%08lx", kernel);
		if (initrd_size)
			printf(", initrd at 0x%08lx", initrd_start);
		if (data->oftree)
			printf(", oftree at 0x%p", data->oftree);
		printf("...\n");
	}

	start_linux((void *)kernel, swap, initrd_start, initrd_size, data->oftree);

	reset_cpu(0);

	return -ERESTARTSYS;
}

static int do_bootm_linux(struct image_data *data)
{
	return __do_bootm_linux(data, 0);
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

static int do_bootz_linux_fdt(int fd, struct image_data *data)
{
	struct fdt_header __header, *header;
	void *oftree;
	int ret;

	u32 end;

	if (data->oftree)
		return -ENXIO;

	header = &__header;
	ret = read(fd, header, sizeof(*header));
	if (ret < sizeof(*header))
		return ret;

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
		data->of_root_node = of_unflatten_dtb(NULL, oftree);
		if (!data->of_root_node) {
			pr_err("unable to unflatten devicetree\n");
			ret = -EINVAL;
			goto err_free;
		}
	} else {
		data->oftree = oftree;
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
	u32 end;
	unsigned long load_address = data->os_address;

	if (load_address == UIMAGE_INVALID_ADDRESS) {
		struct memory_bank *bank = list_first_entry(&memory_banks,
				struct memory_bank, list);
		data->os_address = bank->start + SZ_8M;
		load_address = data->os_address;
		if (bootm_verbose(data))
			printf("no os load address, defaulting to 0x%08lx\n",
				load_address);
	}

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

	if (swap)
		end = swab32(end);

	data->os_res = request_sdram_region("zimage", load_address, end);
	if (!data->os_res) {
		pr_err("bootm/zImage: failed to request memory at 0x%lx to 0x%lx (%d).\n",
		       load_address, load_address + end, end);
		ret = -ENOMEM;
		goto err_out;
	}

	zimage = (void *)data->os_res->start;

	memcpy(zimage, header, sizeof(*header));

	ret = read_full(fd, zimage + sizeof(*header), end - sizeof(*header));
	if (ret < 0)
		goto err_out;
	if (ret < end - sizeof(*header)) {
		printf("premature end of image\n");
		ret = -EIO;
		goto err_out;
	}

	if (swap) {
		void *ptr;
		for (ptr = zimage; ptr < zimage + end; ptr += 4)
			*(u32 *)ptr = swab32(*(u32 *)ptr);
	}

	ret = do_bootz_linux_fdt(fd, data);
	if (ret && ret != -ENXIO)
		goto err_out;

	close(fd);
	return __do_bootm_linux(data, swap);

err_out:
	close(fd);

	return ret;
}

static struct image_handler zimage_handler = {
	.name = "ARM zImage",
	.bootm = do_bootz_linux,
	.filetype = filetype_arm_zimage,
};

static int do_bootm_barebox(struct image_data *data)
{
	void (*barebox)(void);

	barebox = read_file(data->os_file, NULL);
	if (!barebox)
		return -EINVAL;

	shutdown_barebox();

	barebox();

	reset_cpu(0);
}

static struct image_handler barebox_handler = {
	.name = "ARM barebox",
	.bootm = do_bootm_barebox,
	.filetype = filetype_arm_barebox,
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
		ret = -ENOMEM;
		goto err_out;
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
		armlinux_set_bootparams((void*)header->tags_addr);

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

		reset_cpu(0);
	}

	close(fd);
	return __do_bootm_linux(data, 0);

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

#ifdef CONFIG_CMD_BOOTM_AIMAGE
BAREBOX_MAGICVAR(aimage_noverwrite_bootargs, "Disable overwrite of the bootargs with the one present in aimage");
BAREBOX_MAGICVAR(aimage_noverwrite_tags, "Disable overwrite of the tags addr with the one present in aimage");
#endif

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

static int armlinux_register_image_handler(void)
{
	register_image_handler(&barebox_handler);
	register_image_handler(&uimage_handler);
	register_image_handler(&rawimage_handler);
	register_image_handler(&zimage_handler);
	if (IS_BUILTIN(CONFIG_CMD_BOOTM_AIMAGE)) {
		register_image_handler(&aimage_handler);
		binfmt_register(&binfmt_aimage_hook);
	}
	binfmt_register(&binfmt_arm_zimage_hook);
	binfmt_register(&binfmt_barebox_hook);

	return 0;
}
late_initcall(armlinux_register_image_handler);
