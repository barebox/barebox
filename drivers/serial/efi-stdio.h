#ifndef EFI_STDIO_H_
#define EFI_STDIO_H_

#include <efi.h>

struct efi_simple_text_input_ex_protocol;

typedef efi_status_t (EFIAPI *efi_input_reset_ex)(
	struct efi_simple_text_input_ex_protocol *this,
	efi_bool_t extended_verification
);

struct efi_key_state {
	u32 shift_state;
	u8 toggle_state;
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
	struct efi_key_data keydata,
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
	void *wait_for_key_ex;
	efi_set_state set_state;
	efi_register_keystroke_notify register_key_notify;
	efi_unregister_keystroke_notify unregister_key_notify;
};

#endif
