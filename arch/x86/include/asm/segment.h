/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix */

#ifndef _ASM_X86_SEGMENT_H
#define _ASM_X86_SEGMENT_H

#include <linux/compiler.h>

/**
 * @file
 * @brief To be able to mark functions running in real _and_ flat mode
 */

/**
 * Section for every program code needed to bring barebox from real mode
 * to flat mode
 */
#define __bootcode	__section(.boot.text)

/**
 * Section for every data needed to bring barebox from real mode
 * to flat mode
 */
#define __bootdata	__section(.boot.data)

#endif /* _ASM_X86_SEGMENT_H */
