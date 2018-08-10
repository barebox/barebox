#ifndef __MACH_RESET_REASON_H__
#define __MACH_RESET_REASON_H__

#include <reset_source.h>

#define IMX_SRC_SRSR_IPP_RESET		BIT(0)
#define IMX_SRC_SRSR_CSU_RESET		BIT(2)
#define IMX_SRC_SRSR_IPP_USER_RESET	BIT(3)
#define IMX_SRC_SRSR_WDOG1_RESET	BIT(4)
#define IMX_SRC_SRSR_JTAG_RESET		BIT(5)
#define IMX_SRC_SRSR_JTAG_SW_RESET	BIT(6)
#define IMX_SRC_SRSR_WDOG3_RESET	BIT(7)
#define IMX_SRC_SRSR_WDOG4_RESET	BIT(8)
#define IMX_SRC_SRSR_TEMPSENSE_RESET	BIT(9)
#define IMX_SRC_SRSR_WARM_BOOT		BIT(16)

#define IMX_SRC_SRSR	0x008
#define IMX7_SRC_SRSR	0x05c

#define VF610_SRC_SRSR_SW_RST		BIT(18)
#define VF610_SRC_SRSR_RESETB		BIT(7)
#define VF610_SRC_SRSR_JTAG_RST		BIT(5)
#define VF610_SRC_SRSR_WDOG_M4		BIT(4)
#define VF610_SRC_SRSR_WDOG_A5		BIT(3)
#define VF610_SRC_SRSR_POR_RST		BIT(0)

struct imx_reset_reason {
	uint32_t mask;
	enum reset_src_type type;
	int instance;
};

void imx_set_reset_reason(void __iomem *, const struct imx_reset_reason *);

extern const struct imx_reset_reason imx_reset_reasons[];
extern const struct imx_reset_reason imx7_reset_reasons[];

#endif /* __MACH_RESET_REASON_H__ */
