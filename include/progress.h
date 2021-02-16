/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PROGRSS_H
#define __PROGRSS_H

#include <linux/types.h>

/* Initialize a progress bar. If max > 0 a one line progress
 * bar is printed where 'max' corresponds to 100%. If max == 0
 * a multi line progress bar is printed.
 */
void init_progression_bar(loff_t max);

/* update a progress bar to a new value. If now < 0 then a
 * spinner is printed.
 */
void show_progress(loff_t now);

#endif /*  __PROGRSS_H */
