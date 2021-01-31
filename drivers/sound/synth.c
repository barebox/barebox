// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Samsung Electronics, R. Chandrasekar <rcsekar@samsung.com>
 * Copyright (C) 2021 Ahmad Fatoum
 */

#include <common.h>
#include <linux/fixp-arith.h>
#include <linux/math64.h>
#include <sound.h>

void __synth_sin(unsigned freq, s16 amplitude, s16 *stream,
		 unsigned sample_rate, unsigned nsamples)
{
    int64_t v = 0;
    int i = 0;

    for (i = 0; i < nsamples; i++) {
	    /* Assume RHS sign extension, true for GCC */
	    stream[i] = (fixp_sin32(div_s64(v * 360, sample_rate)) * (int64_t)amplitude) >> 31;
	    v += freq;
    }
}

void __synth_square(unsigned freq, s16 amplitude, s16 *stream,
		    unsigned sample_rate, unsigned nsamples)
{
	unsigned period = freq ? sample_rate / freq : 0;
	int half = period / 2;

	while (nsamples) {
		int i;

		for (i = 0; nsamples && i < half; i++) {
			nsamples--;
			*stream++ = amplitude;
		}
		for (i = 0; nsamples && i < period - half; i++) {
			nsamples--;
			*stream++ = -amplitude;
		}
	}
}
