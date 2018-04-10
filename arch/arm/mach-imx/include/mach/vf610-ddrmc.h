#ifndef __MACH_DDRMC_H
#define __MACH_DDRMC_H

#include <mach/vf610-regs.h>


#define DDRMC_CR(x)	((x) * 4)

#define DDRMC_CR01_MAX_COL_REG(reg)	(((reg) >>  8) & 0b01111)
#define DDRMC_CR01_MAX_ROW_REG(reg)	(((reg) >>  0) & 0b11111)
#define DDRMC_CR73_COL_DIFF(reg)	(((reg) >> 16) & 0b00111)
#define DDRMC_CR73_ROW_DIFF(reg)	(((reg) >>  8) & 0b00011)
#define DDRMC_CR73_BANK_DIFF(reg)	(((reg) >>  0) & 0b00011)

#define DDRMC_CR78_REDUC	BIT(8)


#endif /* __MACH_MMDC_H */
