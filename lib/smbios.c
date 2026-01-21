// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: 2015, Bin Meng <bmeng.cn@gmail.com>
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/8327aa9af4be32b4236dfd25c8cefe568e9b3641/lib/smbios.c
// SPDX-Comment: Origin-URL: https://github.com/coreboot/coreboot/blob/main/src/arch/x86/smbios.c

#include <linux/bitfield.h>
#include <linux/stringify.h>
#include <linux/printk.h>
#include <linux/kstrtox.h>
#include <linux/sprintf.h>
#include <linux/minmax.h>
#include <linux/align.h>
#include <linux/array_size.h>
#include <linux/uuid.h>
#include <string.h>
#include <smbios.h>
#include <machine_id.h>
#include <generated/version.h>
#include <malloc.h>
#include <efi/mode.h>
#include <watchdog.h>
#include <restart.h>
#include <linux/reboot-mode.h>
#include <barebox.h>
#include <barebox-info.h>
#include <tables_csum.h>
#include <of.h>
#include <reset_source.h>
#include <asm/io.h>
#include <linux/sizes.h>

#define BAREBOX_VERSION_YEAR	FIELD_GET(0xFFFF0000u, LINUX_VERSION_CODE)
#define BAREBOX_VERSION_MONTH	FIELD_GET(0x0000FF00u, LINUX_VERSION_CODE)
#define BAREBOX_VERSION_PATCH	FIELD_GET(0x000000FFu, LINUX_VERSION_CODE)

static char *smbios_version;

static inline void *map_sysmem(void *addr, size_t len)
{
	return addr;
}

static inline void unmap_sysmem(void *addr) { }

/**
 * struct smbios_ctx - context for writing SMBIOS tables
 *
 * @eos:		end-of-string pointer for the table being processed.
 *			This is set up when we start processing a table
 * @next_ptr:		pointer to the start of the next string to be added.
 *			When the table is not empty, this points to the byte
 *			after the \0 of the previous string.
 * @last_str:		points to the last string that was written to the table,
 *			or NULL if none
 */
struct smbios_ctx {
	char *eos;
	char *next_ptr;
	char *last_str;
};

/**
 * Function prototype to write a specific type of SMBIOS structure
 *
 * @addr:	start address to write the structure
 * @handle:	the structure's handle, a unique 16-bit number
 * @ctx:	context for writing the tables
 * Return:	size of the structure
 */
typedef int (*smbios_write_type)(void **addr, int handle,
				 struct smbios_ctx *ctx);

/**
 * struct smbios_write_method - Information about a table-writing function
 *
 * @write: Function to call
 */
struct smbios_write_method {
	smbios_write_type write;
};

/**
 * smbios_add_string() - add a string to the string area
 *
 * This adds a string to the string area which is appended directly after
 * the formatted portion of an SMBIOS structure.
 *
 * @ctx:	SMBIOS context
 * @str:	string to add
 * Return:	string number in the string area. 0 if str is NULL.
 */
static int smbios_add_string(struct smbios_ctx *ctx, const char *str)
{
	int i = 1;
	char *p = ctx->eos;

	if (!str)
		return 0;

	for (;;) {
		if (!*p) {
			ctx->last_str = p;
			strcpy(p, str);
			p += strlen(str);
			*p++ = '\0';
			ctx->next_ptr = p;
			*p++ = '\0';

			return i;
		}

		if (!strcmp(p, str)) {
			ctx->last_str = p;
			return i;
		}

		p += strlen(p) + 1;
		i++;
	}
}

static void smbios_set_eos(struct smbios_ctx *ctx, char *eos)
{
	ctx->eos = eos;
	ctx->next_ptr = eos;
	ctx->last_str = NULL;
}

int smbios_update_version(const char *version)
{
	char *ptr = smbios_version;
	uint old_len, len;

	if (!ptr)
		return -ENOENT;

	/*
	 * This string is supposed to have at least enough bytes and is
	 * padded with spaces. Update it, taking care not to move the
	 * \0 terminator, so that other strings in the string table
	 * are not disturbed. See smbios_add_string()
	 */
	old_len = strnlen(ptr, SMBIOS_STR_MAX);
	len = strnlen(version, SMBIOS_STR_MAX);
	if (len > old_len)
		return -ENOSPC;

	pr_debug("Replacing SMBIOS type 0 version string '%s'\n", ptr);
	memcpy(ptr, version, len);

	return 0;
}

