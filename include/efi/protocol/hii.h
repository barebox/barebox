/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Human Interface Infrastructure (HII)
 */

#ifndef __EFI_PROTOCOL_HII_H_
#define __EFI_PROTOCOL_HII_H_

#include <efi/types.h>

typedef void *efi_hii_handle_t;
typedef u16 *efi_string_t;
typedef u16 efi_string_id_t;
typedef u32 efi_hii_font_style_t;
typedef u16 efi_question_id_t;
typedef u16 efi_image_id_t;
typedef u16 efi_form_id_t;

struct efi_hii_package_list_header {
	efi_guid_t package_list_guid;
	u32 package_length;
} __packed;

/**
 * struct efi_hii_package_header - EFI HII package header
 *
 * @fields:	'fields' replaces the bit-fields defined in the EFI
 *		specification to to avoid possible compiler incompatibilities::
 *
 *		    u32 length:24;
 *		    u32 type:8;
 */
struct efi_hii_package_header {
	u32 fields;
} __packed;

#define __EFI_HII_PACKAGE_LEN_SHIFT	0
#define __EFI_HII_PACKAGE_TYPE_SHIFT	24
#define __EFI_HII_PACKAGE_LEN_MASK	0xffffff
#define __EFI_HII_PACKAGE_TYPE_MASK	0xff

#define EFI_HII_PACKAGE_TYPE_ALL          0x00
#define EFI_HII_PACKAGE_TYPE_GUID         0x01
#define EFI_HII_PACKAGE_FORMS             0x02
#define EFI_HII_PACKAGE_STRINGS           0x04
#define EFI_HII_PACKAGE_FONTS             0x05
#define EFI_HII_PACKAGE_IMAGES            0x06
#define EFI_HII_PACKAGE_SIMPLE_FONTS      0x07
#define EFI_HII_PACKAGE_DEVICE_PATH       0x08
#define EFI_HII_PACKAGE_KEYBOARD_LAYOUT   0x09
#define EFI_HII_PACKAGE_ANIMATIONS        0x0A
#define EFI_HII_PACKAGE_END               0xDF
#define EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN 0xE0
#define EFI_HII_PACKAGE_TYPE_SYSTEM_END   0xFF

/*
 * HII GUID package
 */
struct efi_hii_guid_package {
	struct efi_hii_package_header header;
	efi_guid_t guid;
	char data[];
} __packed;

/*
 * HII string package
 */
struct efi_hii_strings_package {
	struct efi_hii_package_header header;
	u32 header_size;
	u32 string_info_offset;
	u16 language_window[16];
	efi_string_id_t language_name;
	u8  language[];
} __packed;

struct efi_hii_string_block {
	u8 block_type;
	/* u8 block_body[]; */
} __packed;

#define EFI_HII_SIBT_END               0x00
#define EFI_HII_SIBT_STRING_SCSU       0x10
#define EFI_HII_SIBT_STRING_SCSU_FONT  0x11
#define EFI_HII_SIBT_STRINGS_SCSU      0x12
#define EFI_HII_SIBT_STRINGS_SCSU_FONT 0x13
#define EFI_HII_SIBT_STRING_UCS2       0x14
#define EFI_HII_SIBT_STRING_UCS2_FONT  0x15
#define EFI_HII_SIBT_STRINGS_UCS2      0x16
#define EFI_HII_SIBT_STRINGS_UCS2_FONT 0x17
#define EFI_HII_SIBT_DUPLICATE         0x20
#define EFI_HII_SIBT_SKIP2             0x21
#define EFI_HII_SIBT_SKIP1             0x22
#define EFI_HII_SIBT_EXT1              0x30
#define EFI_HII_SIBT_EXT2              0x31
#define EFI_HII_SIBT_EXT4              0x32
#define EFI_HII_SIBT_FONT              0x40

struct efi_hii_sibt_string_ucs2_block {
	struct efi_hii_string_block header;
	u16 string_text[];
} __packed;

