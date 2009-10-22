
#define __flash_header_start	__section(.flash_header_start)
#define __flash_header		__section(.flash_header)
#define __dcd_entry		__section(.dcd_entry)
#define __image_len		__section(.image_len)

struct mx25_dcd_entry {
	unsigned long ptr_type;
	unsigned long addr;
	unsigned long val;
};

#define DCD_BARKER	0xb17219e9

struct mx25_dcd_header {
	unsigned long barker;
	unsigned long block_len;
};

struct mx25_rsa_public_key {
	unsigned char	rsa_exponent[4];
	unsigned char	*rsa_modululs;
	unsigned short	*exponent_size;
	unsigned short	modulus_size;
	unsigned char	init_flag;
};

#define APP_CODE_BARKER	0x000000b1

struct mx25_flash_header {
	void				*app_code_jump_vector;
	unsigned long			app_code_barker;
	void				*app_code_csf;
	struct mx25_dcd_header		**dcd_ptr_ptr;
	struct mx25_rsa_public_key	*super_root_key;
	struct mx25_dcd_header		*dcd;
	void				*app_dest;
};

struct mx25_nand_flash_header {
	struct mx25_flash_header	flash_header;
	struct mx25_dcd_header		dcd_header;
};

