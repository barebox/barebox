/*
 * (C) Copyright 2009
 * Juergen Beisert, Pengutronix
 *
 * (C) Copyright 2001-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, d.mueller@elsoft.ch
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

uint32_t s3c_get_mpllclk(void);
uint32_t s3c_get_upllclk(void);
uint32_t s3c_get_fclk(void);
uint32_t s3c_get_hclk(void);
uint32_t s3c_get_pclk(void);
uint32_t s3c_get_uclk(void);

unsigned s3c_get_uart_clk(unsigned src);

#ifdef CONFIG_ARCH_S3C24xx
uint32_t s3c24xx_get_memory_size(void);
void s3c24xx_disable_second_sdram_bank(void);
#endif

#ifdef CONFIG_ARCH_S5PCxx
void s5p_init_pll(void);
#endif

#ifdef CONFIG_ARCH_S3C64xx
unsigned s3c_set_epllclk(unsigned, unsigned, unsigned, unsigned);
uint32_t s3c_get_epllclk(void);
unsigned s3c_get_hsmmc_clk(int);
void s3c_set_hsmmc_clk(int, int, unsigned);
unsigned s3c6410_get_memory_size(void);
struct s3c6410_chipselect {
	unsigned adr_setup_t; /* in [ns] */
	unsigned access_setup_t; /* in [ns] */
	unsigned access_t; /* in [ns] */
	unsigned cs_hold_t; /* in [ns] */
	unsigned adr_hold_t; /* in [ns] */
	unsigned char width; /* 8 or 16 */
};
int s3c6410_setup_chipselect(int, const struct s3c6410_chipselect*);
#endif
