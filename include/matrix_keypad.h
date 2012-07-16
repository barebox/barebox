#ifndef _MATRIX_KEYPAD_H
#define _MATRIX_KEYPAD_H

#define MATRIX_MAX_ROWS		32
#define MATRIX_MAX_COLS		32

#define KEY(row, col, val)	((((row) & (MATRIX_MAX_ROWS - 1)) << 24) |\
				 (((col) & (MATRIX_MAX_COLS - 1)) << 16) |\
				 ((val) & 0xffff))

#define KEY_ROW(k)		(((k) >> 24) & 0xff)
#define KEY_COL(k)		(((k) >> 16) & 0xff)
#define KEY_VAL(k)		((k) & 0xffff)

#define MATRIX_SCAN_CODE(row, col, row_shift)	(((row) << (row_shift)) + (col))

/**
 * struct matrix_keymap_data - keymap for matrix keyboards
 * @keymap: pointer to array of uint32 values encoded with KEY() macro
 *	representing keymap
 * @keymap_size: number of entries (initialized) in this keymap
 *
 * This structure is supposed to be used by platform code to supply
 * keymaps to drivers that implement matrix-like keypads/keyboards.
 */
struct matrix_keymap_data {
	const uint32_t *keymap;
	unsigned int	keymap_size;
};

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
static inline void
matrix_keypad_build_keymap(const struct matrix_keymap_data *keymap_data,
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
}

#endif /* _MATRIX_KEYPAD_H */
