/* SPDX-License-Identifier: GPL-2.0 */

        .section .note.GNU-stack,"",%progbits
        .section .shasum,"a"
        .globl  sha_sum
sha_sum:
        .incbin "images/barebox.sha.bin"
        .globl  sha_sum_end
sha_sum_end:
