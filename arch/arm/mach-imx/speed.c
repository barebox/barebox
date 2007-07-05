/*
 *
 * (c) 2004 Sascha Hauer <sascha@saschahauer.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <common.h>
#include <asm/arch/imx-regs.h>
#include <init.h>
#include <driver.h>

/* ------------------------------------------------------------------------- */
/* NOTE: This describes the proper use of this file.
 *
 * CONFIG_SYS_CLK_FREQ should be defined as the input frequency of the PLL.
 * SH FIXME: 16780000 in our case
 * get_FCLK(), get_HCLK(), get_PCLK() and get_UCLK() return the clock of
 * the specified bus in HZ.
 */
/* ------------------------------------------------------------------------- */

ulong get_systemPLLCLK(void)
{
	/* FIXME: We assume System_SEL = 0 here */
	u32 spctl0 = SPCTL0;
	u32 mfi = (spctl0 >> 10) & 0xf;
	u32 mfn = spctl0 & 0x3f;
	u32 mfd = (spctl0 >> 16) & 0x3f;
	u32 pd =  (spctl0 >> 26) & 0xf;

	mfi = mfi<=5 ? 5 : mfi;

	return (2*(CONFIG_SYSPLL_CLK_FREQ>>10)*( (mfi<<10) + (mfn<<10)/(mfd+1)))/(pd+1);
}

ulong get_mcuPLLCLK(void)
{
	/* FIXME: We assume System_SEL = 0 here */
	u32 mpctl0 = MPCTL0;
	u32 mfi = (mpctl0 >> 10) & 0xf;
	u32 mfn = mpctl0 & 0x3f;
	u32 mfd = (mpctl0 >> 16) & 0x3f;
	u32 pd =  (mpctl0 >> 26) & 0xf;

	mfi = mfi<=5 ? 5 : mfi;

	return (2*(CONFIG_SYS_CLK_FREQ>>10)*( (mfi<<10) + (mfn<<10)/(mfd+1)))/(pd+1);
}

ulong get_FCLK(void)
{
	return (( CSCR>>15)&1) ? get_mcuPLLCLK()>>1 : get_mcuPLLCLK();
}

ulong get_HCLK(void)
{
	u32 bclkdiv = (( CSCR >> 10 ) & 0xf) + 1;
	return get_systemPLLCLK() / bclkdiv;
}

ulong get_BCLK(void)
{
	return get_HCLK();
}

ulong get_PERCLK1(void)
{
	return get_systemPLLCLK() / (((PCDR) & 0xf)+1);
}

ulong get_PERCLK2(void)
{
	return get_systemPLLCLK() / (((PCDR>>4) & 0xf)+1);
}

ulong get_PERCLK3(void)
{
	return get_systemPLLCLK() / (((PCDR>>16) & 0x7f)+1);
}

#if 0
typedef enum imx_cookies {
        PARAM_CPUCLK,
        PARAM_SYSCLOCK,
        PARAM_PERCLK1,
        PARAM_PERCLK2,
        PARAM_PERCLK3,
        PARAM_BCLK,
        PARAM_HCLK,
        PARAM_FCLK,
        PARAM_ARCH_NUMBER,
        PARAM_LAST,
} imx_cookies_t;

static struct param_d imx_params[] = {
        [PARAM_CPUCLK]      = { .name = "imx_cpuclk", .flags = PARAM_FLAG_RO},
        [PARAM_SYSCLOCK]    = { .name = "imx_system_clk", .flags = PARAM_FLAG_RO},
        [PARAM_PERCLK1]     = { .name = "imx_perclk1", .flags = PARAM_FLAG_RO},
        [PARAM_PERCLK2]     = { .name = "imx_perclk2", .flags = PARAM_FLAG_RO},
        [PARAM_PERCLK3]     = { .name = "imx_perclk3", .flags = PARAM_FLAG_RO},
        [PARAM_BCLK]        = { .name = "imx_bclk", .flags = PARAM_FLAG_RO},
        [PARAM_HCLK]        = { .name = "imx_hclk", .flags = PARAM_FLAG_RO},
        [PARAM_FCLK]        = { .name = "imx_fclk", .flags = PARAM_FLAG_RO},
        [PARAM_ARCH_NUMBER] = { .name = "arch_number",},
};

static int imx_clk_init(void)
{
        int i;

	imx_params[PARAM_CPUCLK].value.val_ulong = get_mcuPLLCLK();
	imx_params[PARAM_SYSCLOCK].value.val_ulong = get_systemPLLCLK();
	imx_params[PARAM_PERCLK1].value.val_ulong = get_PERCLK1();
	imx_params[PARAM_PERCLK2].value.val_ulong = get_PERCLK2();
	imx_params[PARAM_PERCLK3].value.val_ulong = get_PERCLK3();
	imx_params[PARAM_BCLK].value.val_ulong = get_BCLK();
	imx_params[PARAM_HCLK].value.val_ulong = get_HCLK();
	imx_params[PARAM_FCLK].value.val_ulong = get_FCLK();
	imx_params[PARAM_ARCH_NUMBER].value.val_ulong = arch_number;

        for (i = 0; i < PARAM_LAST; i++)
                global_add_parameter(&imx_params[i]);

        return 0;
}

device_initcall(imx_clk_init);
#endif
