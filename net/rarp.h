/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2000 Wolfgang Denk <wd@denx.de>, DENX Software Engineering */

#ifndef __RARP_H__
#define __RARP_H__

#ifndef __NET_H__
#include <net.h>
#endif /* __NET_H__ */


/**********************************************************************/
/*
 *	Global functions and variables.
 */

extern int	RarpTry;

extern void RarpRequest (void);	/* Send a RARP request */

/**********************************************************************/

#endif /* __RARP_H__ */
