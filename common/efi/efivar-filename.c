/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/ctype.h>
#include <string.h>
#include <efi/efi-util.h>

static int char_to_nibble(char c)
{
	int ret = tolower(c);

	return ret <= '9' ? ret - '0' : ret - 'a' + 10;
}

static int read_byte_str(const char *str, u8 *out)
{
	if (!isxdigit(*str) || !isxdigit(*(str + 1)))
		return -EINVAL;

	*out = (char_to_nibble(*str) << 4) | char_to_nibble(*(str + 1));

	return 0;
}

static int efi_guid_parse(const char *str, efi_guid_t *guid)
{
	int i, ret;
	u8 idx[] = { 3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };

	for (i = 0; i < 16; i++) {
		ret = read_byte_str(str, &guid->b[idx[i]]);
		if (ret)
			return ret;
		str += 2;

		switch (i) {
		case 3:
		case 5:
		case 7:
		case 9:
			if (*str != '-')
				return -EINVAL;
			str++;
			break;
		}
	}

	return 0;
}


int __efivarfs_parse_filename(const char *filename, efi_guid_t *vendor,
			      s16 *name, size_t *namelen)
{
	int len, ret;
	const char *guidstr;
	int i;

	len = strlen(filename);

	if (len < sizeof("-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"))
		return -EINVAL;

	guidstr = filename + len - sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx");
	if (*guidstr != '-' || guidstr == filename)
		return -EINVAL;

	guidstr++;

	ret = efi_guid_parse(guidstr, vendor);
	if (ret)
		return ret;

	if (guidstr - filename > *namelen)
		ret = -EFBIG;

	*namelen = guidstr - filename;

	if (ret)
		return ret;

	for (i = 0; i < *namelen - 1; i++)
		name[i] = filename[i];

	name[i] = L'\0';

	return 0;
}

int efivarfs_parse_filename(const char *filename, efi_guid_t *vendor,
			    s16 **name)
{
	int ret;
	s16 *varname;
	size_t namelen = 0;
	int i;

	if (*filename == '/')
		filename++;

	ret = __efivarfs_parse_filename(filename, vendor, NULL, &namelen);
	if (ret != -EFBIG)
		return ret;

	varname = xzalloc(namelen * sizeof(s16));

	for (i = 0; i < namelen - 1; i++)
		varname[i] = filename[i];

	varname[i] = L'\0';

	*name = varname;

	return 0;
}


