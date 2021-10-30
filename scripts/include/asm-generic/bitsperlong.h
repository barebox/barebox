/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BITS_PER_LONG_H_
#define __BITS_PER_LONG_H_

#ifndef __WORDSIZE
#define __WORDSIZE (__SIZEOF_LONG__ * 8)
#endif

#define BITS_PER_LONG __WORDSIZE

#endif
