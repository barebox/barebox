/*
 *  Copyright (C) 2012 Altera Corporation <www.altera.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef	_SDRAM_H_
#define	_SDRAM_H_

/* Group: sdr.phygrp.sccgrp                                                */
#define SDR_PHYGRP_SCCGRP_ADDRESS 0x0
/* Group: sdr.phygrp.phymgrgrp                                             */
#define SDR_PHYGRP_PHYMGRGRP_ADDRESS 0x1000
/* Group: sdr.phygrp.rwmgrgrp                                              */
#define SDR_PHYGRP_RWMGRGRP_ADDRESS 0x2000
/* Group: sdr.phygrp.datamgrgrp                                            */
#define SDR_PHYGRP_DATAMGRGRP_ADDRESS 0x4000
/* Group: sdr.phygrp.regfilegrp                                            */
#define SDR_PHYGRP_REGFILEGRP_ADDRESS 0x4800
/* Group: sdr.ctrlgrp                                                      */
#define SDR_CTRLGRP_ADDRESS 0x5000
/* Register: sdr.ctrlgrp.ctrlcfg                                           */
#define SDR_CTRLGRP_CTRLCFG_ADDRESS 0x5000
/* Register: sdr.ctrlgrp.dramtiming1                                       */
#define SDR_CTRLGRP_DRAMTIMING1_ADDRESS 0x5004
/* Register: sdr.ctrlgrp.dramtiming2                                       */
#define SDR_CTRLGRP_DRAMTIMING2_ADDRESS 0x5008
/* Register: sdr.ctrlgrp.dramtiming3                                       */
#define SDR_CTRLGRP_DRAMTIMING3_ADDRESS 0x500c
/* Register: sdr.ctrlgrp.dramtiming4                                       */
#define SDR_CTRLGRP_DRAMTIMING4_ADDRESS 0x5010
/* Register: sdr.ctrlgrp.lowpwrtiming                                      */
#define SDR_CTRLGRP_LOWPWRTIMING_ADDRESS 0x5014
/* Register: sdr.ctrlgrp.dramodt                                           */
#define SDR_CTRLGRP_DRAMODT_ADDRESS 0x5018
/* Register: sdr.ctrlgrp.dramaddrw                                         */
#define SDR_CTRLGRP_DRAMADDRW_ADDRESS 0x502c
/* Register: sdr.ctrlgrp.dramifwidth                                       */
#define SDR_CTRLGRP_DRAMIFWIDTH_ADDRESS 0x5030
/* Register: sdr.ctrlgrp.dramdevwidth                                      */
#define SDR_CTRLGRP_DRAMDEVWIDTH_ADDRESS 0x5034
/* Register: sdr.ctrlgrp.dramsts                                           */
#define SDR_CTRLGRP_DRAMSTS_ADDRESS 0x5038
/* Register: sdr.ctrlgrp.dramintr                                          */
#define SDR_CTRLGRP_DRAMINTR_ADDRESS 0x503c
/* Register: sdr.ctrlgrp.sbecount                                          */
#define SDR_CTRLGRP_SBECOUNT_ADDRESS 0x5040
/* Register: sdr.ctrlgrp.dbecount                                          */
#define SDR_CTRLGRP_DBECOUNT_ADDRESS 0x5044
/* Register: sdr.ctrlgrp.erraddr                                           */
#define SDR_CTRLGRP_ERRADDR_ADDRESS 0x5048
/* Register: sdr.ctrlgrp.dropcount                                         */
#define SDR_CTRLGRP_DROPCOUNT_ADDRESS 0x504c
/* Register: sdr.ctrlgrp.dropaddr                                          */
#define SDR_CTRLGRP_DROPADDR_ADDRESS 0x5050
/* Register: sdr.ctrlgrp.staticcfg                                         */
#define SDR_CTRLGRP_STATICCFG_ADDRESS 0x505c
/* Register: sdr.ctrlgrp.ctrlwidth                                         */
#define SDR_CTRLGRP_CTRLWIDTH_ADDRESS 0x5060
/* Register: sdr.ctrlgrp.cportwidth                                        */
#define SDR_CTRLGRP_CPORTWIDTH_ADDRESS 0x5064
/* Register: sdr.ctrlgrp.cportwmap                                         */
#define SDR_CTRLGRP_CPORTWMAP_ADDRESS 0x5068
/* Register: sdr.ctrlgrp.cportrmap                                         */
#define SDR_CTRLGRP_CPORTRMAP_ADDRESS 0x506c
/* Register: sdr.ctrlgrp.rfifocmap                                         */
#define SDR_CTRLGRP_RFIFOCMAP_ADDRESS 0x5070
/* Register: sdr.ctrlgrp.wfifocmap                                         */
#define SDR_CTRLGRP_WFIFOCMAP_ADDRESS 0x5074
/* Register: sdr.ctrlgrp.cportrdwr                                         */
#define SDR_CTRLGRP_CPORTRDWR_ADDRESS 0x5078
/* Register: sdr.ctrlgrp.portcfg                                           */
#define SDR_CTRLGRP_PORTCFG_ADDRESS 0x507c
/* Register: sdr.ctrlgrp.fpgaportrst                                       */
#define SDR_CTRLGRP_FPGAPORTRST_ADDRESS 0x5080
/* Register: sdr.ctrlgrp.fifocfg                                           */
#define SDR_CTRLGRP_FIFOCFG_ADDRESS 0x5088
/* Register: sdr.ctrlgrp.mppriority                                        */
#define SDR_CTRLGRP_MPPRIORITY_ADDRESS 0x50ac
/* Wide Register: sdr.ctrlgrp.mpweight                                     */
#define SDR_CTRLGRP_MPWEIGHT_ADDRESS 0x50b0
/* Register: sdr.ctrlgrp.mpweight.mpweight_0                               */
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_0_ADDRESS 0x50b0
/* Register: sdr.ctrlgrp.mpweight.mpweight_1                               */
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_1_ADDRESS 0x50b4
/* Register: sdr.ctrlgrp.mpweight.mpweight_2                               */
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_2_ADDRESS 0x50b8
/* Register: sdr.ctrlgrp.mpweight.mpweight_3                               */
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_3_ADDRESS 0x50bc
/* Register: sdr.ctrlgrp.mppacing.mppacing_0                               */
#define SDR_CTRLGRP_MPPACING_MPPACING_0_ADDRESS 0x50c0
/* Register: sdr.ctrlgrp.mppacing.mppacing_1                               */
#define SDR_CTRLGRP_MPPACING_MPPACING_1_ADDRESS 0x50c4
/* Register: sdr.ctrlgrp.mppacing.mppacing_2                               */
#define SDR_CTRLGRP_MPPACING_MPPACING_2_ADDRESS 0x50c8
/* Register: sdr.ctrlgrp.mppacing.mppacing_3                               */
#define SDR_CTRLGRP_MPPACING_MPPACING_3_ADDRESS 0x50cc
/* Register: sdr.ctrlgrp.mpthresholdrst.mpthresholdrst_0                   */
#define SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_0_ADDRESS 0x50d0
/* Register: sdr.ctrlgrp.mpthresholdrst.mpthresholdrst_1                   */
#define SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_1_ADDRESS 0x50d4
/* Register: sdr.ctrlgrp.mpthresholdrst.mpthresholdrst_2                   */
#define SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_2_ADDRESS 0x50d8
/* Wide Register: sdr.ctrlgrp.phyctrl                                      */
#define SDR_CTRLGRP_PHYCTRL_ADDRESS 0x5150
/* Register: sdr.ctrlgrp.phyctrl.phyctrl_0                                 */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ADDRESS 0x5150
/* Register: sdr.ctrlgrp.phyctrl.phyctrl_1                                 */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_ADDRESS 0x5154
/* Register: sdr.ctrlgrp.phyctrl.phyctrl_2                                 */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_ADDRESS 0x5158
/* Register instance: sdr::ctrlgrp::phyctrl.phyctrl_0                      */
/* Register template referenced: sdr::ctrlgrp::phyctrl::phyctrl_0          */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_OFFSET 0x150
/* Register instance: sdr::ctrlgrp::phyctrl.phyctrl_1                      */
/* Register template referenced: sdr::ctrlgrp::phyctrl::phyctrl_1          */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_OFFSET 0x154
/* Register instance: sdr::ctrlgrp::phyctrl.phyctrl_2                      */
/* Register template referenced: sdr::ctrlgrp::phyctrl::phyctrl_2          */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_OFFSET 0x158