/*
 * HII forms package
 * TODO: full scope of definitions
 */
struct efi_hii_time {
	u8 hour;
	u8 minute;
	u8 second;
};

struct efi_hii_date {
	u16 year;
	u8 month;
	u8 day;
};

struct efi_hii_ref {
	efi_question_id_t question_id;
	efi_form_id_t form_id;
	efi_guid_t form_set_guid;
	efi_string_id_t device_path;
};

union efi_ifr_type_value {
	u8 u8;				// EFI_IFR_TYPE_NUM_SIZE_8
	u16 u16;			// EFI_IFR_TYPE_NUM_SIZE_16
	u32 u32;			// EFI_IFR_TYPE_NUM_SIZE_32
	u64 u64;			// EFI_IFR_TYPE_NUM_SIZE_64
	bool b;				// EFI_IFR_TYPE_BOOLEAN
	struct efi_hii_time time;	// EFI_IFR_TYPE_TIME
	struct efi_hii_date date;	// EFI_IFR_TYPE_DATE
	efi_string_id_t string;		// EFI_IFR_TYPE_STRING, EFI_IFR_TYPE_ACTION
	struct efi_hii_ref ref;		// EFI_IFR_TYPE_REF
	// u8 buffer[];			// EFI_IFR_TYPE_BUFFER
};

#define EFI_IFR_TYPE_NUM_SIZE_8		0x00
#define EFI_IFR_TYPE_NUM_SIZE_16	0x01
#define EFI_IFR_TYPE_NUM_SIZE_32	0x02
#define EFI_IFR_TYPE_NUM_SIZE_64	0x03
#define EFI_IFR_TYPE_BOOLEAN		0x04
#define EFI_IFR_TYPE_TIME		0x05
#define EFI_IFR_TYPE_DATE		0x06
#define EFI_IFR_TYPE_STRING		0x07
#define EFI_IFR_TYPE_OTHER		0x08
#define EFI_IFR_TYPE_UNDEFINED		0x09
#define EFI_IFR_TYPE_ACTION		0x0A
#define EFI_IFR_TYPE_BUFFER		0x0B
#define EFI_IFR_TYPE_REF		0x0C
#define EFI_IFR_OPTION_DEFAULT		0x10
#define EFI_IFR_OPTION_DEFAULT_MFG	0x20

#define EFI_IFR_ONE_OF_OPTION_OP	0x09

struct efi_ifr_op_header {
	u8 opCode;
	u8 length:7;
	u8 scope:1;
};

struct efi_ifr_one_of_option {
	struct efi_ifr_op_header header;
	efi_string_id_t option;
	u8 flags;
	u8 type;
	union efi_ifr_type_value value;
};

typedef efi_uintn_t efi_browser_action_t;

#define EFI_BROWSER_ACTION_REQUEST_NONE			0
#define EFI_BROWSER_ACTION_REQUEST_RESET		1
#define EFI_BROWSER_ACTION_REQUEST_SUBMIT		2
#define EFI_BROWSER_ACTION_REQUEST_EXIT			3
#define EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT	4
#define EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT	5
#define EFI_BROWSER_ACTION_REQUEST_FORM_APPLY		6
#define EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD		7
#define EFI_BROWSER_ACTION_REQUEST_RECONNECT		8

typedef efi_uintn_t efi_browser_action_request_t;

#define EFI_BROWSER_ACTION_CHANGING			0
#define EFI_BROWSER_ACTION_CHANGED			1
#define EFI_BROWSER_ACTION_RETRIEVE			2
#define EFI_BROWSER_ACTION_FORM_OPEN			3
#define EFI_BROWSER_ACTION_FORM_CLOSE			4
#define EFI_BROWSER_ACTION_SUBMITTED			5
#define EFI_BROWSER_ACTION_DEFAULT_STANDARD		0x1000
#define EFI_BROWSER_ACTION_DEFAULT_MANUFACTURING	0x1001
#define EFI_BROWSER_ACTION_DEFAULT_SAFE			0x1002
#define EFI_BROWSER_ACTION_DEFAULT_PLATFORM		0x2000
#define EFI_BROWSER_ACTION_DEFAULT_HARDWARE		0x3000
#define EFI_BROWSER_ACTION_DEFAULT_FIRMWARE		0x4000

