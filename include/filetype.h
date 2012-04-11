#ifndef __FILE_TYPE_H
#define __FILE_TYPE_H

/*
 * List of file types we know
 */
enum filetype {
	filetype_unknown,
	filetype_arm_zimage,
	filetype_lzo_compressed,
	filetype_arm_barebox,
	filetype_uimage,
	filetype_ubi,
	filetype_jffs2,
	filetype_gzip,
	filetype_bzip2,
	filetype_oftree,
	filetype_aimage,
};

const char *file_type_to_string(enum filetype f);
enum filetype file_detect_type(void *_buf);
enum filetype file_name_detect_type(const char *filename);

#endif /* __FILE_TYPE_H */
