/*
 * getopt.h - a simple getopt(3) implementation.
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __GETOPT_H
#define __GETOPT_H

extern int opterr;
extern int optind;
extern int optopt;
extern char *optarg;

/*
 * Simple getopt(3) implementation.
 * This version of getopt does not take long options but should
 * otherwise behave like one expects.
 *
 * - It takes ':' in optstring for required arguments and '::'
 *   for optional arguments.
 * - arguments can be followed directly by optargs (like -loptarg)
 *   or in the next argv[] element (like -l optarg).
 * - arguments can be grouped together (like ls -alR)
 * - options can be mixed with nonoptions (like ls /bin -R)
 */

int getopt(int argc, char *argv[], char *optstring);

/*
 * We do not start a new process for each getopt() run, so we
 * need this function to reset the static variables.
 */
void getopt_reset(void);

#endif /* __GETOPT_H */