/*
 * HII keyboard package
 */
typedef enum {
	EFI_KEY_LCTRL, EFI_KEY_A0, EFI_KEY_LALT, EFI_KEY_SPACE_BAR,
	EFI_KEY_A2, EFI_KEY_A3, EFI_KEY_A4, EFI_KEY_RCTRL, EFI_KEY_LEFT_ARROW,
	EFI_KEY_DOWN_ARROW, EFI_KEY_RIGHT_ARROW, EFI_KEY_ZERO,
	EFI_KEY_PERIOD, EFI_KEY_ENTER, EFI_KEY_LSHIFT, EFI_KEY_B0,
	EFI_KEY_B1, EFI_KEY_B2, EFI_KEY_B3, EFI_KEY_B4, EFI_KEY_B5, EFI_KEY_B6,
	EFI_KEY_B7, EFI_KEY_B8, EFI_KEY_B9, EFI_KEY_B10, EFI_KEY_RSHIFT,
	EFI_KEY_UP_ARROW, EFI_KEY_ONE, EFI_KEY_TWO, EFI_KEY_THREE,
	EFI_KEY_CAPS_LOCK, EFI_KEY_C1, EFI_KEY_C2, EFI_KEY_C3, EFI_KEY_C4,
	EFI_KEY_C5, EFI_KEY_C6, EFI_KEY_C7, EFI_KEY_C8, EFI_KEY_C9,
	EFI_KEY_C10, EFI_KEY_C11, EFI_KEY_C12, EFI_KEY_FOUR, EFI_KEY_FIVE,
	EFI_KEY_SIX, EFI_KEY_PLUS, EFI_KEY_TAB, EFI_KEY_D1, EFI_KEY_D2,
	EFI_KEY_D3, EFI_KEY_D4, EFI_KEY_D5, EFI_KEY_D6, EFI_KEY_D7, EFI_KEY_D8,
	EFI_KEY_D9, EFI_KEY_D10, EFI_KEY_D11, EFI_KEY_D12, EFI_KEY_D13,
	EFI_KEY_DEL, EFI_KEY_END, EFI_KEY_PG_DN, EFI_KEY_SEVEN, EFI_KEY_EIGHT,
	EFI_KEY_NINE, EFI_KEY_E0, EFI_KEY_E1, EFI_KEY_E2, EFI_KEY_E3,
	EFI_KEY_E4, EFI_KEY_E5, EFI_KEY_E6, EFI_KEY_E7, EFI_KEY_E8, EFI_KEY_E9,
	EFI_KEY_E10, EFI_KEY_E11, EFI_KEY_E12, EFI_KEY_BACK_SPACE,
	EFI_KEY_INS, EFI_KEY_HOME, EFI_KEY_PG_UP, EFI_KEY_NLCK, EFI_KEY_SLASH,
	EFI_KEY_ASTERISK, EFI_KEY_MINUS, EFI_KEY_ESC, EFI_KEY_F1, EFI_KEY_F2,
	EFI_KEY_F3, EFI_KEY_F4, EFI_KEY_F5, EFI_KEY_F6, EFI_KEY_F7, EFI_KEY_F8,
	EFI_KEY_F9, EFI_KEY_F10, EFI_KEY_F11, EFI_KEY_F12, EFI_KEY_PRINT,
	EFI_KEY_SLCK, EFI_KEY_PAUSE,
} efi_key;

struct efi_key_descriptor {
	u32 key;
	u16 unicode;
	u16 shifted_unicode;
	u16 alt_gr_unicode;
	u16 shifted_alt_gr_unicode;
	u16 modifier;
	u16 affected_attribute;
} __packed;

