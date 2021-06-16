#ifndef __MACH_ROCKCHIP_H
#define __MACH_ROCKCHIP_H

#ifdef CONFIG_ARCH_RK3188
int rk3188_init(void);
#else
static inline int rk3188_init(void)
{
	return -ENOTSUPP;
}
#endif

#ifdef CONFIG_ARCH_RK3288
int rk3288_init(void);
#else
static inline int rk3288_init(void)
{
	return -ENOTSUPP;
}
#endif

#endif /* __MACH_ROCKCHIP_H */
