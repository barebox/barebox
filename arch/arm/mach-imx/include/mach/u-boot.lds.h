
#define PRE_IMAGE .pre_image : {			\
		KEEP(*(.flash_header_start*))		\
		. = ALIGN(0x400);			\
		KEEP(*(.flash_header*))			\
		KEEP(*(.dcd_entry*))			\
		KEEP(*(.image_len*))			\
	}