struct efi_hii_keyboard_layout {
	u16 layout_length;
	/*
	 * The EFI spec defines this as efi_guid_t.
	 * clang and gcc both report alignment problems here.
	 * clang with -Wunaligned-access
	 * warning: field guid within 'struct efi_hii_keyboard_layout' is less
	 * aligned than 'efi_guid_t' and is usually due to
	 * 'struct efi_hii_keyboard_layout' being packed, which can lead to
	 * unaligned accesses
	 *
	 * GCC with -Wpacked-not-aligned -Waddress-of-packed-member
	 * 'efi_guid_t' offset 2 in 'struct efi_hii_keyboard_layout'
	 * isn't aligned to 4
	 *
	 * Removing the alignment from efi_guid_t is not an option, since
	 * it is also used in non-packed structs and that would break
	 * calculations with offsetof
	 *
	 * This is the only place we get a report for. That happens because
	 * all other declarations of efi_guid_t within a packed struct happens
	 * to be 4-byte aligned.  i.e a u32, a u64 a 2 * u16 or any combination
	 * that ends up landing efi_guid_t on a 4byte boundary precedes.
	 *
	 * Replace this with a 1-byte aligned counterpart of b[16].  This is a
	 * packed struct so the memory  placement of efi_guid_t should not change
	 *
	 */
	u8 guid[16];
	u32 layout_descriptor_string_offset;
	u8 descriptor_count;
	/* struct efi_key_descriptor descriptors[]; follows here */
} __packed;

struct efi_hii_keyboard_package {
	struct efi_hii_package_header header;
	u16 layout_count;
	struct efi_hii_keyboard_layout layout[];
} __packed;

struct efi_font_info {
	efi_hii_font_style_t font_style;
	u16 font_size;
	u16 font_name[1];
};

struct efi_hii_string_protocol {
	efi_status_t(EFIAPI *new_string)(
		const struct efi_hii_string_protocol *this,
		efi_hii_handle_t package_list,
		efi_string_id_t *string_id,
		const u8 *language,
		const u16 *language_name,
		const efi_string_t string,
		const struct efi_font_info *string_font_info);
	efi_status_t(EFIAPI *get_string)(
		const struct efi_hii_string_protocol *this,
		const u8 *language,
		efi_hii_handle_t package_list,
		efi_string_id_t string_id,
		efi_string_t string,
		efi_uintn_t *string_size,
		struct efi_font_info **string_font_info);
	efi_status_t(EFIAPI *set_string)(
		const struct efi_hii_string_protocol *this,
		efi_hii_handle_t package_list,
		efi_string_id_t string_id,
		const u8 *language,
		const efi_string_t string,
		const struct efi_font_info *string_font_info);
	efi_status_t(EFIAPI *get_languages)(
		const struct efi_hii_string_protocol *this,
		efi_hii_handle_t package_list,
		u8 *languages,
		efi_uintn_t *languages_size);
	efi_status_t(EFIAPI *get_secondary_languages)(
		const struct efi_hii_string_protocol *this,
		efi_hii_handle_t package_list,
		const u8 *primary_language,
		u8 *secondary_languages,
		efi_uintn_t *secondary_languages_size);
};

