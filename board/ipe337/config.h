
#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * Clock settings
 */

/* CONFIG_CLKIN_HZ is any value in Hz                            */
#if   defined(CONFIG_MACH_IPE337_V1)
#define CONFIG_CLKIN_HZ          25000000
#elif defined(CONFIG_MACH_IPE337_V2)
#define CONFIG_CLKIN_HZ          40000000
#else
#error "Unknown IPE337 revision"
#endif

/* CONFIG_CLKIN_HALF controls what is passed to PLL 0=CLKIN      */
/*                                                  1=CLKIN/2    */
#define CONFIG_CLKIN_HALF               0
/* CONFIG_PLL_BYPASS controls if the PLL is used 0=don't bypass  */
/*                                               1=bypass PLL    */
#define CONFIG_PLL_BYPASS               0
/* CONFIG_VCO_MULT controls what the multiplier of the PLL is.   */
/* Values can range from 1-64                                    */
#define CONFIG_VCO_MULT		       10         /* POR default */
/* CONFIG_CCLK_DIV controls what the core clock divider is       */
/* Values can be 1, 2, 4, or 8 ONLY                              */
#define CONFIG_CCLK_DIV			1         /* POR default */
/* CONFIG_SCLK_DIV controls what the peripheral clock divider is */
/* Values can range from 1-15                                    */
#define CONFIG_SCLK_DIV			5         /* POR default */

/* Frequencies selected: 400MHz CCLK / 80MHz SCLK ^= 12.5ns cycle time*/

#define AMGCTLVAL               0x1F

/* no need for speed, currently, leave at defaults */
#define AMBCTL0VAL              0xFFC2FFC2
#define AMBCTL1VAL              0xFFC2FFC2

#define CONFIG_MEM_MT48LC16M16A2TG_75    1
#define CONFIG_MEM_ADD_WDTH              9           /* 8, 9, 10, 11    */
#define CONFIG_MEM_SIZE                 64           /* 128, 64, 32, 16 */

#endif	/* __CONFIG_H */
