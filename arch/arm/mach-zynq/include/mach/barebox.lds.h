#define PRE_IMAGE \
	.pre_image : {					\
		. = 0x20;				\
		KEEP(*(.flash_header_0x0*))		\
		. = 0xa0;				\
		KEEP(*(.ps7reg_entry_0x0A0))		\
		. = 0x8c0;				\
	}
