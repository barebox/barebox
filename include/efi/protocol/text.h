/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_TEXT_H_
#define __EFI_PROTOCOL_TEXT_H_

#include <efi/types.h>

struct efi_simple_text_input_ex_protocol;

typedef efi_status_t (EFIAPI *efi_input_reset_ex)(
	struct efi_simple_text_input_ex_protocol *this,
	bool extended_verification
);

struct efi_key_state {
	u32 shift_state;
	u8 toggle_state;
};

struct efi_input_key {
	u16 scan_code;
	efi_char16_t unicode_char;
};

struct efi_key_data {
	struct efi_input_key key;
	struct efi_key_state state;
};

typedef efi_status_t (EFIAPI *efi_input_read_key_ex)(
	struct efi_simple_text_input_ex_protocol *this,
	struct efi_key_data *keydata
);

typedef efi_status_t (EFIAPI *efi_set_state)(
	struct efi_simple_text_input_ex_protocol *this,
	u8 *key_toggle_state
);

typedef efi_status_t (EFIAPI *efi_key_notify_function)(
	struct efi_key_data *keydata
);

typedef efi_status_t (EFIAPI *efi_register_keystroke_notify)(
	struct efi_simple_text_input_ex_protocol *this,
	struct efi_key_data *keydata,
	efi_key_notify_function key_notification_function,
	void **notify_handle
);

typedef efi_status_t (EFIAPI *efi_unregister_keystroke_notify)(
	struct efi_simple_text_input_ex_protocol *this,
	void *notification_handle
);

struct efi_simple_text_input_ex_protocol {
	efi_input_reset_ex reset;
	efi_input_read_key_ex read_key_stroke_ex;
	struct efi_event *wait_for_key_ex;
	efi_set_state set_state;
	efi_register_keystroke_notify register_key_notify;
	efi_unregister_keystroke_notify unregister_key_notify;
};

struct simple_text_output_mode {
	s32 max_mode;
	s32 mode;
	s32 attribute;
	s32 cursor_column;
	s32 cursor_row;
	bool cursor_visible;
};

struct efi_simple_text_output_protocol {
	efi_status_t (EFIAPI *reset)(
			struct efi_simple_text_output_protocol *this,
			char extended_verification);
	efi_status_t (EFIAPI *output_string)(struct efi_simple_text_output_protocol *this, const efi_char16_t *str);
	efi_status_t (EFIAPI *test_string)(struct efi_simple_text_output_protocol *this,
			const efi_char16_t *str);

	efi_status_t(EFIAPI *query_mode)(struct efi_simple_text_output_protocol *this,
			unsigned long mode_number, unsigned long *columns, unsigned long *rows);
	efi_status_t(EFIAPI *set_mode)(struct efi_simple_text_output_protocol *this,
			unsigned long mode_number);
	efi_status_t(EFIAPI *set_attribute)(struct efi_simple_text_output_protocol *this,
			unsigned long attribute);
	efi_status_t(EFIAPI *clear_screen) (struct efi_simple_text_output_protocol *this);
	efi_status_t(EFIAPI *set_cursor_position) (struct efi_simple_text_output_protocol *this,
			unsigned long column, unsigned long row);
	efi_status_t(EFIAPI *enable_cursor)(struct efi_simple_text_output_protocol *this,
			bool enable);
	struct simple_text_output_mode *mode;
};

struct efi_simple_text_input_protocol {
	efi_status_t(EFIAPI *reset)(struct efi_simple_text_input_protocol *this,
			bool ExtendedVerification);
	efi_status_t(EFIAPI *read_key_stroke)(struct efi_simple_text_input_protocol *this,
			struct efi_input_key *key);
	struct efi_event *wait_for_key;
};

