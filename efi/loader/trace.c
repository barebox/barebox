// SPDX-License-Identifier: GPL-2.0+

#include <efi/loader/trace.h>
#include <linux/minmax.h>

/* 1 if inside barebox code, 0 if inside EFI payload code */
static int nesting_level = 1;

/**
 * indent_string() - returns a string for indenting with two spaces per level
 * @level: indent level
 *
 * A maximum of ten indent levels is supported. Higher indent levels will be
 * truncated.
 *
 * Return: A string for indenting with two spaces per level is
 *         returned.
 */
static const char *indent_string(int level)
{
	const char *indent = "                    ";
	const int max = strlen(indent);

	if (level < 0)
		return "!!EFI_EXIT called without matching EFI_ENTRY!! ";

	level = min(max, level * 2);
	return &indent[max - level];
}

const char *__efi_nesting(void)
{
	return indent_string(nesting_level);
}

const char *__efi_nesting_inc(void)
{
	return indent_string(nesting_level++);
}

const char *__efi_nesting_dec(void)
{
	return indent_string(--nesting_level);
}
