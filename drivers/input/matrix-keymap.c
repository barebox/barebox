/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <input/matrix_keypad.h>

static int matrix_keypad_parse_of_keymap(struct device_d *dev,
					 unsigned int row_shift,
					 unsigned short *keymap)
{
	unsigned int proplen, i, size;
	const __be32 *prop;
	struct device_node *np = dev->device_node;
	const char *propname = "linux,keymap";

	prop = of_get_property(np, propname, &proplen);
	if (!prop) {
		dev_err(dev, "OF: %s property not defined in %s\n",
			propname, np->full_name);
		return -ENOENT;
	}

	if (proplen % sizeof(u32)) {
		dev_err(dev, "OF: Malformed keycode property %s in %s\n",
			propname, np->full_name);
		return -EINVAL;
	}

	size = proplen / sizeof(u32);

	for (i = 0; i < size; i++) {
		unsigned int key = be32_to_cpup(prop + i);

		unsigned int row = KEY_ROW(key);
		unsigned int col = KEY_COL(key);
		unsigned short code = KEY_VAL(key);

		if (row >= MATRIX_MAX_ROWS || col >= MATRIX_MAX_COLS) {
			dev_err(dev, "rows/cols out of range\n");
			continue;
		}

		keymap[MATRIX_SCAN_CODE(row, col, row_shift)] = code;
	}

	return 0;
}
/**
 * matrix_keypad_build_keymap - convert platform keymap into matrix keymap
 * @keymap_data: keymap supplied by the platform code
 * @row_shift: number of bits to shift row value by to advance to the next
 * line in the keymap
 * @keymap: expanded version of keymap that is suitable for use by
 * matrix keyboad driver
 * This function converts platform keymap (encoded with KEY() macro) into
 * an array of keycodes that is suitable for using in a standard matrix
 * keyboard driver that uses row and col as indices.
 */
int matrix_keypad_build_keymap(struct device_d *dev, const struct matrix_keymap_data *keymap_data,
			   unsigned int row_shift,
			   unsigned short *keymap)
{
	int i;

	if (IS_ENABLED(CONFIG_OFDEVICE) && dev->device_node)
		return matrix_keypad_parse_of_keymap(dev, row_shift, keymap);

	if (!keymap_data)
		return -EINVAL;

	for (i = 0; i < keymap_data->keymap_size; i++) {
		unsigned int key = keymap_data->keymap[i];
		unsigned int row = KEY_ROW(key);
		unsigned int col = KEY_COL(key);
		unsigned short code = KEY_VAL(key);

		keymap[MATRIX_SCAN_CODE(row, col, row_shift)] = code;
	}

	return 0;
}
