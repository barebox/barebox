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
int matrix_keypad_build_keymap(const struct matrix_keymap_data *keymap_data,
			   unsigned int row_shift,
			   unsigned short *keymap)
{
	int i;

	for (i = 0; i < keymap_data->keymap_size; i++) {
		unsigned int key = keymap_data->keymap[i];
		unsigned int row = KEY_ROW(key);
		unsigned int col = KEY_COL(key);
		unsigned short code = KEY_VAL(key);

		keymap[MATRIX_SCAN_CODE(row, col, row_shift)] = code;
	}

	return 0;
}
