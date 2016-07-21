/*
 * getopt.c - a simple getopt(3) implementation. See getopt.h for explanation.
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
 */

#include <common.h>
#include <module.h>
#include <getopt.h>

int opterr = 1;
int optind = 1;
int optopt;
char *optarg;

EXPORT_SYMBOL(optind);
EXPORT_SYMBOL(opterr);
EXPORT_SYMBOL(optopt);
EXPORT_SYMBOL(optarg);

static int optindex = 1; /* option index in the current argv[] element */
static int nonopts = 0;  /* number of nonopts found */

void getopt_context_store(struct getopt_context *gc)
{
	gc->optind = optind;
	gc->opterr = opterr;
	gc->optopt = optopt;
	gc->optarg = optarg;
	gc->nonopts = nonopts;
	gc->optindex = optindex;

	optind = opterr = optindex = 1;
	nonopts = 0;
}
EXPORT_SYMBOL(getopt_context_store);

void getopt_context_restore(struct getopt_context *gc)
{
	optind = gc->optind;
	opterr = gc->opterr;
	optopt = gc->optopt;
	optarg = gc->optarg;
	nonopts = gc->nonopts;
	optindex = gc->optindex;
}
EXPORT_SYMBOL(getopt_context_restore);

int getopt(int argc, char *argv[], const char *optstring)
{
	char curopt;   /* current option character */
	const char *curoptp; /* pointer to the current option in optstring */

	while(1) {
		debug("optindex: %d nonopts: %d optind: %d\n", optindex, nonopts, optind);

		if (optindex == 1 && argv[optind] && !strcmp(argv[optind], "--")) {
			optind++;
			return -1;
		}

		/* first put nonopts to the end */
		while (optind + nonopts < argc && *argv[optind] != '-') {
			int i;
			char *tmp;

			nonopts++;
			tmp = argv[optind];
			for (i = optind; i + 1 < argc; i++)
				argv[i] = argv[i + 1];
			argv[argc - 1] = tmp;
		}

		if (optind + nonopts >= argc)
			return -1;

		/* We have found an option */
		curopt = argv[optind][optindex];
		if (curopt)
			break;
		/* no more options in current argv[] element. Start
		 * over with looking for nonopts
		 */
		optind++;
		optindex = 1;
	}

	/* look up current option in optstring */
	curoptp = strchr(optstring, curopt);

	if (!curoptp) {
		if (opterr)
			printf("%s: invalid option -- %c\n", argv[0], curopt);
		optopt = curopt;
		optindex++;
		return '?';
	}

	if (*(curoptp + 1) != ':') {
		/* option with no argument. Just return it */
		optarg = NULL;
		optindex++;
		return curopt;
	}

	if (*(curoptp + 1) && *(curoptp + 2) == ':') {
		/* optional argument */
		if (argv[optind][optindex + 1]) {
			/* optional argument with directly following optarg */
			optarg = argv[optind] + optindex + 1;
			optindex = 1;
			optind++;
			return curopt;
		}
		if (optind + nonopts  + 1 == argc) {
			/* We are at the last argv[] element */
			optarg = NULL;
			optind++;
			return curopt;
		}
		if (*argv[optind + 1] != '-') {
			/* optional argument with optarg in next argv[] element
			 */
			optind++;
			optarg = argv[optind];
			optind++;
			optindex = 1;
			return curopt;
		}

		/* no optional argument found */
		optarg = NULL;
		optindex = 1;
		optind++;
		return curopt;
	}

	if (argv[optind][optindex + 1]) {
		/* required argument with directly following optarg */
		optarg = argv[optind] + optindex + 1;
		optind++;
		optindex = 1;
		return curopt;
	}

	optind++;
	optindex = 1;

	if (optind + nonopts >= argc || argv[optind][0] == '-') {
		if (opterr)
			printf("option requires an argument -- %c\n",
				curopt);
		optopt = curopt;
		return ':';
	}

	optarg = argv[optind];
	optind++;
	return curopt;
}
EXPORT_SYMBOL(getopt);
