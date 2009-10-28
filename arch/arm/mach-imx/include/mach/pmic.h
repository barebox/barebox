#ifndef __ASM_ARCH_PMIC_H
#define __ASM_ARCH_PMIC_H

/* The only function the PMIC driver currently exports. It's purpose
 * is to adjust the switchers to 1.45V in order to speed up the CPU
 * to 400MHz. This is probably board dependent, so we have to think
 * about a proper API for the PMIC
 */
int pmic_power(void);

#endif /* __ASM_ARCH_PMIC_H */
