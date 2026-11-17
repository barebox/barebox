/* SPDX-License-Identifier: GPL-2.0-only */

#ifdef PIGGY_MEMSZ
/*
 * Memory barebox proper occupies once loaded in place, which unlike the
 * uncompressed length includes the BSS. In contrast to the piggy data,
 * this is relocated with the rest of the PBL, so it may be read any time.
 */
        .section .rodata.piggy_memsz,"a"
        .balign 4
        .globl  piggy_memsz
piggy_memsz:
        .long   PIGGY_MEMSZ
#endif

        .section .piggydata,"a"
        .globl  input_data
input_data:
        .incbin "images/barebox.z"
        .globl  input_data_end
input_data_end:
