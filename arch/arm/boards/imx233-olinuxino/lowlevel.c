#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx23-regs.h>
#include <mach/init.h>
#include <io.h>
#include <debug_ll.h>
#include <mach/iomux.h>
#include <generated/mach-types.h>

ENTRY_FUNCTION(start_barebox_olinuxino_imx23, r0, r1, r2)
{
	barebox_arm_entry(IMX_MEMORY_BASE, SZ_64M, (void *)MACH_TYPE_IMX233_OLINUXINO);
}

static const uint32_t pad_setup[] = {
	/* debug port */
	PWM1_DUART_TX | STRENGTH(S4MA),    /* PWM0/DUART_TXD - U_DEBUG PIN 2 */
	PWM0_DUART_RX | STRENGTH(S4MA),    /* PWM0/DUART_RXD - U_DEBUG PIN 1 */

	/* SDRAM */
	EMI_D0 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D1 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D2 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D3 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D4 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D5 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D6 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D7 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D8 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D9 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D10 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D11 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D12 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D13 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D14 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_D15 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_DQM0 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_DQM1 | VE_2_5V | STRENGTH(S12MA) | PULLUP(1),
	EMI_DQS0 | VE_2_5V | STRENGTH(S12MA),
	EMI_DQS1 | VE_2_5V | STRENGTH(S12MA),

	EMI_CLK | VE_2_5V | STRENGTH(S12MA),
	EMI_CLKN | VE_2_5V | STRENGTH(S12MA),
	EMI_A0 | VE_2_5V | STRENGTH(S12MA),
	EMI_A1 | VE_2_5V | STRENGTH(S12MA),
	EMI_A2 | VE_2_5V | STRENGTH(S12MA),
	EMI_A3 | VE_2_5V | STRENGTH(S12MA),
	EMI_A4 | VE_2_5V | STRENGTH(S12MA),
	EMI_A5 | VE_2_5V | STRENGTH(S12MA),
	EMI_A6 | VE_2_5V | STRENGTH(S12MA),
	EMI_A7 | VE_2_5V | STRENGTH(S12MA),
	EMI_A8 | VE_2_5V | STRENGTH(S12MA),
	EMI_A9 | VE_2_5V | STRENGTH(S12MA),
	EMI_A10 | VE_2_5V | STRENGTH(S12MA),
	EMI_A11 | VE_2_5V | STRENGTH(S12MA),
	EMI_A12 | VE_2_5V | STRENGTH(S12MA),
	EMI_BA0 | VE_2_5V | STRENGTH(S12MA),
	EMI_BA1 | VE_2_5V | STRENGTH(S12MA),

	EMI_CASN | VE_2_5V | STRENGTH(S12MA),
	EMI_CE0N | VE_2_5V | STRENGTH(S12MA),
	EMI_CE1N | VE_2_5V | STRENGTH(S12MA),
	EMI_CKE | VE_2_5V | STRENGTH(S12MA),
	EMI_RASN | VE_2_5V | STRENGTH(S12MA),
	EMI_WEN | VE_2_5V | STRENGTH(S12MA),

	/* auart */
	I2C_SDA_AUART1_RX | STRENGTH(S4MA),
	I2C_CLK_AUART1_TX | STRENGTH(S4MA),

	/* LCD */
	LCD_D17 | STRENGTH(S12MA),         /*PIN18/LCD_D17   -   GPIO PIN 3 */
	LCD_D16 | STRENGTH(S12MA),
	LCD_D15 | STRENGTH(S12MA),
	LCD_D14 | STRENGTH(S12MA),
	LCD_D13 | STRENGTH(S12MA),
	LCD_D12 | STRENGTH(S12MA),
	LCD_D11 | STRENGTH(S12MA),
	LCD_D10 | STRENGTH(S12MA),
	LCD_D9 | STRENGTH(S12MA),
	LCD_D8 | STRENGTH(S12MA),
	LCD_D7 | STRENGTH(S12MA),
	LCD_D6 | STRENGTH(S12MA),
	LCD_D5 | STRENGTH(S12MA),
	LCD_D4 | STRENGTH(S12MA),
	LCD_D3 | STRENGTH(S12MA),
	LCD_D2 | STRENGTH(S12MA),            /* PIN3/LCD_D02   - GPIO PIN 31*/
	LCD_D1 | STRENGTH(S12MA),            /* PIN2/LCD_D01   - GPIO PIN 33*/
	LCD_D0 | STRENGTH(S12MA),            /* PIN1/LCD_D00   - GPIO PIN 35*/
	LCD_CS,                              /* PIN26/LCD_CS   - GPIO PIN 20*/
	LCD_RS,                              /* PIN25/LCD_RS   - GPIO PIN 18*/
	LCD_WR,                              /* PIN24/LCD_WR   - GPIO PIN 16*/
	LCD_RESET,                           /* PIN23/LCD_DISP - GPIO PIN 14*/
	LCD_ENABE | STRENGTH(S12MA),  /* PIN22/LCD_EN/I2C_SCL  - GPIO PIN 12*/
	LCD_VSYNC | STRENGTH(S12MA), /* PIN21/LCD_HSYNC/I2C_SDA- GPIO PIN 10*/
	LCD_HSYNC | STRENGTH(S12MA),     /* PIN20/LCD_VSYNC    - GPIO PIN  8*/
	LCD_DOTCLOCK | STRENGTH(S12MA),  /* PIN19/LCD_DOTCLK   - GPIO PIN  6*/

	/* SD card interface */
	SSP1_DATA0 | PULLUP(1),
	SSP1_DATA1 | PULLUP(1),
	SSP1_DATA2 | PULLUP(1),
	SSP1_DATA3 | PULLUP(1),
	SSP1_SCK,
	SSP1_CMD | PULLUP(1),
	SSP1_DETECT | PULLUP(1),

	/* LED */
	SSP1_DETECT_GPIO | GPIO_OUT | GPIO_VALUE(1),

	/* GPIO - USB hub LAN9512-JZX*/
	GPMI_ALE_GPIO | GPIO_OUT | GPIO_VALUE(1),
};


/* Fine-tune the DRAM configuration. */
static void imx23_olinuxino_adjust_memory_params(uint32_t *dram_vals)
{
	/* Enable Auto Precharge. */
	dram_vals[3] |= 1 << 8;
	/* Enable Fast Writes. */
	dram_vals[5] |= 1 << 8;
	/* tEMRS = 3*tCK */
	dram_vals[10] &= ~(0x3 << 8);
	dram_vals[10] |= (0x3 << 8);
	/* CASLAT = 3*tCK */
	dram_vals[11] &= ~(0x3 << 0);
	dram_vals[11] |= (0x3 << 0);
	/* tCKE = 1*tCK */
	dram_vals[12] &= ~(0x7 << 0);
	dram_vals[12] |= (0x1 << 0);
	/* CASLAT_LIN_GATE = 3*tCK , CASLAT_LIN = 3*tCK, tWTR=2*tCK */
	dram_vals[13] &= ~((0xf << 16) | (0xf << 24) | (0xf << 0));
	dram_vals[13] |= (0x6 << 16) | (0x6 << 24) | (0x2 << 0);
	/* tDAL = 6*tCK */
	dram_vals[15] &= ~(0xf << 16);
	dram_vals[15] |= (0x6 << 16);
	/* tREF = 1040*tCK */
	dram_vals[26] &= ~0xffff;
	dram_vals[26] |= 0x0410;
	/* tRAS_MAX = 9334*tCK */
	dram_vals[32] &= ~0xffff;
	dram_vals[32] |= 0x2475;
}

static noinline void imx23_olinuxino_init(void)
{
	int i;

	/* initizalize gpios */
	for (i = 0; i < ARRAY_SIZE(pad_setup); i++)
		imx_gpio_mode(pad_setup[i]);

	pr_debug("initializing power...\n");

	mx23_power_init(POWER_USE_5V, &mx23_power_default);

	pr_debug("initializing SDRAM...\n");

	imx23_olinuxino_adjust_memory_params(mx23_dram_vals);
	/* EMI_CLK of 480 / 2 * (18/33) = 130.90 MHz */
	mxs_mem_init_clock(2, 33);
	mx23_mem_init();

	pr_debug("DONE\n");
}

ENTRY_FUNCTION(prep_start_barebox_olinuxino_imx23, r0, r1, r2)
{
	void (*back)(unsigned long) = (void *)get_lr();

	relocate_to_current_adr();
	setup_c();

	imx23_olinuxino_init();

	back(0);
}
