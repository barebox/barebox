/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * Generated code from MX8M_DDR_tool
 */

#include "ddr.h"

void ddr_cfg_phy(void);
void ddr_init(void)
{
	volatile unsigned int tmp, tmp_t;
	
	/** Initialize DDR clock and DDRC registers **/
	reg32_write(0x3038a088,0x7070000);
	reg32_write(0x3038a084,0x4030000);
	reg32_write(0x303a00ec,0xffff);
	tmp=reg32_read(0x303a00f8);
	tmp |= 0x20;
	reg32_write(0x303a00f8,tmp);
	reg32_write(0x30391000,0x8f000000);
	reg32_write(0x30391004,0x8f000000);
	reg32_write(0x30360068,0xece580);
	tmp=reg32_read(0x30360060);
	tmp &= ~0x80;
	reg32_write(0x30360060,tmp);
	tmp=reg32_read(0x30360060);
	tmp |= 0x200;
	reg32_write(0x30360060,tmp);
	tmp=reg32_read(0x30360060);
	tmp &= ~0x20;
	reg32_write(0x30360060,tmp);
	tmp=reg32_read(0x30360060);
	tmp &= ~0x10;
	reg32_write(0x30360060,tmp);
	do{
		tmp=reg32_read(0x30360060);
		if(tmp&0x80000000) break;
	}while(1);
	reg32_write(0x30391000,0x8f000006);
	reg32_write(0x3d400304,0x1);
	reg32_write(0x3d400030,0x1);
	reg32_write(0x3d400000,0xa3080020);
	reg32_write(0x3d400028,0x0);
	reg32_write(0x3d400020,0x203);
	reg32_write(0x3d400024,0x186a000);
	reg32_write(0x3d400064,0x6100e0);
	reg32_write(0x3d4000d0,0xc003061c);
	reg32_write(0x3d4000d4,0x9e0000);
	reg32_write(0x3d4000dc,0xd4002d);
	reg32_write(0x3d4000e0,0x310008);
	reg32_write(0x3d4000e8,0x66004a);
	reg32_write(0x3d4000ec,0x16004a);
	reg32_write(0x3d400100,0x1a201b22);
	reg32_write(0x3d400104,0x60633);
	reg32_write(0x3d40010c,0xc0c000);
	reg32_write(0x3d400110,0xf04080f);
	reg32_write(0x3d400114,0x2040c0c);
	reg32_write(0x3d400118,0x1010007);
	reg32_write(0x3d40011c,0x401);
	reg32_write(0x3d400130,0x20600);
	reg32_write(0x3d400134,0xc100002);
	reg32_write(0x3d400138,0xe6);
	reg32_write(0x3d400144,0xa00050);
	reg32_write(0x3d400180,0x3200018);
	reg32_write(0x3d400184,0x28061a8);
	reg32_write(0x3d400188,0x0);
	reg32_write(0x3d400190,0x497820a);
	reg32_write(0x3d400194,0x80303);
	reg32_write(0x3d4001a0,0xe0400018);
	reg32_write(0x3d4001a4,0xdf00e4);
	reg32_write(0x3d4001a8,0x80000000);
	reg32_write(0x3d4001b0,0x11);
	reg32_write(0x3d4001b4,0x170a);
	reg32_write(0x3d4001c0,0x1);
	reg32_write(0x3d4001c4,0x1);
	reg32_write(0x3d4000f4,0x639);
	reg32_write(0x3d400108,0x70e1214);
	reg32_write(0x3d400200,0x15);
	reg32_write(0x3d40020c,0x0);
	reg32_write(0x3d400210,0x1f1f);
	reg32_write(0x3d400204,0x80808);
	reg32_write(0x3d400214,0x7070707);
	reg32_write(0x3d400218,0x48080707);
	reg32_write(0x3d402020,0x1);
	reg32_write(0x3d402024,0x518b00);
	reg32_write(0x3d402050,0x20d040);
	reg32_write(0x3d402064,0x14002f);
	reg32_write(0x3d4020dc,0x940009);
	reg32_write(0x3d4020e0,0x310000);
	reg32_write(0x3d4020e8,0x66004a);
	reg32_write(0x3d4020ec,0x16004a);
	reg32_write(0x3d402100,0xb070508);
	reg32_write(0x3d402104,0x3040b);
	reg32_write(0x3d402108,0x305090c);
	reg32_write(0x3d40210c,0x505000);
	reg32_write(0x3d402110,0x4040204);
	reg32_write(0x3d402114,0x2030303);
	reg32_write(0x3d402118,0x1010004);
	reg32_write(0x3d40211c,0x301);
	reg32_write(0x3d402130,0x20300);
	reg32_write(0x3d402134,0xa100002);
	reg32_write(0x3d402138,0x31);
	reg32_write(0x3d402144,0x220011);
	reg32_write(0x3d402180,0xa70006);
	reg32_write(0x3d402190,0x3858202);
	reg32_write(0x3d402194,0x80303);
	reg32_write(0x3d4021b4,0x502);
	reg32_write(0x3d400244,0x0);
	reg32_write(0x3d400250,0x29001505);
	reg32_write(0x3d400254,0x2c);
	reg32_write(0x3d40025c,0x5900575b);
	reg32_write(0x3d400264,0x9);
	reg32_write(0x3d40026c,0x2005574);
	reg32_write(0x3d400300,0x16);
	reg32_write(0x3d400304,0x0);
	reg32_write(0x3d40030c,0x0);
	reg32_write(0x3d400320,0x1);
	reg32_write(0x3d40036c,0x11);
	reg32_write(0x3d400400,0x111);
	reg32_write(0x3d400404,0x10f3);
	reg32_write(0x3d400408,0x72ff);
	reg32_write(0x3d400490,0x1);
	reg32_write(0x3d400494,0x1110d00);
	reg32_write(0x3d400498,0x620790);
	reg32_write(0x3d40049c,0x100001);
	reg32_write(0x3d4004a0,0x41f);
	reg32_write(0x30391000,0x8f000004);
	reg32_write(0x30391000,0x8f000000);
	reg32_write(0x3d400030,0xa8);
	do{
		tmp=reg32_read(0x3d400004);
		if(tmp&0x223) break;
	}while(1);
	reg32_write(0x3d400320,0x0);
	reg32_write(0x3d000000,0x1);
	reg32_write(0x3d4001b0,0x10);
	reg32_write(0x3c040280,0x0);
	reg32_write(0x3c040284,0x1);
	reg32_write(0x3c040288,0x2);
	reg32_write(0x3c04028c,0x3);
	reg32_write(0x3c040290,0x4);
	reg32_write(0x3c040294,0x5);
	reg32_write(0x3c040298,0x6);
	reg32_write(0x3c04029c,0x7);
	reg32_write(0x3c044280,0x0);
	reg32_write(0x3c044284,0x1);
	reg32_write(0x3c044288,0x2);
	reg32_write(0x3c04428c,0x3);
	reg32_write(0x3c044290,0x4);
	reg32_write(0x3c044294,0x5);
	reg32_write(0x3c044298,0x6);
	reg32_write(0x3c04429c,0x7);
	reg32_write(0x3c048280,0x0);
	reg32_write(0x3c048284,0x1);
	reg32_write(0x3c048288,0x2);
	reg32_write(0x3c04828c,0x3);
	reg32_write(0x3c048290,0x4);
	reg32_write(0x3c048294,0x5);
	reg32_write(0x3c048298,0x6);
	reg32_write(0x3c04829c,0x7);
	reg32_write(0x3c04c280,0x0);
	reg32_write(0x3c04c284,0x1);
	reg32_write(0x3c04c288,0x2);
	reg32_write(0x3c04c28c,0x3);
	reg32_write(0x3c04c290,0x4);
	reg32_write(0x3c04c294,0x5);
	reg32_write(0x3c04c298,0x6);
	reg32_write(0x3c04c29c,0x7);

	/* Configure DDR PHY's registers */
	ddr_cfg_phy();

	reg32_write(DDRC_RFSHCTL3(0), 0x00000000);
	reg32_write(DDRC_SWCTL(0), 0x0000);
	/*
	 * ------------------- 9 -------------------
	 * Set DFIMISC.dfi_init_start to 1 
	 *  -----------------------------------------
	 */
	reg32_write(DDRC_DFIMISC(0), 0x00000030);
	reg32_write(DDRC_SWCTL(0), 0x0001);

	/* wait DFISTAT.dfi_init_complete to 1 */
	tmp_t = 0;
	while(tmp_t==0){
		tmp  = reg32_read(DDRC_DFISTAT(0));
		tmp_t = tmp & 0x01;
		tmp  = reg32_read(DDRC_MRSTAT(0));
	}

	reg32_write(DDRC_SWCTL(0), 0x0000);

	/* clear DFIMISC.dfi_init_complete_en */
	reg32_write(DDRC_DFIMISC(0), 0x00000010);
	reg32_write(DDRC_DFIMISC(0), 0x00000011);
	reg32_write(DDRC_PWRCTL(0), 0x00000088);

	tmp = reg32_read(DDRC_CRCPARSTAT(0));
	/*
	 * set SWCTL.sw_done to enable quasi-dynamic register
	 * programming outside reset.
	 */
	reg32_write(DDRC_SWCTL(0), 0x00000001);

	/* wait SWSTAT.sw_done_ack to 1 */
	while((reg32_read(DDRC_SWSTAT(0)) & 0x1) == 0)
		;

	/* wait STAT.operating_mode([1:0] for ddr3) to normal state */
	while ((reg32_read(DDRC_STAT(0)) & 0x3) != 0x1)
		;

	reg32_write(DDRC_PWRCTL(0), 0x00000088);
	/* reg32_write(DDRC_PWRCTL(0), 0x018a); */
	tmp = reg32_read(DDRC_CRCPARSTAT(0));

	/* enable port 0 */
	reg32_write(DDRC_PCTRL_0(0), 0x00000001);
	/* enable DDR auto-refresh mode */
	tmp = reg32_read(DDRC_RFSHCTL3(0)) & ~0x1;
	reg32_write(DDRC_RFSHCTL3(0), tmp);
}