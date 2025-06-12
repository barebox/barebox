#ifndef __DTS_TI_K3_PINCTRL_BAREBOX_H
#define __DTS_TI_K3_PINCTRL_BAREBOX_H

#include <arm64/ti/k3-pinctrl.h>
#define AM62LX_IOPAD(pa, val, muxmode)         (((pa) & 0x1fff)) ((val) | (muxmode))

#endif /* __DTS_TI_K3_PINCTRL_BAREBOX_H */
