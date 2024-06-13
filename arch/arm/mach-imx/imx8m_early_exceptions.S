/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/linkage.h>

/*
 * Early exception vectors.
 */
	.align	11
	.globl	early_imx8m_vectors
early_imx8m_vectors:
	.align	7
		eret

	.align	7
		eret

	.align	7
		eret

	.align	7
		eret

	.align	7
		eret

	.align	7
		eret

	.align	7
		eret

	.align	7
		eret
