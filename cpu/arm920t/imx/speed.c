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

typedef enum imx_cookies {
        PARAM_SYSCLOCK,
        PARAM_PERCLK1,
        PARAM_PERCLK2,
        PARAM_PERCLK3,
        PARAM_BCLK,
        PARAM_HCLK,
        PARAM_FCLK,
        PARAM_CPUCLK,
        PARAM_ARCH_NUMBER,
        PARAM_LAST,
} imx_cookies_t;

char *clk_get(struct device_d* dev, ulong cookie)
{
        ulong clock = 0;
        static char buf[11];

        switch (cookie) {
        case PARAM_SYSCLOCK:
                clock = get_systemPLLCLK();
                break;
        case PARAM_PERCLK1:
                clock = get_PERCLK1();
                break;
        case PARAM_PERCLK2:
                clock = get_PERCLK2();
                break;
        case PARAM_PERCLK3:
                clock = get_PERCLK3();
                break;
        case PARAM_BCLK:
                clock = get_BCLK();
                break;
        case PARAM_HCLK:
                clock = get_HCLK();
                break;
        case PARAM_FCLK:
                clock = get_FCLK();
                break;
        case PARAM_CPUCLK:
                clock = get_mcuPLLCLK();
                break;
        }

        sprintf(buf, "%ld",clock);
        return buf;
}

static int arch_number = CONFIG_ARCH_NUMBER;

static char *arch_number_get(struct device_d* dev, ulong cookie)
{
        static char buf[5];

        sprintf(buf, "%d", arch_number);

        return buf;
}

static int arch_number_set(struct device_d* dev, ulong cookie, char *newval)
{
        arch_number = simple_strtoul(newval, NULL, 10);
        return 0;
}

static struct param_d imx_params[] = {
        { .name = "imx_system_clk", .cookie = PARAM_SYSCLOCK, .get = clk_get},
        { .name = "imx_perclk1", .cookie = PARAM_PERCLK1, .get = clk_get},
        { .name = "imx_perclk2", .cookie = PARAM_PERCLK2, .get = clk_get},
        { .name = "imx_perclk3", .cookie = PARAM_PERCLK3, .get = clk_get},
        { .name = "imx_bclk", .cookie = PARAM_BCLK, .get = clk_get},
        { .name = "imx_hclk", .cookie = PARAM_HCLK, .get = clk_get},
        { .name = "imx_fclk", .cookie = PARAM_FCLK, .get = clk_get},
        { .name = "imx_cpuclk", .cookie = PARAM_CPUCLK, .get = clk_get},
        { .name = "arch_number", .cookie = PARAM_CPUCLK, .get = arch_number_get, .set = arch_number_set},
};

static int imx_clk_init(void)
{
        int i;

        for (i = 0; i < PARAM_LAST; i++)
                global_add_parameter(&imx_params[i]);

        return 0;
}

device_initcall(imx_clk_init);