struct efi_hii_database_protocol {
	efi_status_t(EFIAPI *new_package_list)(
		const struct efi_hii_database_protocol *this,
		const struct efi_hii_package_list_header *package_list,
		const efi_handle_t driver_handle,
		efi_hii_handle_t *handle);
	efi_status_t(EFIAPI *remove_package_list)(
		const struct efi_hii_database_protocol *this,
		efi_hii_handle_t handle);
	efi_status_t(EFIAPI *update_package_list)(
		const struct efi_hii_database_protocol *this,
		efi_hii_handle_t handle,
		const struct efi_hii_package_list_header *package_list);
	efi_status_t(EFIAPI *list_package_lists)(
		const struct efi_hii_database_protocol *this,
		u8 package_type,
		const efi_guid_t *package_guid,
		efi_uintn_t *handle_buffer_length,
		efi_hii_handle_t *handle);
	efi_status_t(EFIAPI *export_package_lists)(
		const struct efi_hii_database_protocol *this,
		efi_hii_handle_t handle,
		efi_uintn_t *buffer_size,
		struct efi_hii_package_list_header *buffer);
	efi_status_t(EFIAPI *register_package_notify)(
		const struct efi_hii_database_protocol *this,
		u8 package_type,
		const efi_guid_t *package_guid,
		const void *package_notify_fn,
		efi_uintn_t notify_type,
		efi_handle_t *notify_handle);
	efi_status_t(EFIAPI *unregister_package_notify)(
		const struct efi_hii_database_protocol *this,
		efi_handle_t notification_handle
		);
	efi_status_t(EFIAPI *find_keyboard_layouts)(
		const struct efi_hii_database_protocol *this,
		u16 *key_guid_buffer_length,
		efi_guid_t *key_guid_buffer);
	efi_status_t(EFIAPI *get_keyboard_layout)(
		const struct efi_hii_database_protocol *this,
		efi_guid_t *key_guid,
		u16 *keyboard_layout_length,
		struct efi_hii_keyboard_layout *keyboard_layout);
	efi_status_t(EFIAPI *set_keyboard_layout)(
		const struct efi_hii_database_protocol *this,
		efi_guid_t *key_guid);
	efi_status_t(EFIAPI *get_package_list_handle)(
		const struct efi_hii_database_protocol *this,
		efi_hii_handle_t package_list_handle,
		efi_handle_t *driver_handle);
};

struct efi_hii_config_routing_protocol {
	efi_status_t(EFIAPI *extract_config)(
		const struct efi_hii_config_routing_protocol *this,
		const efi_string_t request,
		efi_string_t *progress,
		efi_string_t *results);
	efi_status_t(EFIAPI *export_config)(
		const struct efi_hii_config_routing_protocol *this,
		efi_string_t *results);
	efi_status_t(EFIAPI *route_config)(
		const struct efi_hii_config_routing_protocol *this,
		const efi_string_t configuration,
		efi_string_t *progress);
	efi_status_t(EFIAPI *block_to_config)(
		const struct efi_hii_config_routing_protocol *this,
		const efi_string_t config_request,
		const uint8_t *block,
		const efi_uintn_t block_size,
		efi_string_t *config,
		efi_string_t *progress);
	efi_status_t(EFIAPI *config_to_block)(
		const struct efi_hii_config_routing_protocol *this,
		const efi_string_t config_resp,
		const uint8_t *block,
		const efi_uintn_t *block_size,
		efi_string_t *progress);
	efi_status_t(EFIAPI *get_alt_config)(
		const struct efi_hii_config_routing_protocol *this,
		const efi_string_t config_resp,
		const efi_guid_t *guid,
		const efi_string_t name,
		const struct efi_device_path *device_path,
		const efi_string_t alt_cfg_id,
		efi_string_t *alt_cfg_resp);
};

struct efi_hii_config_access_protocol {
	efi_status_t(EFIAPI *extract_config_access)(
		const struct efi_hii_config_access_protocol *this,
		const efi_string_t request,
		efi_string_t *progress,
		efi_string_t *results);
	efi_status_t(EFIAPI *route_config_access)(
		const struct efi_hii_config_access_protocol *this,
		const efi_string_t configuration,
		efi_string_t *progress);
	efi_status_t(EFIAPI *form_callback)(
		const struct efi_hii_config_access_protocol *this,
		efi_browser_action_t action,
		efi_question_id_t question_id,
		u8 type,
		union efi_ifr_type_value *value,
		efi_browser_action_request_t *action_request);
};

#endif
