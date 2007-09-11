
/*
 * Board Layout
 */
#define CONFIG_MALLOC_LEN	(16384 << 10)
#define CONFIG_MALLOC_BASE	(TEXT_BASE - CONFIG_MALLOC_LEN)
#define CONFIG_STACKBASE	(CONFIG_MALLOC_BASE - 4)

/*
 * Clock settings
 */

/* CONFIG_CLKIN_HZ is any value in Hz                            */
#define CONFIG_CLKIN_HZ          25000000
/* CONFIG_CLKIN_HALF controls what is passed to PLL 0=CLKIN      */
/*                                                  1=CLKIN/2    */
#define CONFIG_CLKIN_HALF               0
/* CONFIG_PLL_BYPASS controls if the PLL is used 0=don't bypass  */
/*                                               1=bypass PLL    */
#define CONFIG_PLL_BYPASS               0
/* CONFIG_VCO_MULT controls what the multiplier of the PLL is.   */
/* Values can range from 1-64                                    */
#define CONFIG_VCO_MULT			22
/* CONFIG_CCLK_DIV controls what the core clock divider is       */
/* Values can be 1, 2, 4, or 8 ONLY                              */
#define CONFIG_CCLK_DIV			4
/* CONFIG_SCLK_DIV controls what the peripheral clock divider is */
/* Values can range from 1-15                                    */
#define CONFIG_SCLK_DIV			7

#define AMGCTLVAL               0x1F
#define AMBCTL0VAL              0xFFC2FFC2
#define AMBCTL1VAL              0xFFC2FFC2

#define CONFIG_MEM_MT48LC64M4A2FB_7E    1
#define CONFIG_MEM_ADD_WDTH             9             /* 8, 9, 10, 11    */
#define CONFIG_MEM_SIZE                 128           /* 128, 64, 32, 16 */

