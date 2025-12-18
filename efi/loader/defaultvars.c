// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/e7a85ec651ed5794eb9a837e1073f6b3146af501/lib/efi_loader/efi_setup.c

#define pr_fmt(fmt) "efi-loader: defaultvars: " fmt

#include <efi/types.h>
#include <efi/runtime.h>
#include <efi/error.h>
#include <efi/guid.h>
#include <efi/loader.h>
#include <efi/variable.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <init.h>
#include <param.h>
#include <magicvar.h>
#include <xfuncs.h>


/**
 * efi_init_platform_lang() - define supported languages
 *
 * Set the PlatformLangCodes and PlatformLang variables.
 *
 * Return:	status code
 */
static efi_status_t efi_init_platform_lang(void *_efi_lang_codes)
{
	efi_status_t ret;
	efi_uintn_t data_size = 0;
	char *efi_lang_codes = _efi_lang_codes;
	char *lang = efi_lang_codes;
	char *pos;

	/*
	 * Variable PlatformLangCodes defines the language codes that the
	 * machine can support.
	 */
	ret = efi_set_variable_int(u"PlatformLangCodes",
				   &efi_global_variable_guid,
				   EFI_VARIABLE_BOOTSERVICE_ACCESS |
				   EFI_VARIABLE_RUNTIME_ACCESS |
				   EFI_VARIABLE_READ_ONLY,
				   strlen(efi_lang_codes) + 1,
				   efi_lang_codes, false);
	if (ret != EFI_SUCCESS)
		goto out;

	/*
	 * Variable PlatformLang defines the language that the machine has been
	 * configured for.
	 */
	ret = efi_get_variable_int(u"PlatformLang",
				   &efi_global_variable_guid,
				   NULL, &data_size, &pos, NULL);
	if (ret == EFI_BUFFER_TOO_SMALL) {
		/* The variable is already set. Do not change it. */
		ret = EFI_SUCCESS;
		goto out;
	}

	/*
	 * The list of supported languages is semicolon separated. Use the first
	 * language to initialize PlatformLang.
	 */
	pos = strchr(lang, ';');
	if (pos)
		*pos = 0;

	ret = efi_set_variable_int(u"PlatformLang",
				   &efi_global_variable_guid,
				   EFI_VARIABLE_NON_VOLATILE |
				   EFI_VARIABLE_BOOTSERVICE_ACCESS |
				   EFI_VARIABLE_RUNTIME_ACCESS,
				   1 + strlen(lang), lang, false);
out:
	if (ret != EFI_SUCCESS)
		pr_warn("EFI: cannot initialize platform language settings\n");
	return ret;
}

/**
 * efi_init_os_indications() - indicate supported features for OS requests
 *
 * Set the OsIndicationsSupported variable.
 *
 * Return:	status code
 */
static efi_status_t efi_init_os_indications(void *data)
{
	u64 os_indications_supported = 0;

	return efi_set_variable_int(u"OsIndicationsSupported",
				    &efi_global_variable_guid,
				    EFI_VARIABLE_BOOTSERVICE_ACCESS |
				    EFI_VARIABLE_RUNTIME_ACCESS |
				    EFI_VARIABLE_READ_ONLY,
				    sizeof(os_indications_supported),
				    &os_indications_supported, false);
}

/*
 * This value is used to initialize the PlatformLangCodes variable. Its
 * value is a semicolon (;) separated list of language codes in native
 * RFC 4646 format, e.g. "en-US;de-DE". The first language code is used
 * to initialize the PlatformLang variable.
 */
static char *efi_lang_codes;

static int efi_defaultvars_init(void)
{
	efi_lang_codes = xstrdup("en-US");
	dev_add_param_string(&efidev, "lang.codes", NULL, NULL,
			     &efi_lang_codes, NULL);

	efi_register_deferred_init(efi_init_platform_lang, efi_lang_codes);
	efi_register_deferred_init(efi_init_os_indications, NULL);
	return 0;
}
late_initcall(efi_defaultvars_init);

BAREBOX_MAGICVAR(efi.lang.codes, "semicolon-separated list of RFC 4646 formatted languages for PlatformLangCodes");
