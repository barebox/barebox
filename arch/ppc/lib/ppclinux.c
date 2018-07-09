#define DEBUG

#include <common.h>
#include <command.h>
#include <image.h>
#include <init.h>
#include <malloc.h>
#include <environment.h>
#include <asm/bitops.h>
#include <asm/processor.h>
#include <boot.h>
#include <bootm.h>
#include <errno.h>
#include <restart.h>
#include <fs.h>

static struct fdt_header *bootm_relocate_fdt(struct image_data *data,
					     struct fdt_header *fdt)
{
	void *os = (void *)data->os_address;
	void *newfdt;

	if (os < LINUX_TLB1_MAX_ADDR) {
		/* The kernel is within  the boot TLB mapping.
		 * Put the DTB above if there is no space
		 * below.
		 */
		if (os < (void *)fdt->totalsize) {
			os = (void *)PAGE_ALIGN((phys_addr_t)os +
					data->os->header.ih_size);
			os += fdt->totalsize;
			if (os < LINUX_TLB1_MAX_ADDR)
				os = LINUX_TLB1_MAX_ADDR;
		}
	}

	if (os > LINUX_TLB1_MAX_ADDR) {
		pr_crit("Unable to relocate DTB to Linux TLB\n");
		return NULL;
	}

	newfdt = (void *)PAGE_ALIGN_DOWN((phys_addr_t)os - fdt->totalsize);
	memcpy(newfdt, fdt, fdt->totalsize);
	free(fdt);

	pr_info("Relocating device tree to 0x%p\n", newfdt);
	return newfdt;
}

static int do_bootm_linux(struct image_data *data)
{
	void	(*kernel)(void *, void *, unsigned long,
			unsigned long, unsigned long);
	int ret;
	struct fdt_header *fdt;

	ret = bootm_load_os(data, data->os_address);
	if (ret)
		return ret;

	fdt = of_get_fixed_tree(data->of_root_node);
	if (!fdt) {
		pr_err("bootm: No devicetree given.\n");
		return -EINVAL;
	}

	if (data->dryrun)
		return 0;

	/* Relocate the device tree if outside the initial
	 * Linux mapped TLB.
	 */
	if (IS_ENABLED(CONFIG_MPC85xx)) {
		if (((void *)fdt + fdt->totalsize) > LINUX_TLB1_MAX_ADDR) {
			fdt = bootm_relocate_fdt(data, fdt);
			if (!fdt)
				goto error;
		}
	}

	fdt_add_reserve_map(fdt);

	kernel = (void *)(data->os_address + data->os_entry);

	/*
	 * Linux Kernel Parameters (passing device tree):
	 *   r3: ptr to OF flat tree, followed by the board info data
	 *   r4: physical pointer to the kernel itself
	 *   r5: NULL
	 *   r6: NULL
	 *   r7: NULL
	 */
	kernel(fdt, kernel, 0, 0, 0);

	restart_machine();

error:
	return -1;
}

static struct image_handler handler = {
	.name = "PowerPC Linux",
	.bootm = do_bootm_linux,
	.filetype = filetype_uimage,
	.ih_os = IH_OS_LINUX,
};

static int ppclinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(ppclinux_register_image_handler);
