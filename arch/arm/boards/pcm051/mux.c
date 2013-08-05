#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <mach/am33xx-mux.h>

static const struct module_pin_mux mmc0_pin_mux[] = {
	{OFFSET(mmc0_dat3), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT3 */
	{OFFSET(mmc0_dat2), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT2 */
	{OFFSET(mmc0_dat1), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT1 */
	{OFFSET(mmc0_dat0), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT0 */
	{OFFSET(mmc0_clk), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_CLK */
	{OFFSET(mmc0_cmd), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_CMD */
	{OFFSET(spi0_cs1), (MODE(5) | RXACTIVE | PULLUP_EN)},	/* MMC0_CD */
	{-1},
};

static const struct module_pin_mux user_led_pin_mux[] = {
	{OFFSET(gpmc_csn1), MODE(7) | PULLUDEN}, /* USER LED1 */
	{OFFSET(gpmc_csn2), MODE(7) | PULLUDEN}, /* USER LED2 */
	{-1},
};

static const struct module_pin_mux user_btn_pin_mux[] = {
	{OFFSET(emu0), MODE(7) | RXACTIVE | PULLUP_EN},
	{OFFSET(emu1), MODE(7) | RXACTIVE | PULLUP_EN},
	{-1},
};

void pcm051_enable_mmc0_pin_mux(void)
{
	configure_module_pin_mux(mmc0_pin_mux);
}

void pcm051_enable_user_led_pin_mux(void)
{
	configure_module_pin_mux(user_led_pin_mux);
}

void pcm051_enable_user_btn_pin_mux(void)
{
	configure_module_pin_mux(user_btn_pin_mux);
}
