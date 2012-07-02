#ifndef __MACH_POWER_H
#define __MACH_POWER_H

void imx_power_prepare_usbphy(void);
int imx_get_vddio(void);
int imx_set_vddio(int);

#endif /* __MACH_POWER_H */
