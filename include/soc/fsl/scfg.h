#ifndef __SOC_FSL_SCFG_H
#define __SOC_FSL_SCFG_H

#include <soc/fsl/scfg.h>
#include <linux/compiler.h>

enum scfg_endianess {
	SCFG_ENDIANESS_INVALID,
	SCFG_ENDIANESS_LITTLE,
	SCFG_ENDIANESS_BIG,
};

void scfg_clrsetbits32(void __iomem *addr, u32 clear, u32 set);
void scfg_clrbits32(void __iomem *addr, u32 clear);
void scfg_setbits32(void __iomem *addr, u32 set);
void scfg_out16(void __iomem *addr, u16 val);
void scfg_init(enum scfg_endianess endianess);

#endif /* __SOC_FSL_SCFG_H */