#define EFI_SHIFT_STATE_INVALID		0x00000000
#define EFI_RIGHT_SHIFT_PRESSED		0x00000001
#define EFI_LEFT_SHIFT_PRESSED		0x00000002
#define EFI_RIGHT_CONTROL_PRESSED	0x00000004
#define EFI_LEFT_CONTROL_PRESSED	0x00000008
#define EFI_RIGHT_ALT_PRESSED		0x00000010
#define EFI_LEFT_ALT_PRESSED		0x00000020
#define EFI_RIGHT_LOGO_PRESSED		0x00000040
#define EFI_LEFT_LOGO_PRESSED		0x00000080
#define EFI_MENU_KEY_PRESSED		0x00000100
#define EFI_SYS_REQ_PRESSED		0x00000200
#define EFI_SHIFT_STATE_VALID		0x80000000

#define EFI_CONTROL_PRESSED             (EFI_RIGHT_CONTROL_PRESSED | EFI_LEFT_CONTROL_PRESSED)
#define EFI_ALT_PRESSED                 (EFI_RIGHT_ALT_PRESSED | EFI_LEFT_ALT_PRESSED)
#define KEYPRESS(keys, scan, uni) ((((uint64_t)keys) << 32) | ((scan) << 16) | (uni))
#define KEYCHAR(k) ((k) & 0xffff)
#define CHAR_CTRL(c) ((c) - 'a' + 1)

#define EFI_TOGGLE_STATE_INVALID	0x00
#define EFI_SCROLL_LOCK_ACTIVE		0x01
#define EFI_NUM_LOCK_ACTIVE		0x02
#define EFI_CAPS_LOCK_ACTIVE		0x04
#define EFI_KEY_STATE_EXPOSED		0x40
#define EFI_TOGGLE_STATE_VALID		0x80

#define EFI_BLACK   0x00
#define EFI_BLUE    0x01
#define EFI_GREEN   0x02
#define EFI_CYAN            (EFI_BLUE | EFI_GREEN)
#define EFI_RED     0x04
#define EFI_MAGENTA         (EFI_BLUE | EFI_RED)
#define EFI_BROWN           (EFI_GREEN | EFI_RED)
#define EFI_LIGHTGRAY       (EFI_BLUE | EFI_GREEN | EFI_RED)
#define EFI_BRIGHT  0x08
#define EFI_DARKGRAY        (EFI_BRIGHT)
#define EFI_LIGHTBLUE       (EFI_BLUE | EFI_BRIGHT)
#define EFI_LIGHTGREEN      (EFI_GREEN | EFI_BRIGHT)
#define EFI_LIGHTCYAN       (EFI_CYAN | EFI_BRIGHT)
#define EFI_LIGHTRED        (EFI_RED | EFI_BRIGHT)
#define EFI_LIGHTMAGENTA    (EFI_MAGENTA | EFI_BRIGHT)
#define EFI_YELLOW          (EFI_BROWN | EFI_BRIGHT)
#define EFI_WHITE           (EFI_BLUE | EFI_GREEN | EFI_RED | EFI_BRIGHT)

#define EFI_TEXT_ATTR(f,b)	((f) | ((b) << 4))
/* extract foreground color from EFI attribute */
#define EFI_ATTR_FG(attr)	((attr) & 0x07)
/* treat high bit of FG as bright/bold (similar to edk2) */
#define EFI_ATTR_BOLD(attr)	(((attr) >> 3) & 0x01)
/* extract background color from EFI attribute */
#define EFI_ATTR_BG(attr)	(((attr) >> 4) & 0x7)

#define EFI_BACKGROUND_BLACK        0x00
#define EFI_BACKGROUND_BLUE         0x10
#define EFI_BACKGROUND_GREEN        0x20
#define EFI_BACKGROUND_CYAN         (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_GREEN)
#define EFI_BACKGROUND_RED          0x40
#define EFI_BACKGROUND_MAGENTA      (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_RED)
#define EFI_BACKGROUND_BROWN        (EFI_BACKGROUND_GREEN | EFI_BACKGROUND_RED)
#define EFI_BACKGROUND_LIGHTGRAY    (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_GREEN | EFI_BACKGROUND_RED)

#endif