/**
 * smbios_string_table_len() - compute the string area size
 *
 * This computes the size of the string area including the string terminator.
 *
 * @ctx:	SMBIOS context
 * Return:	string area size
 */
static int smbios_string_table_len(const struct smbios_ctx *ctx)
{
	/* In case no string is defined we have to return two \0 */
	if (ctx->next_ptr == ctx->eos)
		return 2;

	/* Allow for the final \0 after all strings */
	return (ctx->next_ptr + 1) - ctx->eos;
}

static int smbios_write_type0(void **current, int handle,
			      struct smbios_ctx *ctx)
{
	char dmidate[sizeof("mm/dd/yyyy")];
	struct smbios_type0 *t;
	int len = sizeof(*t);

	t = map_sysmem(*current, len);
	memset(t, 0, len);
	fill_smbios_header(t, SMBIOS_BIOS_INFORMATION, len, handle);
	smbios_set_eos(ctx, t->eos);
	t->vendor = smbios_add_string(ctx, "barebox");

	t->bios_ver = smbios_add_string(ctx, version_string);
	if (t->bios_ver)
		smbios_version = ctx->last_str;

	pr_debug("smbios_version = %p: '%s'\n", smbios_version,
		  smbios_version);

	scnprintf(dmidate, sizeof(dmidate), "%02d/01/%04d",
		  BAREBOX_VERSION_MONTH, BAREBOX_VERSION_YEAR);

	t->bios_release_date = smbios_add_string(ctx, dmidate);

	t->bios_characteristics = BIOS_CHARACTERISTICS_PCI_SUPPORTED |
				  BIOS_CHARACTERISTICS_SELECTABLE_BOOT |
				  BIOS_CHARACTERISTICS_UPGRADEABLE;

	if (efi_is_loader())
		t->bios_characteristics_ext2 |= BIOS_CHARACTERISTICS_EXT2_UEFI;

	t->bios_characteristics_ext2 |= BIOS_CHARACTERISTICS_EXT2_TARGET;

	/* bios_major_release has only one byte, so drop century */
	t->bios_major_release = BAREBOX_VERSION_YEAR % 100;
	t->bios_minor_release = BAREBOX_VERSION_YEAR * 10 + min(BAREBOX_VERSION_PATCH, 9u);
	t->ec_major_release = 0xff;
	t->ec_minor_release = 0xff;

	len = t->hdr.length + smbios_string_table_len(ctx);
	*current += len;
	unmap_sysmem(t);

	return len;
}

static int smbios_write_type1(void **current, int handle,
			      struct smbios_ctx *ctx)
{
	struct smbios_type1 *t;
	int len = sizeof(*t);
	const uuid_t *product_uuid = NULL;
	uuid_t product_uuid_buf;
	char *vendor;

	t = map_sysmem(*current, len);
	memset(t, 0, len);
	fill_smbios_header(t, SMBIOS_SYSTEM_INFORMATION, len, handle);
	smbios_set_eos(ctx, t->eos);

	vendor = of_get_machine_vendor();
	t->manufacturer = smbios_add_string(ctx, vendor);
	free(vendor);

	t->product_name = smbios_add_string(ctx, barebox_get_model());
	t->version = smbios_add_string(ctx, NULL);
	t->serial_number = smbios_add_string(ctx, barebox_get_serial_number());

	product_uuid = barebox_get_product_uuid();
	if (!product_uuid && IS_ENABLED(CONFIG_MACHINE_ID_SPECIFIC)) {
		int err;

		err = machine_id_get_app_specific(&product_uuid_buf,
						  ARRAY_AND_SIZE("barebox-smbios"),
						  NULL);
		if (!err)
			product_uuid = &product_uuid_buf;
	}

	if (product_uuid)
		export_uuid(t->uuid, product_uuid);

	if (reset_source_get() == RESET_WKE)
		t->wakeup_type = SMBIOS_WAKEUP_TYPE_OTHER;
	else
		t->wakeup_type = SMBIOS_WAKEUP_TYPE_UNKNOWN;

	t->sku_number = smbios_add_string(ctx, NULL);
	t->family = smbios_add_string(ctx, NULL);

	len = t->hdr.length + smbios_string_table_len(ctx);
	*current += len;
	unmap_sysmem(t);

	return len;
}

static int smbios_write_type23(void **current, int handle,
			       struct smbios_ctx *ctx)
{
	struct smbios_type23 *t;
	int len = sizeof(*t);

