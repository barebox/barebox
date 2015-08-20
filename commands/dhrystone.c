/*
 * (C) Copyright 2014 - 2015 Phytec Messtechnik GmbH
 * Author: Stefan Müller-Klieser <s.mueller-klieser@phytec.de>
 * Author: Daniel Schultz <d.schultz@phytec.de>
 *
 * based on "DHRYSTONE" Benchmark Program
 * Version:    C, Version 2.1
 * Date:       May 25, 1988
 * Author:     Reinhold P. Weicker
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
#include <command.h>
#include <errno.h>
#include <clock.h>
#include <asm-generic/div64.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h> /* for strcpy, strcmp */

enum idents {ident_1, ident_2, ident_3, ident_4, ident_5};

/* General definitions: */

struct record {
		struct record	*ptr_comp;
		enum idents	discr;
		union {
			struct {
				enum idents	enum_comp;
				int		int_comp;
				char		str_comp[31];
			} var_1;
			struct {
				enum idents	enum_comp_2;
				char		str_2_comp[31];
			} var_2;
			struct {
				char char_1_Comp;
				char char_2_Comp;
			} var_3;
		} variant;
};

/* Global Variables: */
static struct record	*record_glob;
static struct record	*next_record_glob;
static int		int_glob;
static bool		bool_glob;
static char		char_1_glob;
static char		char_2_glob;
static int		arr_1_glob[50];
static int		arr_2_glob[50][50];

/* Cycle Variables: */
static int		int_1;
static int		int_2;
static int		int_3;
static char		char_i;
static enum idents	ident;
static char		str_1[31];
static char		str_2[31];
static int		i;


#define TOO_SMALL_TIME (50 * MSECOND)
/* Measurements should last at least 50mseconds */
#define TOO_LARGE_TIME (2 * SECOND)


static enum idents compare_chars(char char_1, char char_2)
{
	if (char_1 != char_2)
		return ident_1;
	char_1_glob = char_1;
	return ident_2;
}

static bool compare_strs(char str_1[31], char str_2[31])
{
	int offset;

	offset = 2;
	while (offset <= 2)
		if (compare_chars(str_1[offset], str_2[offset+1]) == ident_1)
			++offset;
	if (strcmp(str_1, str_2) > 0) {
		int_glob = offset + 7;
		return true;
	}
	return false;
}

static bool check_ident(enum idents ident)
{
	if (ident == ident_3)
		return true;
	return false;
}

static void proc_7(int input_1, int input_2, int *out)
{
	*out = input_2 + input_1 + 2;
}

static void proc_6(enum idents ident, enum idents *ident_out)
{
	*ident_out = ident;
	if (!check_ident(ident))
		*ident_out = ident_4;
	switch (ident) {
	case ident_1:
		*ident_out = ident_1;
		break;
	case ident_2:
		if (int_glob > 100)
			*ident_out = ident_1;
		else
			*ident_out = ident_4;
		break;
	case ident_3:
		*ident_out = ident_2;
		break;
	case ident_4:
		break;
	case ident_5:
		*ident_out = ident_3;
		break;
	}
}

static void fill_record(struct record *record)
{
	struct record *next_record = record->ptr_comp;

	*record->ptr_comp = *record_glob;
	record->variant.var_1.int_comp = 5;
	next_record->variant.var_1.int_comp = record->variant.var_1.int_comp;
	next_record->ptr_comp = record->ptr_comp;
	proc_7(10, int_glob, &record_glob->variant.var_1.int_comp);
	/* Ptr_Val_Par->ptr_comp->ptr_comp == record_glob->ptr_comp */
	if (next_record->discr == ident_1) { /* then, executed */
		next_record->variant.var_1.int_comp = 6;
		proc_6(record->variant.var_1.enum_comp,
			&next_record->variant.var_1.enum_comp);
		next_record->ptr_comp = record_glob->ptr_comp;
		proc_7(next_record->variant.var_1.int_comp, 10,
			&next_record->variant.var_1.int_comp);
	} else { /* not executed */
		*record = *record->ptr_comp;
	}
}

static void proc_2(int *out)
{
	if (char_1_glob == 'A')
		*out = *out + 9 - int_glob;
}

static void proc_4(void)
{
	bool_glob = (char_1_glob == 'A') | bool_glob;
	char_2_glob = 'B';
}

static void proc_5(void)
{
	char_1_glob = 'A';
	bool_glob = false;
}

/* dhry_2.c */

static void fill_array(int arr_1[50], int arr_2[50][50], int val_1, int val_2)
{
	int i;
	int offset;

	offset = val_1 + 5;
	arr_1[offset] = val_2;
	arr_1[offset+1] = arr_1[offset];
	arr_1[offset+30] = offset;
	for (i = offset; i <= offset+1; ++i)
		arr_2[offset][i] = offset;
	arr_2[offset][offset-1] += 1;
	arr_2[offset+20][offset] = arr_1[offset];
	int_glob = 5;
}

