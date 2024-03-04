/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef EFI_STDIO_H_
#define EFI_STDIO_H_

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

#define EFI_SHIFT_STATE_VALID           0x80000000
#define EFI_RIGHT_CONTROL_PRESSED       0x00000004
#define EFI_LEFT_CONTROL_PRESSED        0x00000008
#define EFI_RIGHT_ALT_PRESSED           0x00000010
#define EFI_LEFT_ALT_PRESSED            0x00000020

#define EFI_CONTROL_PRESSED             (EFI_RIGHT_CONTROL_PRESSED | EFI_LEFT_CONTROL_PRESSED)
#define EFI_ALT_PRESSED                 (EFI_RIGHT_ALT_PRESSED | EFI_LEFT_ALT_PRESSED)
#define KEYPRESS(keys, scan, uni) ((((uint64_t)keys) << 32) | ((scan) << 16) | (uni))
#define KEYCHAR(k) ((k) & 0xffff)
#define CHAR_CTRL(c) ((c) - 'a' + 1)

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

#define EFI_TEXT_ATTR(f,b)  ((f) | ((b) << 4))

#define EFI_BACKGROUND_BLACK        0x00
#define EFI_BACKGROUND_BLUE         0x10
#define EFI_BACKGROUND_GREEN        0x20
#define EFI_BACKGROUND_CYAN         (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_GREEN)
#define EFI_BACKGROUND_RED          0x40
#define EFI_BACKGROUND_MAGENTA      (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_RED)
#define EFI_BACKGROUND_BROWN        (EFI_BACKGROUND_GREEN | EFI_BACKGROUND_RED)
#define EFI_BACKGROUND_LIGHTGRAY    (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_GREEN | EFI_BACKGROUND_RED)

#endif