	t = map_sysmem(*current, len);
	memset(t, 0, len);

	t->capabilities =  0;

	if (restart_handler_get_by_name(NULL, 0))
		t->capabilities |= SMBIOS_SYSRESET_CAP_SYSRESET_ENABLED;

	t->capabilities |= SMBIOS_SYSRESET_CAP_BOOTOPT_RESET_OS;
	t->capabilities |= SMBIOS_SYSRESET_CAP_BOOTLIMIT_RESET_OS;

	if (watchdog_get_default())
		t->capabilities |= SMBIOS_SYSRESET_CAP_WATCHDOG;

	t->reset_count = 0xffff;
	t->reset_limit = 0xffff;

	// TODO: we could pass actual info here, but how should we round
	// a 30s watchdog timeout?
	t->timer_interval = 0xffff;
	t->timeout = 0xffff;

	fill_smbios_header(t, SMBIOS_SYSTEM_RESET, len, handle);
	smbios_set_eos(ctx, t->eos);

	*current += len;
	unmap_sysmem(t);

	return len;
}

static u8 compute_boot_status(void)
{
	switch (reset_source_get()) {
		case RESET_WDG:
			return SMBIOS_BOOT_STATUS_WATCHDOG_REBOOT;
		case RESET_THERM:
		case RESET_BROWNOUT:
			return SMBIOS_BOOT_STATUS_FW_DETECTED_HWFAULT;
		default:
			break;
	}

	switch (get_autoboot_state()) {
		case AUTOBOOT_ABORT:
		case AUTOBOOT_HALT:
			return SMBIOS_BOOT_STATUS_USER_REQ_BOOT;
		default:
			break;
	}

	if (reboot_mode_get())
		return SMBIOS_BOOT_STATUS_PREV_REQ_IMAGE;

	return SMBIOS_BOOT_STATUS_NO_ERRORS;
}

static int smbios_write_type32(void **current, int handle,
			       struct smbios_ctx *ctx)
{
	struct smbios_type32 *t;
	int len = sizeof(*t);

	t = map_sysmem(*current, len);
	memset(t, 0, len);

	t->boot_status = compute_boot_status();

	fill_smbios_header(t, SMBIOS_SYSTEM_BOOT_INFORMATION, len, handle);
	smbios_set_eos(ctx, t->eos);

	*current += len;
	unmap_sysmem(t);

	return len;
}

static int smbios_write_type127(void **current, int handle,
				struct smbios_ctx *ctx)
{
	struct smbios_type127 *t;
	int len = sizeof(*t);

	t = map_sysmem(*current, len);
	memset(t, 0, len);
	fill_smbios_header(t, SMBIOS_END_OF_TABLE, len, handle);

	*current += len;
	unmap_sysmem(t);

	return len;
}

static struct smbios_write_method smbios_write_funcs[] = {
	{ smbios_write_type0, },
	{ smbios_write_type1, },
	{ smbios_write_type23, },
	{ smbios_write_type32, },
	{ smbios_write_type127 },
};

void *write_smbios_table(void *addr)
{
	void *table_addr, *start_addr;
	struct smbios3_entry *se;
	struct smbios_ctx ctx;
	void *tables;
	int len = 0;
	int handle = 0;
	int i;

	start_addr = addr;

	/* move past the (so-far-unwritten) header to start writing structs */
	addr = PTR_ALIGN(addr + sizeof(struct smbios3_entry), 16);
	tables = addr;

	/* populate minimum required tables */
	for (i = 0; i < ARRAY_SIZE(smbios_write_funcs); i++) {
		const struct smbios_write_method *method;

		method = &smbios_write_funcs[i];
		len += method->write(&addr, handle++, &ctx);
	}

	table_addr = map_sysmem(tables, 0);

	/* now go back and write the SMBIOS3 header */
	se = map_sysmem(start_addr, sizeof(struct smbios3_entry));
	memset(se, '\0', sizeof(struct smbios3_entry));
	memcpy(se->anchor, "_SM3_", 5);
	se->length = sizeof(struct smbios3_entry);
	se->major_ver = SMBIOS_MAJOR_VER;
	se->minor_ver = SMBIOS_MINOR_VER;
	se->doc_rev = 0;
	se->entry_point_rev = 1;
	se->table_maximum_size = len;
	se->struct_table_address = virt_to_phys(table_addr);
	se->checksum = table_compute_checksum(se, sizeof(struct smbios3_entry));
	unmap_sysmem(se);

	return addr;
}
