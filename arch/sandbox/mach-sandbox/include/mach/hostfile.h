#ifndef __ASM_ARCH_HOSTFILE_H
#define __ASM_ARCH_HOSTFILE_H

struct hf_info {
	int fd;
	unsigned long long base;
	unsigned long long size;
	const char *devname;
	const char *filename;
	unsigned int is_blockdev:1;
	unsigned int is_cdev:1;
	unsigned int is_readonly:1;
};

int barebox_register_filedev(struct hf_info *hf);

#endif /* __ASM_ARCH_HOSTFILE_H */
