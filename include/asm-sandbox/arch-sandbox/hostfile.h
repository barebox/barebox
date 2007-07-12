#ifndef __ASM_ARCH_HOSTFILE_H
#define __ASM_ARCH_HOSTFILE_H

struct hf_platform_data {
	int fd;
	size_t size;
	unsigned long map_base;
	char *filename;
};

int u_boot_register_filedev(struct hf_platform_data *hf, char *name_template);

#endif /* __ASM_ARCH_HOSTFILE_H */

