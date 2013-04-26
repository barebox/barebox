/*
 * Copyright 2010 Digi International Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef _CCXMX51_H_
#define _CCXMX51_H_

struct ccxmx51_hwid {
	u8		variant;
	u8		version;
	u32		sn;
	char		mloc;
};

struct ccxmx51_ident {
	const char	*id_string;
	const int	mem_sz;
	const char	industrial;
	const char	eth0;
	const char	eth1;
	const char	wless;
};

extern struct ccxmx51_ident *ccxmx51_id;

#endif	/* _CCXMX51_H_ */
