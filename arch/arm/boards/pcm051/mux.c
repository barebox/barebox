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
	{OFFSET(spi0_cs1), (MODE(7) | RXACTIVE | PULLUP_EN)},	/* MMC0_CD */
	{-1},
};

static const struct module_pin_mux nand_pin_mux[] = {
	{OFFSET(gpmc_ad0), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD0 */
	{OFFSET(gpmc_ad1), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD1 */
	{OFFSET(gpmc_ad2), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD2 */
	{OFFSET(gpmc_ad3), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD3 */
	{OFFSET(gpmc_ad4), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD4 */
	{OFFSET(gpmc_ad5), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD5 */
	{OFFSET(gpmc_ad6), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD6 */
	{OFFSET(gpmc_ad7), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD7 */
	{OFFSET(gpmc_wait0), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* NAND WAIT */
	{OFFSET(gpmc_csn0), (MODE(0) | PULLUDEN)},	/* NAND_CS0 */
	{OFFSET(gpmc_advn_ale), (MODE(0) | PULLUDEN)},	/* NAND_ADV_ALE */
	{OFFSET(gpmc_oen_ren), (MODE(0) | PULLUDEN)},	/* NAND_OE */
	{OFFSET(gpmc_wen), (MODE(0) | PULLUDEN)},	/* NAND_WEN */
	{OFFSET(gpmc_be0n_cle), (MODE(0) | PULLUDEN)},	/* NAND_BE_CLE */
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

void pcm051_enable_nand_pin_mux(void)
{
	configure_module_pin_mux(nand_pin_mux);
}

void pcm051_enable_user_led_pin_mux(void)
{
	configure_module_pin_mux(user_led_pin_mux);
}

void pcm051_enable_user_btn_pin_mux(void)
{
	configure_module_pin_mux(user_btn_pin_mux);
}
