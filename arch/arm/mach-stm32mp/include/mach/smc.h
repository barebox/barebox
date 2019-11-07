#ifndef __MACH_STM32_SMC_H__
#define __MACH_STM32_SMC_H__

#include <linux/arm-smccc.h>

/* Secure Service access from Non-secure */
#define STM32_SMC_RCC                   0x82001000
#define STM32_SMC_PWR                   0x82001001
#define STM32_SMC_RTC                   0x82001002
#define STM32_SMC_BSEC                  0x82001003

/* Register access service use for RCC/RTC/PWR */
#define STM32_SMC_REG_WRITE             0x1
#define STM32_SMC_REG_SET               0x2
#define STM32_SMC_REG_CLEAR             0x3

static inline int stm32mp_smc(u32 svc, u8 op, u32 data1, u32 data2, u32 *val)
{
        struct arm_smccc_res res;

        arm_smccc_smc(svc, op, data1, data2, 0, 0, 0, 0, &res);
        if (val)
                *val = res.a1;

        return (int)res.a0;
}

#endif