static void one_dhrystone_cycle(void)
{
	proc_5();
	proc_4();
	/* char_1_glob == 'A', char_2_glob == 'B', bool_glob == true */
	int_1 = 2;
	int_2 = 3;
	strcpy(str_2, "DHRYSTONE PROGRAM, 2'ND STRING");
	ident = ident_2;
	bool_glob = !compare_strs(str_1, str_2);
	/* bool_glob == 1 */
	while (int_1 < int_2) {
		int_3 = 5 * int_1 - int_2;
		/* int_3 == 7 */
		proc_7(int_1, int_2, &int_3);
		/* int_3 == 7 */
		int_1 += 1;
	}
	/* int_1 == 3, int_2 == 3, int_3 == 7 */
	fill_array(arr_1_glob, arr_2_glob, int_1, int_3);
	/* int_glob == 5 */
	fill_record(record_glob);
	for (char_i = 'A'; char_i <= char_2_glob; ++char_i) {
		if (ident == compare_chars(char_i, 'C')) {
			proc_6(ident_1, &ident);
			strcpy(str_2,
				"DHRYSTONE PROGRAM, 3'RD STRING");
			int_2 = i;
			int_glob = i;
		}
	}
	/* int_1 == 3, int_2 == 3, int_3 == 7 */
	int_2 = int_2 * int_1;
	int_1 = int_2 / int_3;
	int_2 = 7 * (int_2 - int_3) - int_1;
	/* int_1 == 1, int_2 == 13, int_3 == 7 */
	proc_2(&int_1);
	/* int_1 == 5 */
}

