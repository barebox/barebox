#ifndef __IMX8_DDRC_H__
#define __IMX8_DDRC_H__

#include <mach/imx8mq-regs.h>
#include <io.h>
#include <firmware.h>
#include <linux/compiler.h>

enum ddrc_phy_firmware_offset {
	DDRC_PHY_IMEM = 0x00050000U,
	DDRC_PHY_DMEM = 0x00054000U,
};

void ddrc_phy_load_firmware(void __iomem *,
			    enum ddrc_phy_firmware_offset,
			    const u16 *, size_t);

#define DDRC_PHY_REG(x)	((x) * 4)

void ddrc_phy_wait_training_complete(void __iomem *phy);


/*
 * i.MX8M DDR Tool compatibility layer
 */

#define reg32_write(a, v)	writel(v, a)
#define reg32_read(a)		readl(a)

static inline void wait_ddrphy_training_complete(void)
{
	ddrc_phy_wait_training_complete(IOMEM(MX8MQ_DDRC_PHY_BASE_ADDR));
}

#define __ddr_load_train_code(imem, dmem)				\
	do {								\
		const u16 *__mem;					\
		size_t __size;						\
									\
		get_builtin_firmware(imem, &__mem, &__size);		\
		ddrc_phy_load_firmware(IOMEM(MX8MQ_DDRC_PHY_BASE_ADDR),	\
				       DDRC_PHY_IMEM, __mem, __size);	\
									\
		get_builtin_firmware(dmem, &__mem, &__size);		\
		ddrc_phy_load_firmware(IOMEM(MX8MQ_DDRC_PHY_BASE_ADDR),	\
				       DDRC_PHY_DMEM, __mem, __size);	\
	} while (0)

#define ddr_load_train_code(imem_dmem) __ddr_load_train_code(imem_dmem)

#define DDRC_IPS_BASE_ADDR(X) (0x3d400000 + (X * 0x2000000))

#define DDRC_STAT(X)             (DDRC_IPS_BASE_ADDR(X) + 0x04)
#define DDRC_MRSTAT(X)           (DDRC_IPS_BASE_ADDR(X) + 0x18)
#define DDRC_PWRCTL(X)           (DDRC_IPS_BASE_ADDR(X) + 0x30)
#define DDRC_RFSHCTL3(X)         (DDRC_IPS_BASE_ADDR(X) + 0x60)
#define DDRC_CRCPARSTAT(X)       (DDRC_IPS_BASE_ADDR(X) + 0xcc)
#define DDRC_DFIMISC(X)          (DDRC_IPS_BASE_ADDR(X) + 0x1b0)
#define DDRC_DFISTAT(X)          (DDRC_IPS_BASE_ADDR(X) + 0x1bc)
#define DDRC_SWCTL(X)            (DDRC_IPS_BASE_ADDR(X) + 0x320)
#define DDRC_SWSTAT(X)           (DDRC_IPS_BASE_ADDR(X) + 0x324)
#define DDRC_PCTRL_0(X)          (DDRC_IPS_BASE_ADDR(X) + 0x490)

#define IP2APB_DDRPHY_IPS_BASE_ADDR(X) (0x3c000000 + (X * 0x2000000))

#endif