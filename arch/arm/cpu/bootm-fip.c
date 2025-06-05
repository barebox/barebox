// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "bootm-fip: " fmt

#include <bootm.h>
#include <common.h>
#include <init.h>
#include <memory.h>
#include <linux/sizes.h>
#include <asm/boot.h>
#include <fip.h>
#include <libfile.h>
#include <bootm.h>
#include <fiptool.h>

const struct fip_binding *fip_bindings;

#define for_each_fip_binding(bindings, binding) \
	for (binding = bindings; !uuid_is_null(&binding->uuid); binding++)

static inline struct resource *desc_get_res(struct fip_image_desc *desc)
{
	return desc->private_data;
}

static inline void desc_set_res(struct fip_image_desc *desc,
				struct resource *res)
{
	desc->private_data = res;
}

static int desc_to_sdram(struct fip_image_desc *loadable, ulong load_address)
{
	struct resource *res;

	if (desc_get_res(loadable))
		return 0;

	res = request_sdram_region("fip", load_address,
				   loadable->image->toc_e.size,
				   MEMTYPE_LOADER_CODE, MEMATTRS_RW);
	if (!res)
		return -EBUSY;

	memcpy((void *)res->start,
	       loadable->image->buffer, loadable->image->toc_e.size);

	desc_set_res(loadable, res);

	return 0;
}

static void desc_release_sdram(struct fip_image_desc *loadable)
{
	struct resource *res = loadable ? desc_get_res(loadable) : NULL;
	if (res)
		release_sdram_region(res);
}

enum { IMAGE_BL33, IMAGE_HW_CONFIG, IMAGE_COUNT };

static int parse_dtb_registry(struct fip_image_desc *fw_config_desc,
			      struct fip_image_desc *loadables[],
			      int verbose)
{
	struct fip_toc_entry *toc_entry = &fw_config_desc->image->toc_e;
	struct device_node *fw_config, *fconf = NULL, *image, *tmp;
	const struct fip_binding *binding;
	void *buf;
	int ret = 0;

	if (!fip_bindings) {
		if (verbose)
			printf("Platform registered no FIP bindings. Skipping firmware config\n");
		return 0;
	}

	/* We have no alignment guarantees for the buffer, so we need to copy it */
	buf = xmemdup(fw_config_desc->image->buffer, toc_entry->size);

	fw_config = of_unflatten_dtb(buf, toc_entry->size);
	if (!IS_ERR(fw_config))
		fconf = of_find_compatible_node(fw_config, NULL,
						"fconf,dyn_cfg-dtb_registry");
	if (!fconf) {
		pr_warn("error parsing fw_config devicetree\n");
		goto out;
	}

	for_each_fip_binding(fip_bindings, binding) {
		for_each_child_of_node_safe(fconf, tmp, image) {
			u64 load_addr;
			u32 id;
			int ret;

			ret = of_property_read_u32(image, "id", &id);
			if (ret)
				continue;

			ret = of_property_read_u64(image, "load-address", &load_addr);
			if (ret || load_addr > ULONG_MAX)
				continue;

			if (id != binding->id)
				continue;

			for (int i = 0; i < IMAGE_COUNT; i++) {
				struct fip_image_desc *loadable = loadables[i];

				if (!loadable)
					continue;

				if (!uuid_equal(&binding->uuid,
						&loadable->image->toc_e.uuid))
					continue;

				ret = desc_to_sdram(loadable, load_addr);
				if (ret) {
					pr_warn("load address 0x%08llx for image ID %u"
						" conflicts with reservation\n",
						load_addr, binding->id);
					goto out;
				}

				if (verbose > 1)
					printf("loaded image ID %u to address 0x%08llx\n",
						binding->id, load_addr);
				break;
			}

			/* No need to iterate over this node again, so drop it */
			of_delete_node(image);
		}
	}

out:
	if (!IS_ERR(fw_config))
		of_delete_node(fw_config);
	free(buf);
	return ret;
}

