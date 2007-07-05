#ifndef __INCLUDE_ASM_ARCH_COMMON_H
#define __INCLUDE_ASM_ARCH_COMMON_H

typedef MPC86xx_SYS_INFO sys_info_t;
void   get_sys_info  ( sys_info_t * );
void	cpu_init_f    (void);

#endif  /* __INCLUDE_ASM_ARCH_COMMON_H */
