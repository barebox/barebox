/*
Copyright (c) 2012, Altera Corporation
All rights reserved.

SPDX-License-Identifier:    BSD-3-Clause

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Altera Corporation nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ALTERA CORPORATION BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define __RW_MGR_ac_mrs1 0x04
#define __RW_MGR_ac_mrs3 0x06
#define __RW_MGR_ac_write_bank_0_col_0_nodata_wl_1 0x1C
#define __RW_MGR_ac_act_1 0x11
#define __RW_MGR_ac_write_postdata 0x1A
#define __RW_MGR_ac_act_0 0x10
#define __RW_MGR_ac_des 0x0D
#define __RW_MGR_ac_init_reset_1_cke_0 0x01
#define __RW_MGR_ac_write_data 0x19
#define __RW_MGR_ac_init_reset_0_cke_0 0x00
#define __RW_MGR_ac_read_bank_0_1_norden 0x22
#define __RW_MGR_ac_pre_all 0x12
#define __RW_MGR_ac_mrs0_user 0x02
#define __RW_MGR_ac_mrs0_dll_reset 0x03
#define __RW_MGR_ac_read_bank_0_0 0x1D
#define __RW_MGR_ac_write_bank_0_col_1 0x16
#define __RW_MGR_ac_read_bank_0_1 0x1F
#define __RW_MGR_ac_write_bank_1_col_0 0x15
#define __RW_MGR_ac_write_bank_1_col_1 0x17
#define __RW_MGR_ac_write_bank_0_col_0 0x14
#define __RW_MGR_ac_read_bank_1_0 0x1E
#define __RW_MGR_ac_mrs1_mirr 0x0A
#define __RW_MGR_ac_read_bank_1_1 0x20
#define __RW_MGR_ac_des_odt_1 0x0E
#define __RW_MGR_ac_mrs0_dll_reset_mirr 0x09
#define __RW_MGR_ac_zqcl 0x07
#define __RW_MGR_ac_write_predata 0x18
#define __RW_MGR_ac_mrs0_user_mirr 0x08
#define __RW_MGR_ac_ref 0x13
#define __RW_MGR_ac_nop 0x0F
#define __RW_MGR_ac_rdimm 0x23
#define __RW_MGR_ac_mrs2_mirr 0x0B
#define __RW_MGR_ac_write_bank_0_col_0_nodata 0x1B
#define __RW_MGR_ac_read_en 0x21
#define __RW_MGR_ac_mrs3_mirr 0x0C
#define __RW_MGR_ac_mrs2 0x05
#define __RW_MGR_CONTENT_ac_mrs1 0x10090006
#define __RW_MGR_CONTENT_ac_mrs3 0x100B0000
#define __RW_MGR_CONTENT_ac_write_bank_0_col_0_nodata_wl_1 0x18980000
#define __RW_MGR_CONTENT_ac_act_1 0x106B0000
#define __RW_MGR_CONTENT_ac_write_postdata 0x38780000
#define __RW_MGR_CONTENT_ac_act_0 0x10680000
#define __RW_MGR_CONTENT_ac_des 0x30780000
#define __RW_MGR_CONTENT_ac_init_reset_1_cke_0 0x20780000
#define __RW_MGR_CONTENT_ac_write_data 0x3CF80000
#define __RW_MGR_CONTENT_ac_init_reset_0_cke_0 0x20700000
#define __RW_MGR_CONTENT_ac_read_bank_0_1_norden 0x10580008
#define __RW_MGR_CONTENT_ac_pre_all 0x10280400
#define __RW_MGR_CONTENT_ac_mrs0_user 0x10080471
#define __RW_MGR_CONTENT_ac_mrs0_dll_reset 0x10080570
#define __RW_MGR_CONTENT_ac_read_bank_0_0 0x13580000
#define __RW_MGR_CONTENT_ac_write_bank_0_col_1 0x1C980008
#define __RW_MGR_CONTENT_ac_read_bank_0_1 0x13580008
#define __RW_MGR_CONTENT_ac_write_bank_1_col_0 0x1C9B0000
#define __RW_MGR_CONTENT_ac_write_bank_1_col_1 0x1C9B0008
#define __RW_MGR_CONTENT_ac_write_bank_0_col_0 0x1C980000
#define __RW_MGR_CONTENT_ac_read_bank_1_0 0x135B0000
#define __RW_MGR_CONTENT_ac_mrs1_mirr 0x100A0006
#define __RW_MGR_CONTENT_ac_read_bank_1_1 0x135B0008
#define __RW_MGR_CONTENT_ac_des_odt_1 0x38780000
#define __RW_MGR_CONTENT_ac_mrs0_dll_reset_mirr 0x100804E8
#define __RW_MGR_CONTENT_ac_zqcl 0x10380400
#define __RW_MGR_CONTENT_ac_write_predata 0x38F80000
#define __RW_MGR_CONTENT_ac_mrs0_user_mirr 0x10080469
#define __RW_MGR_CONTENT_ac_ref 0x10480000
#define __RW_MGR_CONTENT_ac_nop 0x30780000
#define __RW_MGR_CONTENT_ac_rdimm 0x10780000
#define __RW_MGR_CONTENT_ac_mrs2_mirr 0x10090218
#define __RW_MGR_CONTENT_ac_write_bank_0_col_0_nodata 0x18180000
#define __RW_MGR_CONTENT_ac_read_en 0x33780000
#define __RW_MGR_CONTENT_ac_mrs3_mirr 0x100B0000
#define __RW_MGR_CONTENT_ac_mrs2 0x100A0218

/*
Copyright (c) 2012, Altera Corporation
All rights reserved.

SPDX-License-Identifier:    BSD-3-Clause

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Altera Corporation nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ALTERA CORPORATION BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define __RW_MGR_READ_B2B_WAIT2 0x6B
#define __RW_MGR_LFSR_WR_RD_BANK_0_WAIT 0x32
#define __RW_MGR_REFRESH_ALL 0x14
#define __RW_MGR_ZQCL 0x06
#define __RW_MGR_LFSR_WR_RD_BANK_0_NOP 0x23
#define __RW_MGR_LFSR_WR_RD_BANK_0_DQS 0x24
#define __RW_MGR_ACTIVATE_0_AND_1 0x0D
#define __RW_MGR_MRS2_MIRR 0x0A
#define __RW_MGR_INIT_RESET_0_CKE_0 0x6F
#define __RW_MGR_LFSR_WR_RD_DM_BANK_0_WAIT 0x46
#define __RW_MGR_ACTIVATE_1 0x0F
#define __RW_MGR_MRS2 0x04
#define __RW_MGR_LFSR_WR_RD_DM_BANK_0_WL_1 0x35
#define __RW_MGR_MRS1 0x03
#define __RW_MGR_IDLE_LOOP1 0x7B
#define __RW_MGR_GUARANTEED_WRITE_WAIT2 0x19
#define __RW_MGR_MRS3 0x05
#define __RW_MGR_IDLE_LOOP2 0x7A
#define __RW_MGR_GUARANTEED_WRITE_WAIT1 0x1F
#define __RW_MGR_LFSR_WR_RD_BANK_0_DATA 0x25
#define __RW_MGR_GUARANTEED_WRITE_WAIT3 0x1D
#define __RW_MGR_RDIMM_CMD 0x79
#define __RW_MGR_LFSR_WR_RD_DM_BANK_0_NOP 0x37
#define __RW_MGR_GUARANTEED_WRITE_WAIT0 0x1B
#define __RW_MGR_LFSR_WR_RD_DM_BANK_0_DATA 0x39
#define __RW_MGR_GUARANTEED_READ_CONT 0x54
#define __RW_MGR_REFRESH_DELAY 0x15
#define __RW_MGR_MRS3_MIRR 0x0B
#define __RW_MGR_IDLE 0x00
#define __RW_MGR_READ_B2B 0x59
#define __RW_MGR_LFSR_WR_RD_DM_BANK_0_DQS 0x38
#define __RW_MGR_GUARANTEED_WRITE 0x18
#define __RW_MGR_PRECHARGE_ALL 0x12
#define __RW_MGR_SGLE_READ 0x7D
#define __RW_MGR_MRS0_USER_MIRR 0x0C
#define __RW_MGR_RETURN 0x01
#define __RW_MGR_LFSR_WR_RD_DM_BANK_0 0x36
#define __RW_MGR_MRS0_USER 0x07
#define __RW_MGR_GUARANTEED_READ 0x4C
#define __RW_MGR_MRS0_DLL_RESET_MIRR 0x08
#define __RW_MGR_INIT_RESET_1_CKE_0 0x74
#define __RW_MGR_ACTIVATE_0_AND_1_WAIT2 0x10
#define __RW_MGR_LFSR_WR_RD_BANK_0_WL_1 0x21
#define __RW_MGR_MRS0_DLL_RESET 0x02
#define __RW_MGR_ACTIVATE_0_AND_1_WAIT1 0x0E
#define __RW_MGR_LFSR_WR_RD_BANK_0 0x22
#define __RW_MGR_CLEAR_DQS_ENABLE 0x49
#define __RW_MGR_MRS1_MIRR 0x09
#define __RW_MGR_READ_B2B_WAIT1 0x61
#define __RW_MGR_CONTENT_READ_B2B_WAIT2 0x00C680
#define __RW_MGR_CONTENT_LFSR_WR_RD_BANK_0_WAIT 0x00A680
#define __RW_MGR_CONTENT_REFRESH_ALL 0x000980
#define __RW_MGR_CONTENT_ZQCL 0x008380
#define __RW_MGR_CONTENT_LFSR_WR_RD_BANK_0_NOP 0x00E700
#define __RW_MGR_CONTENT_LFSR_WR_RD_BANK_0_DQS 0x000C00
#define __RW_MGR_CONTENT_ACTIVATE_0_AND_1 0x000800
#define __RW_MGR_CONTENT_MRS2_MIRR 0x008580
#define __RW_MGR_CONTENT_INIT_RESET_0_CKE_0 0x000000
#define __RW_MGR_CONTENT_LFSR_WR_RD_DM_BANK_0_WAIT 0x00A680
#define __RW_MGR_CONTENT_ACTIVATE_1 0x000880
#define __RW_MGR_CONTENT_MRS2 0x008280
#define __RW_MGR_CONTENT_LFSR_WR_RD_DM_BANK_0_WL_1 0x00CE00
#define __RW_MGR_CONTENT_MRS1 0x008200
#define __RW_MGR_CONTENT_IDLE_LOOP1 0x00A680
#define __RW_MGR_CONTENT_GUARANTEED_WRITE_WAIT2 0x00CCE8
#define __RW_MGR_CONTENT_MRS3 0x008300
#define __RW_MGR_CONTENT_IDLE_LOOP2 0x008680
#define __RW_MGR_CONTENT_GUARANTEED_WRITE_WAIT1 0x00AC88
#define __RW_MGR_CONTENT_LFSR_WR_RD_BANK_0_DATA 0x020CE0
#define __RW_MGR_CONTENT_GUARANTEED_WRITE_WAIT3 0x00EC88
#define __RW_MGR_CONTENT_RDIMM_CMD 0x009180
#define __RW_MGR_CONTENT_LFSR_WR_RD_DM_BANK_0_NOP 0x00E700
#define __RW_MGR_CONTENT_GUARANTEED_WRITE_WAIT0 0x008CE8
#define __RW_MGR_CONTENT_LFSR_WR_RD_DM_BANK_0_DATA 0x030CE0
#define __RW_MGR_CONTENT_GUARANTEED_READ_CONT 0x001168
#define __RW_MGR_CONTENT_REFRESH_DELAY 0x00A680
#define __RW_MGR_CONTENT_MRS3_MIRR 0x008600
#define __RW_MGR_CONTENT_IDLE 0x080000
#define __RW_MGR_CONTENT_READ_B2B 0x040E88
#define __RW_MGR_CONTENT_LFSR_WR_RD_DM_BANK_0_DQS 0x000C00
#define __RW_MGR_CONTENT_GUARANTEED_WRITE 0x000B68
#define __RW_MGR_CONTENT_PRECHARGE_ALL 0x000900
#define __RW_MGR_CONTENT_SGLE_READ 0x040F08
#define __RW_MGR_CONTENT_MRS0_USER_MIRR 0x008400
#define __RW_MGR_CONTENT_RETURN 0x080680
#define __RW_MGR_CONTENT_LFSR_WR_RD_DM_BANK_0 0x00CD80
#define __RW_MGR_CONTENT_MRS0_USER 0x008100
#define __RW_MGR_CONTENT_GUARANTEED_READ 0x001168
#define __RW_MGR_CONTENT_MRS0_DLL_RESET_MIRR 0x008480
#define __RW_MGR_CONTENT_INIT_RESET_1_CKE_0 0x000080
#define __RW_MGR_CONTENT_ACTIVATE_0_AND_1_WAIT2 0x00A680
#define __RW_MGR_CONTENT_LFSR_WR_RD_BANK_0_WL_1 0x00CE00
#define __RW_MGR_CONTENT_MRS0_DLL_RESET 0x008180
#define __RW_MGR_CONTENT_ACTIVATE_0_AND_1_WAIT1 0x008680
#define __RW_MGR_CONTENT_LFSR_WR_RD_BANK_0 0x00CD80
#define __RW_MGR_CONTENT_CLEAR_DQS_ENABLE 0x001158
#define __RW_MGR_CONTENT_MRS1_MIRR 0x008500
#define __RW_MGR_CONTENT_READ_B2B_WAIT1 0x00A680