static int fip_load(struct image_data *data,
		    struct fip_image_desc *fw_config_desc,
		    struct fip_image_desc *loadables[])
{
	resource_size_t start, end;
	int ret;

	if (UIMAGE_IS_ADDRESS_VALID(data->os_address)) {
		ret = desc_to_sdram(loadables[IMAGE_BL33], data->os_address);
		if (ret)
			return ret;
	}

	if (fw_config_desc) {
		ret = parse_dtb_registry(fw_config_desc, loadables, data->verbose);
		if (ret)
			return ret;
	}

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		return ret;

	for (int i = 0; i < IMAGE_COUNT; i++) {
		struct fip_image_desc *loadable = loadables[i];

		if (!loadable || desc_get_res(loadable))
			continue;

		start = ALIGN(start, SZ_4K);

		ret = desc_to_sdram(loadable, start);
		if (ret)
			return ret;

		/* Leave a stack's worth of space after each artifact;
		 * The STM32MP1 barebox entry point depends on it.
		 */
		start += resource_size(desc_get_res(loadable));
		start += CONFIG_STACK_SIZE;
		start = ALIGN(start, 16);
	}

	return 0;
}

static const uuid_t uuid_bl33 = UUID_NON_TRUSTED_FIRMWARE_BL33;
static const uuid_t uuid_hwconfig = UUID_HW_CONFIG;
static const uuid_t uuid_fwconfig = UUID_FW_CONFIG;

static int do_bootm_fip(struct image_data *data)
{
	struct fip_image_desc *fwconfig = NULL;
	struct fip_image_desc *loadables[IMAGE_COUNT] = {};
	struct fip_image_desc *desc;
	struct fip_state *fip;
	int ret;

	if (!data->os_file)
		return -EINVAL;

	fip = fip_new();
	ret = fip_parse(fip, data->os_file, NULL);
	if (ret)
		goto out;

	fip_for_each_desc(fip, desc) {
		struct fip_toc_entry *toc_entry = &desc->image->toc_e;

		if (uuid_equal(&toc_entry->uuid, &uuid_bl33))
			loadables[IMAGE_BL33] = desc;
		else if (uuid_equal(&toc_entry->uuid, &uuid_hwconfig))
			loadables[IMAGE_HW_CONFIG] = desc;
		else if (uuid_equal(&toc_entry->uuid, &uuid_fwconfig))
			fwconfig = desc;
	}

	if (!loadables[IMAGE_BL33]) {
		pr_err("FIP is missing BL33 image to chainload\n");
		ret = -ENOENT;
		goto out;
	}

	ret = fip_load(data, fwconfig, loadables);
	if (ret)
		goto out;

	if (data->verbose) {
		printf("Loaded non-trusted firmware to 0x%08lx",
		       (ulong)desc_get_res(loadables[IMAGE_BL33])->start);
		if (loadables[IMAGE_HW_CONFIG])
			printf(", hardware config to 0x%08lx",
			       (ulong)desc_get_res(loadables[IMAGE_HW_CONFIG])->start);
		printf("\n");
	}

	if (data->dryrun) {
		ret = 0;
		goto out;
	}

	shutdown_barebox();

	/* We don't actually expect NT FW to be a Linux image, but
	 * we want to use the Linux boot calling convention
	 */
	jump_to_linux((void *)desc_get_res(loadables[IMAGE_BL33])->start,
		      desc_get_res(loadables[IMAGE_HW_CONFIG])->start);

	ret = -EIO;
out:
	for (int i = 0; i < IMAGE_COUNT; i++)
		desc_release_sdram(loadables[i]);

	if (fip)
		fip_free(fip);
	return ret;
}

void plat_set_fip_bindings(const struct fip_binding *bindings)
{
	fip_bindings = bindings;
}

struct image_handler fip_image_handler = {
	.name = "FIP",
	.bootm = do_bootm_fip,
	.filetype = filetype_fip,
};

static int stm32mp_register_fip_bootm_handler(void)
{
	return register_image_handler(&fip_image_handler);
}
late_initcall(stm32mp_register_fip_bootm_handler);
