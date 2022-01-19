/* SPDX-License-Identifier: GPL-2.0-only */

        .section .piggydata,"a"
        .globl  input_data
input_data:
        .incbin "images/barebox.z"
        .globl  input_data_end
input_data_end:
