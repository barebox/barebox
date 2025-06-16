/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __ASM_BAREBOX_SANDBOX_H__
#define __ASM_BAREBOX_SANDBOX_H__

#define ENTRY_FUNCTION(name, argc, argv) \
	int name(int argc, char *argv[]); \
	int name(int argc, char *argv[])

#endif