/* Register template: sdr::ctrlgrp::ctrlcfg                                */
#define SDR_CTRLGRP_CTRLCFG_OUTPUTREG_LSB 26
#define SDR_CTRLGRP_CTRLCFG_OUTPUTREG_MASK 0x04000000
#define SDR_CTRLGRP_CTRLCFG_BURSTTERMEN_LSB 25
#define SDR_CTRLGRP_CTRLCFG_BURSTTERMEN_MASK 0x02000000
#define SDR_CTRLGRP_CTRLCFG_BURSTINTREN_LSB 24
#define SDR_CTRLGRP_CTRLCFG_BURSTINTREN_MASK 0x01000000
#define SDR_CTRLGRP_CTRLCFG_NODMPINS_LSB 23
#define SDR_CTRLGRP_CTRLCFG_NODMPINS_MASK 0x00800000
#define SDR_CTRLGRP_CTRLCFG_DQSTRKEN_LSB 22
#define SDR_CTRLGRP_CTRLCFG_DQSTRKEN_MASK 0x00400000
#define SDR_CTRLGRP_CTRLCFG_STARVELIMIT_LSB 16
#define SDR_CTRLGRP_CTRLCFG_STARVELIMIT_MASK 0x003f0000
#define SDR_CTRLGRP_CTRLCFG_REORDEREN_LSB 15
#define SDR_CTRLGRP_CTRLCFG_REORDEREN_MASK 0x00008000
#define SDR_CTRLGRP_CTRLCFG_GENDBE_LSB 14
#define SDR_CTRLGRP_CTRLCFG_GENDBE_MASK 0x00004000
#define SDR_CTRLGRP_CTRLCFG_GENSBE_LSB 13
#define SDR_CTRLGRP_CTRLCFG_GENSBE_MASK 0x00002000
#define SDR_CTRLGRP_CTRLCFG_CFG_ENABLE_ECC_CODE_OVERWRITES_LSB 12
#define SDR_CTRLGRP_CTRLCFG_CFG_ENABLE_ECC_CODE_OVERWRITES_MASK 0x00001000
#define SDR_CTRLGRP_CTRLCFG_ECCCORREN_LSB 11
#define SDR_CTRLGRP_CTRLCFG_ECCCORREN_MASK 0x00000800
#define SDR_CTRLGRP_CTRLCFG_ECCEN_LSB 10
#define SDR_CTRLGRP_CTRLCFG_ECCEN_MASK 0x00000400
#define SDR_CTRLGRP_CTRLCFG_ADDRORDER_LSB 8
#define SDR_CTRLGRP_CTRLCFG_ADDRORDER_MASK 0x00000300
#define SDR_CTRLGRP_CTRLCFG_MEMBL_LSB 3
#define SDR_CTRLGRP_CTRLCFG_MEMBL_MASK 0x000000f8
#define SDR_CTRLGRP_CTRLCFG_MEMTYPE_LSB 0
#define SDR_CTRLGRP_CTRLCFG_MEMTYPE_MASK 0x00000007
/* Register template: sdr::ctrlgrp::dramtiming1                            */
#define SDR_CTRLGRP_DRAMTIMING1_TRFC_LSB 24
#define SDR_CTRLGRP_DRAMTIMING1_TRFC_MASK 0xff000000
#define SDR_CTRLGRP_DRAMTIMING1_TFAW_LSB 18
#define SDR_CTRLGRP_DRAMTIMING1_TFAW_MASK 0x00fc0000
#define SDR_CTRLGRP_DRAMTIMING1_TRRD_LSB 14
#define SDR_CTRLGRP_DRAMTIMING1_TRRD_MASK 0x0003c000
#define SDR_CTRLGRP_DRAMTIMING1_TCL_LSB 9
#define SDR_CTRLGRP_DRAMTIMING1_TCL_MASK 0x00003e00
#define SDR_CTRLGRP_DRAMTIMING1_TAL_LSB 4
#define SDR_CTRLGRP_DRAMTIMING1_TAL_MASK 0x000001f0
#define SDR_CTRLGRP_DRAMTIMING1_TCWL_LSB 0
#define SDR_CTRLGRP_DRAMTIMING1_TCWL_MASK 0x0000000f
/* Register template: sdr::ctrlgrp::dramtiming2                            */
#define SDR_CTRLGRP_DRAMTIMING2_TWTR_LSB 25
#define SDR_CTRLGRP_DRAMTIMING2_TWTR_MASK 0x1e000000
#define SDR_CTRLGRP_DRAMTIMING2_TWR_LSB 21
#define SDR_CTRLGRP_DRAMTIMING2_TWR_MASK 0x01e00000
#define SDR_CTRLGRP_DRAMTIMING2_TRP_LSB 17
#define SDR_CTRLGRP_DRAMTIMING2_TRP_MASK 0x001e0000
#define SDR_CTRLGRP_DRAMTIMING2_TRCD_LSB 13
#define SDR_CTRLGRP_DRAMTIMING2_TRCD_MASK 0x0001e000
#define SDR_CTRLGRP_DRAMTIMING2_TREFI_LSB 0
#define SDR_CTRLGRP_DRAMTIMING2_TREFI_MASK 0x00001fff
/* Register template: sdr::ctrlgrp::dramtiming3                            */
#define SDR_CTRLGRP_DRAMTIMING3_TCCD_LSB 19
#define SDR_CTRLGRP_DRAMTIMING3_TCCD_MASK 0x00780000
#define SDR_CTRLGRP_DRAMTIMING3_TMRD_LSB 15
#define SDR_CTRLGRP_DRAMTIMING3_TMRD_MASK 0x00078000
#define SDR_CTRLGRP_DRAMTIMING3_TRC_LSB 9
#define SDR_CTRLGRP_DRAMTIMING3_TRC_MASK 0x00007e00
#define SDR_CTRLGRP_DRAMTIMING3_TRAS_LSB 4
#define SDR_CTRLGRP_DRAMTIMING3_TRAS_MASK 0x000001f0
#define SDR_CTRLGRP_DRAMTIMING3_TRTP_LSB 0
#define SDR_CTRLGRP_DRAMTIMING3_TRTP_MASK 0x0000000f
/* Register template: sdr::ctrlgrp::dramtiming4                            */
#define SDR_CTRLGRP_DRAMTIMING4_MINPWRSAVECYCLES_LSB 20
#define SDR_CTRLGRP_DRAMTIMING4_MINPWRSAVECYCLES_MASK 0x00f00000
#define SDR_CTRLGRP_DRAMTIMING4_PWRDOWNEXIT_LSB 10
#define SDR_CTRLGRP_DRAMTIMING4_PWRDOWNEXIT_MASK 0x000ffc00
#define SDR_CTRLGRP_DRAMTIMING4_SELFRFSHEXIT_LSB 0
#define SDR_CTRLGRP_DRAMTIMING4_SELFRFSHEXIT_MASK 0x000003ff
/* Register template: sdr::ctrlgrp::lowpwrtiming                           */
#define SDR_CTRLGRP_LOWPWRTIMING_CLKDISABLECYCLES_LSB 16
#define SDR_CTRLGRP_LOWPWRTIMING_CLKDISABLECYCLES_MASK 0x000f0000
#define SDR_CTRLGRP_LOWPWRTIMING_AUTOPDCYCLES_LSB 0
#define SDR_CTRLGRP_LOWPWRTIMING_AUTOPDCYCLES_MASK 0x0000ffff
/* Register template: sdr::ctrlgrp::dramaddrw                              */
#define SDR_CTRLGRP_DRAMADDRW_CSBITS_LSB 13
#define SDR_CTRLGRP_DRAMADDRW_CSBITS_MASK 0x0000e000
#define SDR_CTRLGRP_DRAMADDRW_BANKBITS_LSB 10
#define SDR_CTRLGRP_DRAMADDRW_BANKBITS_MASK 0x00001c00
#define SDR_CTRLGRP_DRAMADDRW_ROWBITS_LSB 5
#define SDR_CTRLGRP_DRAMADDRW_ROWBITS_MASK 0x000003e0
#define SDR_CTRLGRP_DRAMADDRW_COLBITS_LSB 0
#define SDR_CTRLGRP_DRAMADDRW_COLBITS_MASK 0x0000001f
/* Register template: sdr::ctrlgrp::dramifwidth                            */
#define SDR_CTRLGRP_DRAMIFWIDTH_IFWIDTH_LSB 0
#define SDR_CTRLGRP_DRAMIFWIDTH_IFWIDTH_MASK 0x000000ff
/* Register template: sdr::ctrlgrp::dramdevwidth                           */
#define SDR_CTRLGRP_DRAMDEVWIDTH_DEVWIDTH_LSB 0
#define SDR_CTRLGRP_DRAMDEVWIDTH_DEVWIDTH_MASK 0x0000000f
/* Register template: sdr::ctrlgrp::dramintr                               */
#define SDR_CTRLGRP_DRAMINTR_INTRCLR_LSB 4
#define SDR_CTRLGRP_DRAMINTR_INTRCLR_MASK 0x00000010
#define SDR_CTRLGRP_DRAMINTR_CORRDROPMASK_LSB 3
#define SDR_CTRLGRP_DRAMINTR_CORRDROPMASK_MASK 0x00000008
#define SDR_CTRLGRP_DRAMINTR_DBEMASK_LSB 2
#define SDR_CTRLGRP_DRAMINTR_DBEMASK_MASK 0x00000004
#define SDR_CTRLGRP_DRAMINTR_SBEMASK_LSB 1
#define SDR_CTRLGRP_DRAMINTR_SBEMASK_MASK 0x00000002
#define SDR_CTRLGRP_DRAMINTR_INTREN_LSB 0
#define SDR_CTRLGRP_DRAMINTR_INTREN_MASK 0x00000001
/* Register template: sdr::ctrlgrp::sbecount                               */
#define SDR_CTRLGRP_SBECOUNT_COUNT_LSB 0
#define SDR_CTRLGRP_SBECOUNT_COUNT_MASK 0x000000ff
/* Register template: sdr::ctrlgrp::dbecount                               */
#define SDR_CTRLGRP_DBECOUNT_COUNT_LSB 0
#define SDR_CTRLGRP_DBECOUNT_COUNT_MASK 0x000000ff
/* Register template: sdr::ctrlgrp::staticcfg                              */
#define SDR_CTRLGRP_STATICCFG_APPLYCFG_LSB 3
#define SDR_CTRLGRP_STATICCFG_APPLYCFG_MASK 0x00000008
#define SDR_CTRLGRP_STATICCFG_USEECCASDATA_LSB 2
#define SDR_CTRLGRP_STATICCFG_USEECCASDATA_MASK 0x00000004
#define SDR_CTRLGRP_STATICCFG_MEMBL_LSB 0
#define SDR_CTRLGRP_STATICCFG_MEMBL_MASK 0x00000003
/* Register template: sdr::ctrlgrp::ctrlwidth                              */
#define SDR_CTRLGRP_CTRLWIDTH_CTRLWIDTH_LSB 0
#define SDR_CTRLGRP_CTRLWIDTH_CTRLWIDTH_MASK 0x00000003
/* Register template: sdr::ctrlgrp::cportwidth                             */
#define SDR_CTRLGRP_CPORTWIDTH_CMDPORTWIDTH_LSB 0
#define SDR_CTRLGRP_CPORTWIDTH_CMDPORTWIDTH_MASK 0x000fffff
/* Register template: sdr::ctrlgrp::cportwmap                              */
#define SDR_CTRLGRP_CPORTWMAP_CPORTWFIFOMAP_LSB 0
#define SDR_CTRLGRP_CPORTWMAP_CPORTWFIFOMAP_MASK 0x3fffffff
/* Register template: sdr::ctrlgrp::cportrmap                              */
#define SDR_CTRLGRP_CPORTRMAP_CPORTRFIFOMAP_LSB 0
#define SDR_CTRLGRP_CPORTRMAP_CPORTRFIFOMAP_MASK 0x3fffffff
/* Register template: sdr::ctrlgrp::rfifocmap                              */
#define SDR_CTRLGRP_RFIFOCMAP_RFIFOCPORTMAP_LSB 0
#define SDR_CTRLGRP_RFIFOCMAP_RFIFOCPORTMAP_MASK 0x00ffffff
/* Register template: sdr::ctrlgrp::wfifocmap                              */
#define SDR_CTRLGRP_WFIFOCMAP_WFIFOCPORTMAP_LSB 0
#define SDR_CTRLGRP_WFIFOCMAP_WFIFOCPORTMAP_MASK 0x00ffffff
/* Register template: sdr::ctrlgrp::cportrdwr                              */
#define SDR_CTRLGRP_CPORTRDWR_CPORTRDWR_LSB 0
#define SDR_CTRLGRP_CPORTRDWR_CPORTRDWR_MASK 0x000fffff
/* Register template: sdr::ctrlgrp::portcfg                                */
#define SDR_CTRLGRP_PORTCFG_AUTOPCHEN_LSB 10
#define SDR_CTRLGRP_PORTCFG_AUTOPCHEN_MASK 0x000ffc00
#define SDR_CTRLGRP_PORTCFG_PORTPROTOCOL_LSB 0
#define SDR_CTRLGRP_PORTCFG_PORTPROTOCOL_MASK 0x000003ff
/* Register template: sdr::ctrlgrp::fifocfg                                */
#define SDR_CTRLGRP_FIFOCFG_INCSYNC_LSB 10
#define SDR_CTRLGRP_FIFOCFG_INCSYNC_MASK 0x00000400
#define SDR_CTRLGRP_FIFOCFG_SYNCMODE_LSB 0
#define SDR_CTRLGRP_FIFOCFG_SYNCMODE_MASK 0x000003ff
/* Register template: sdr::ctrlgrp::mppriority                             */
#define SDR_CTRLGRP_MPPRIORITY_USERPRIORITY_LSB 0
#define SDR_CTRLGRP_MPPRIORITY_USERPRIORITY_MASK 0x3fffffff
/* Wide Register template: sdr::ctrlgrp::mpweight                          */
/* Register template: sdr::ctrlgrp::mpweight::mpweight_0                   */
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_0_STATICWEIGHT_31_0_LSB 0
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_0_STATICWEIGHT_31_0_MASK 0xffffffff
/* Register template: sdr::ctrlgrp::mpweight::mpweight_1                   */
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_1_SUMOFWEIGHTS_13_0_LSB 18
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_1_SUMOFWEIGHTS_13_0_MASK 0xfffc0000
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_1_STATICWEIGHT_49_32_LSB 0
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_1_STATICWEIGHT_49_32_MASK 0x0003ffff
/* Register template: sdr::ctrlgrp::mpweight::mpweight_2                   */
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_2_SUMOFWEIGHTS_45_14_LSB 0
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_2_SUMOFWEIGHTS_45_14_MASK 0xffffffff
/* Register template: sdr::ctrlgrp::mpweight::mpweight_3                   */
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_3_SUMOFWEIGHTS_63_46_LSB 0
#define SDR_CTRLGRP_MPWEIGHT_MPWEIGHT_3_SUMOFWEIGHTS_63_46_MASK 0x0003ffff
/* Wide Register template: sdr::ctrlgrp::mppacing                          */
/* Register template: sdr::ctrlgrp::mppacing::mppacing_0                   */
#define SDR_CTRLGRP_MPPACING_MPPACING_0_THRESHOLD1_31_0_LSB 0
#define SDR_CTRLGRP_MPPACING_MPPACING_0_THRESHOLD1_31_0_MASK 0xffffffff
/* Register template: sdr::ctrlgrp::mppacing::mppacing_1                   */
#define SDR_CTRLGRP_MPPACING_MPPACING_1_THRESHOLD2_3_0_LSB 28
#define SDR_CTRLGRP_MPPACING_MPPACING_1_THRESHOLD2_3_0_MASK 0xf0000000
#define SDR_CTRLGRP_MPPACING_MPPACING_1_THRESHOLD1_59_32_LSB 0
#define SDR_CTRLGRP_MPPACING_MPPACING_1_THRESHOLD1_59_32_MASK 0x0fffffff
/* Register template: sdr::ctrlgrp::mppacing::mppacing_2                   */
#define SDR_CTRLGRP_MPPACING_MPPACING_2_THRESHOLD2_35_4_LSB 0
#define SDR_CTRLGRP_MPPACING_MPPACING_2_THRESHOLD2_35_4_MASK 0xffffffff
/* Register template: sdr::ctrlgrp::mppacing::mppacing_3                   */
#define SDR_CTRLGRP_MPPACING_MPPACING_3_THRESHOLD2_59_36_LSB 0
#define SDR_CTRLGRP_MPPACING_MPPACING_3_THRESHOLD2_59_36_MASK 0x00ffffff
/* Wide Register template: sdr::ctrlgrp::mpthresholdrst                    */
/* Register template: sdr::ctrlgrp::mpthresholdrst::mpthresholdrst_0       */
#define \
SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_0_THRESHOLDRSTCYCLES_31_0_LSB 0
#define  \
SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_0_THRESHOLDRSTCYCLES_31_0_MASK \
0xffffffff
/* Register template: sdr::ctrlgrp::mpthresholdrst::mpthresholdrst_1       */
#define \
SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_1_THRESHOLDRSTCYCLES_63_32_LSB 0
#define \
SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_1_THRESHOLDRSTCYCLES_63_32_MASK \
0xffffffff
/* Register template: sdr::ctrlgrp::mpthresholdrst::mpthresholdrst_2       */
#define \
SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_2_THRESHOLDRSTCYCLES_79_64_LSB 0
#define \
SDR_CTRLGRP_MPTHRESHOLDRST_MPTHRESHOLDRST_2_THRESHOLDRSTCYCLES_79_64_MASK \
0x0000ffff
/* Register template: sdr::ctrlgrp::remappriority                          */
#define SDR_CTRLGRP_REMAPPRIORITY_PRIORITYREMAP_LSB 0
#define SDR_CTRLGRP_REMAPPRIORITY_PRIORITYREMAP_MASK 0x000000ff
/* Wide Register template: sdr::ctrlgrp::phyctrl                           */
/* Register template: sdr::ctrlgrp::phyctrl::phyctrl_0                     */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_LSB 12
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_WIDTH 20
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_MASK 0xfffff000
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_SAMPLECOUNT_19_0_SET(x) \
 (((x) << 12) & 0xfffff000)
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ADDLATSEL_LSB 10
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ADDLATSEL_MASK 0x00000c00
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ADDLATSEL_SET(x) \
 (((x) << 10) & 0x00000c00)
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_LPDDRDIS_LSB 9
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_LPDDRDIS_MASK 0x00000200
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_LPDDRDIS_SET(x) \
 (((x) << 9) & 0x00000200)
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_RESETDELAYEN_LSB 8
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_RESETDELAYEN_MASK 0x00000100
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_RESETDELAYEN_SET(x) \
 (((x) << 8) & 0x00000100)
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSLOGICDELAYEN_LSB 6
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSLOGICDELAYEN_MASK 0x000000c0
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSLOGICDELAYEN_SET(x) \
 (((x) << 6) & 0x000000c0)
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSDELAYEN_LSB 4
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSDELAYEN_MASK 0x00000030
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQSDELAYEN_SET(x) \
 (((x) << 4) & 0x00000030)
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQDELAYEN_LSB 2
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQDELAYEN_MASK 0x0000000c
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_DQDELAYEN_SET(x) \
 (((x) << 2) & 0x0000000c)
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ACDELAYEN_LSB 0
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ACDELAYEN_MASK 0x00000003
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_0_ACDELAYEN_SET(x) \
 (((x) << 0) & 0x00000003)
/* Register template: sdr::ctrlgrp::phyctrl::phyctrl_1                     */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_LSB 12
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_WIDTH 20
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_MASK 0xfffff000
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_LONGIDLESAMPLECOUNT_19_0_SET(x) \
 (((x) << 12) & 0xfffff000)
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_SAMPLECOUNT_31_20_LSB 0
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_SAMPLECOUNT_31_20_MASK 0x00000fff
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_1_SAMPLECOUNT_31_20_SET(x) \
 (((x) << 0) & 0x00000fff)
/* Register template: sdr::ctrlgrp::phyctrl::phyctrl_2                     */
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_LONGIDLESAMPLECOUNT_31_20_LSB 0
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_LONGIDLESAMPLECOUNT_31_20_MASK 0x00000fff
#define SDR_CTRLGRP_PHYCTRL_PHYCTRL_2_LONGIDLESAMPLECOUNT_31_20_SET(x) \
 (((x) << 0) & 0x00000fff)
/* Register template: sdr::ctrlgrp::dramodt                                */
#define SDR_CTRLGRP_DRAMODT_READ_LSB 4
#define SDR_CTRLGRP_DRAMODT_READ_MASK 0x000000f0
#define SDR_CTRLGRP_DRAMODT_WRITE_LSB 0
#define SDR_CTRLGRP_DRAMODT_WRITE_MASK 0x0000000f
/* Register template: sdr::ctrlgrp::fpgaportrst                            */
#define SDR_CTRLGRP_FPGAPORTRST_READ_PORT_0_LSB 0
#define SDR_CTRLGRP_FPGAPORTRST_WRITE_PORT_0_LSB 4
#define SDR_CTRLGRP_FPGAPORTRST_COMMAND_PORT_0_LSB 8
/* Field instance: sdr::ctrlgrp::dramsts                                   */
#define SDR_CTRLGRP_DRAMSTS_DBEERR_MASK 0x00000008
#define SDR_CTRLGRP_DRAMSTS_SBEERR_MASK 0x00000004

#endif /* _SDRAM_H_ */
