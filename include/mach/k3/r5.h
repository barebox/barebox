#ifndef __MACH_K3_R5_H
#define __MACH_K3_R5_H

void k3_ctrl_mmr_unlock(void);
void k3_mpu_setup_regions(void);
void am625_early_init(void);
struct rproc *ti_k3_am64_get_handle(void);

#endif /* __MACH_K3_R5_H */
