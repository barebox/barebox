/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * linux/compr_mm.h
 *
 * Memory management for pre-boot and ramdisk uncompressors
 *
 * Authors: Alain Knaff <alain@knaff.lu>
 *
 */

#ifndef DECOMPR_MM_H
#define DECOMPR_MM_H

#ifdef STATIC

#include <pbl.h>

/* Code active when included from pre-boot environment: */

/*
 * Some architectures want to ensure there is no local data in their
 * pre-boot environment, so that data can arbitrarily relocated (via
 * GOT references).  This is achieved by defining STATIC_RW_DATA to
 * be null.
 */
#ifndef STATIC_RW_DATA
#define STATIC_RW_DATA static
#endif

#define large_malloc(a) pbl_malloc(a)
#define large_free(a) pbl_free(a)

#define MALLOC pbl_malloc
#define FREE pbl_free

#else

#define large_malloc(a) malloc(a)
#define large_free(a) free(a)

#endif /* STATIC */

#ifndef STATIC
#define STATIC
#endif

#define INIT

#endif /* DECOMPR_MM_H */
