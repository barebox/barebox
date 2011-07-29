#ifndef __ASM_ARCH_HOSTFILE_H
#define __ASM_ARCH_HOSTFILE_H

struct hf_platform_data {
	int fd;
	size_t size;
	unsigned long base;
	char *filename;
	char *name;
};

int barebox_register_filedev(struct hf_platform_data *hf);

#endif /* __ASM_ARCH_HOSTFILE_H */

