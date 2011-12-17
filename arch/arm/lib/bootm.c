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

#include <asm/byteorder.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

static int __do_bootm_linux(struct image_data *data, int swap)
{
	unsigned long kernel;
	unsigned long initrd_start = 0, initrd_size = 0;
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

	if (data->initrd_file && data->initrd_address == UIMAGE_INVALID_ADDRESS) {
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
		initrd_size = data->initrd_res->size;
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

static int armlinux_register_image_handler(void)
{
	register_image_handler(&barebox_handler);
	register_image_handler(&uimage_handler);
	register_image_handler(&rawimage_handler);
	register_image_handler(&zimage_handler);

	return 0;
}
late_initcall(armlinux_register_image_handler);