static int do_dhrystone(int argc, char *argv[])
{
	int	number_of_runs;
	int	actually_number_of_runs = 0;
	/* variables for time measurement                 */
	u64	begin_time;
	u64	end_time;
	u64	user_time;
	u64	nanoseconds;

	/* barebox cmd */
	if (argc == 2) {
		number_of_runs = simple_strtoul(argv[1], NULL, 10);
		if (number_of_runs == 0)
			number_of_runs = 1;
	} else {
		number_of_runs = 10000;
	}

	/* Initializations */
	next_record_glob = xmalloc(sizeof(*next_record_glob));
	record_glob = xmalloc(sizeof(*record_glob));

	printf("\n");
	printf("Dhrystone Benchmark, Version 2.1 (Language: C)\n");
	printf("\n");
	printf("Program compiled without 'register' attribute\n");
	printf("\n");

	ident = ident_2;	/* prevent compiler warning */
	int_2 = 0;		/* prevent compiler warning */
	int_3 = 0;		/* prevent compiler warning */

	record_glob->ptr_comp			= next_record_glob;
	record_glob->discr			= ident_1;
	record_glob->variant.var_1.enum_comp	= ident_3;
	record_glob->variant.var_1.int_comp	= 40;
	strcpy(record_glob->variant.var_1.str_comp,
		"DHRYSTONE PROGRAM, SOME STRING");
	strcpy(str_1, "DHRYSTONE PROGRAM, 1'ST STRING");

	arr_2_glob[8][7] = 10;
	/* Was missing in published program. Without this statement,    */
	/* arr_2_glob [8][7] would have an undefined value.             */
	/* Warning: With 16-Bit processors and number_of_runs > 32000,  */
	/* overflow may occur for this array element.                   */

	/***************/
	/* Start timer */
	/***************/
	begin_time = get_time_ns();

again:
	for (i = 0; i < number_of_runs; i++)
		one_dhrystone_cycle();

	actually_number_of_runs += number_of_runs;
	end_time = get_time_ns();
	user_time = end_time - begin_time;

	if (user_time < TOO_SMALL_TIME)
		goto again;

	printf("Execution finished with %d runs through Dhrystone\n",
		actually_number_of_runs);
	printf("\n");
	printf("Failed variables:\n");
	if (int_glob != 5) {
		printf("int_glob:            %d\n", int_glob);
		printf("        should be:   %d\n", 5);
	}
	if (bool_glob != 1) {
		printf("bool_glob:           %d\n", bool_glob);
		printf("        should be:   %d\n", 1);
	}
	if (char_1_glob != 'A') {
		printf("char_1_glob:         %c\n", char_1_glob);
		printf("        should be:   %c\n", 'A');
	}
	if (char_2_glob != 'B') {
		printf("char_2_glob:         %c\n", char_2_glob);
		printf("        should be:   %c\n", 'B');
	}
	if (arr_1_glob[8] != 7) {
		printf("arr_1_glob[8]:       %d\n", arr_1_glob[8]);
		printf("        should be:   %d\n", 7);
	}
	if (arr_2_glob[8][7] != (actually_number_of_runs + 10)) {
		printf("arr_2_glob[8][7]:    %d\n", arr_2_glob[8][7]);
		printf("        should be:   %d\n",
			actually_number_of_runs + 10);
	}

	if (record_glob->ptr_comp != next_record_glob) {
		printf("record_glob->ptr_comp:    0x%p\n",
			record_glob->ptr_comp);
		printf("        should be:        0x%p\n", next_record_glob);
	}
	if (record_glob->discr != 0) {
		printf("record_glob->discr:       %d\n", record_glob->discr);
		printf("        should be:        %d\n", 0);
	}
	if (record_glob->variant.var_1.enum_comp != 2) {
		printf("record_glob->enum_comp:   %d\n",
			record_glob->variant.var_1.enum_comp);
		printf("        should be:        %d\n", 2);
	}
	if (record_glob->variant.var_1.int_comp != 17) {
		printf("record_glob->int_comp:    %d\n",
			record_glob->variant.var_1.int_comp);
		printf("        should be:        %d\n", 17);
	}
	if (strcmp(record_glob->variant.var_1.str_comp,
		"DHRYSTONE PROGRAM, SOME STRING") != 0) {
		printf("record_glob->str_comp:    %s\n",
			record_glob->variant.var_1.str_comp);
		printf("        should be:        DHRYSTONE PROGRAM, SOME STRING\n");
	}

	if (next_record_glob->ptr_comp != next_record_glob) {
		printf("next_record_glob->ptr_comp:    0x%p\n",
			next_record_glob->ptr_comp);
		printf("        should be:             0x%p\n",
			next_record_glob);
	}
	if (next_record_glob->discr != 0) {
		printf(" next_record_glob->discr:      %d\n",
			next_record_glob->discr);
		printf("        should be:             %d\n", 0);
	}
	if (next_record_glob->variant.var_1.enum_comp != 1) {
		printf("next_record_glob->enum_comp:   %d\n",
			next_record_glob->variant.var_1.enum_comp);
		printf("        should be:             %d\n", 1);
	}
	if (next_record_glob->variant.var_1.int_comp != 18) {
		printf("next_record_glob->int_comp:    %d\n",
			next_record_glob->variant.var_1.int_comp);
		printf("        should be:             %d\n", 18);
	}
	if (strcmp(next_record_glob->variant.var_1.str_comp,
		"DHRYSTONE PROGRAM, SOME STRING") != 0) {
		printf("next_record_glob->str_comp:    %s\n",
			next_record_glob->variant.var_1.str_comp);
		printf("        should be:             DHRYSTONE PROGRAM, SOME STRING\n");
	}
	if (int_1 != 5) {
		printf("int_1:               %d\n", int_1);
		printf("        should be:   %d\n", 5);
	}
	if (int_2 != 13) {
		printf("int_2:               %d\n", int_2);
		printf("        should be:   %d\n", 13);
	}
	if (int_3 != 7) {
		printf("int_3:               %d\n", int_3);
		printf("        should be:   %d\n", 7);
	}
	if (ident != 1) {
		printf("ident:               %d\n", ident);
		printf("        should be:   %d\n", 1);
	}
	if (strcmp(str_1, "DHRYSTONE PROGRAM, 1'ST STRING") != 0) {
		printf("str_1:               %s\n", str_1);
		printf("        should be:   DHRYSTONE PROGRAM, 1'ST STRING\n");
	}
	if (strcmp(str_2, "DHRYSTONE PROGRAM, 2'ND STRING") != 0) {
		printf("str_2:               %s\n", str_2);
		printf("        should be:   DHRYSTONE PROGRAM, 2'ND STRING\n");
	}
	printf("\n");

	if (user_time > TOO_LARGE_TIME)
		printf("Measured time too large to obtain meaningful results or a timer wrap happend.\n");
	else if (end_time < begin_time)
		printf("Timer overflow occured.\n");

	printf("user_time: %llu ns\n", user_time);

	nanoseconds = user_time;
	do_div(nanoseconds, actually_number_of_runs);

	printf("Nanoseconds for one run through Dhrystone: %llu\n",
		nanoseconds);
	printf("Dhrystones per Second:\n ");
	printf("(%d / %llu) * 10^9\n", actually_number_of_runs, user_time);
	printf("DMIPS:\n ");
	printf("((%d / %llu) * 10^9) / 1757\n", actually_number_of_runs,
		user_time);
	printf("\n");

	return 0;
}

BAREBOX_CMD_HELP_START(dhrystone)
BAREBOX_CMD_HELP_TEXT("Run dhrystone benchmark to get an estimation of the CPU freq")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(dhrystone)
	.cmd		= do_dhrystone,
	BAREBOX_CMD_DESC("run dhrystone test, specify number of runs")
	BAREBOX_CMD_OPTS("[number_of_runs]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_dhrystone_help)
BAREBOX_CMD_END
