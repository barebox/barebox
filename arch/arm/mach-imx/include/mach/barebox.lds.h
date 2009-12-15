
#ifdef CONFIG_ARCH_IMX_INTERNAL_BOOT

#define PRE_IMAGE \
	.pre_image : {					\
		KEEP(*(.flash_header_start*))		\
		. = 0x100;				\
		KEEP(*(.flash_header_0x100*))		\
		KEEP(*(.dcd_entry_0x100*))		\
		KEEP(*(.image_len_0x100*))		\
		. = 0x400;				\
		KEEP(*(.flash_header_0x400*))		\
		KEEP(*(.dcd_entry_0x400*))		\
		KEEP(*(.image_len_0x400*))		\
		. = 0x1000;				\
		KEEP(*(.flash_header_0x1000*))		\
		KEEP(*(.dcd_entry_0x1000*))		\
		KEEP(*(.image_len_0x1000*))		\
		. = 0x2000;				\
	}
#endif

