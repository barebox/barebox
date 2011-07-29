#ifndef __ASM_ARM_MEMORY_H
#define __ASM_ARM_MEMORY_H

struct arm_memory {
	struct list_head list;
	struct device_d *dev;
	u32 *ptes;
	unsigned long start;
	unsigned long size;
};

extern struct list_head memory_list;

void armlinux_add_dram(struct device_d *dev);

#define for_each_sdram_bank(mem)	list_for_each_entry(mem, &memory_list, list)

#endif	/* __ASM_ARM_MEMORY_H */
