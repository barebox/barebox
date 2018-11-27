/* vi: set sw=8 ts=8: */
/*
 * hush.c -- a prototype Bourne shell grammar parser
 *      Intended to follow the original Thompson and Ritchie
 *      "small and simple is beautiful" philosophy, which
 *      incidentally is a good match to today's BusyBox.
 *
 * Copyright (C) 2000,2001  Larry Doolittle  <larry@doolittle.boa.org>
 *
 * Credits:
 *      The parser routines proper are all original material, first
 *      written Dec 2000 and Jan 2001 by Larry Doolittle.
 *      The execution engine, the builtins, and much of the underlying
 *      support has been adapted from busybox-0.49pre's lash,
 *      which is Copyright (C) 2000 by Lineo, Inc., and
 *      written by Erik Andersen <andersen@lineo.com>, <andersee@debian.org>.
 *      That, in turn, is based in part on ladsh.c, by Michael K. Johnson and
 *      Erik W. Troan, which they placed in the public domain.  I don't know
 *      how much of the Johnson/Troan code has survived the repeated rewrites.
 * Other credits:
 *      simple_itoa() was lifted from boa-0.93.15
 *      b_addchr() derived from similar w_addchar function in glibc-2.2
 *      setup_redirect(), redirect_opt_num(), and big chunks of main()
 *        and many builtins derived from contributions by Erik Andersen
 *      miscellaneous bugfixes from Matt Kraai
 */

/** @page shell_notes Simple Shell Environment: Hush
 *
 * @par Notes from the source:
 *
 * Intended to follow the original Thompson and Ritchie "small and simple
 * is beautiful" philosophy, which incidentally is a good match to
 * today's BusyBox.
 *
 * There are two big (and related) architecture differences between
 * this parser and the lash parser.  One is that this version is
 * actually designed from the ground up to understand nearly all
 * of the Bourne grammar.  The second, consequential change is that
 * the parser and input reader have been turned inside out.  Now,
 * the parser is in control, and asks for input as needed.  The old
 * way had the input reader in control, and it asked for parsing to
 * take place as needed.  The new way makes it much easier to properly
 * handle the recursion implicit in the various substitutions, especially
 * across continuation lines.
 *
 * Bash grammar not implemented: (how many of these were in original sh?)
 * -     $@ (those sure look like weird quoting rules)
 * -     $_
 * -     ! negation operator for pipes
 * -     &> and >& redirection of stdout+stderr
 * -     Brace Expansion
 * -     Tilde Expansion
 * -     fancy forms of Parameter Expansion
 * -     aliases
 * -     Arithmetic Expansion
 * -     <(list) and >(list) Process Substitution
 * -     reserved words: case, esac, select, function
 * -     Here Documents ( << word )
 * -     Functions
 *
 * Major bugs:
 * -     job handling woefully incomplete and buggy
 * -     reserved word execution woefully incomplete and buggy
 *
 * to-do:
 * -     port selected bugfixes from post-0.49 busybox lash - done?
 * -     finish implementing reserved words: for, while, until, do, done
 * -     change { and } from special chars to reserved words
 * -     builtins: break, continue, eval, return, set, trap, ulimit
 * -     test magic exec
 * -     handle children going into background
 * -     clean up recognition of null pipes
 * -     check setting of global_argc and global_argv
 * -     control-C handling, probably with longjmp
 * -     follow IFS rules more precisely, including update semantics
 * -     figure out what to do with backslash-newline
 * -     explain why we use signal instead of sigaction
 * -     propagate syntax errors, die on resource errors?
 * -     continuation lines, both explicit and implicit - done?
 * -     memory leak finding and plugging - done?
 * -     more testing, especially quoting rules and redirection
 * -     document how quoting rules not precisely followed for variable assignments
 * -     maybe change map[] to use 2-bit entries
 * -     (eventually) remove all the printf's
 *
 * Things that do _not_ work in this environment:
 *
 * - redirecting (stdout to a file for example)
 * - recursion
 *
 * Enable the "Hush parser" in "General Settings", "Select your shell" to
 * get the new console feeling.
 *
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 */

#define pr_fmt(fmt) "hush: " fmt

#include <malloc.h>         /* malloc, free, realloc*/
#include <xfuncs.h>
#include <linux/ctype.h>    /* isalpha, isdigit */
#include <common.h>        /* readline */
#include <environment.h>
#include <command.h>        /* find_cmd */
#include <driver.h>
#include <errno.h>
#include <fs.h>
#include <libbb.h>
#include <password.h>
#include <glob.h>
#include <getopt.h>
#include <libfile.h>
#include <libbb.h>
#include <magicvar.h>
#include <linux/list.h>
#include <binfmt.h>
#include <init.h>
#include <shell.h>

/*cmd_boot.c*/
extern int do_bootd(int flag, int argc, char *argv[]);      /* do_bootd */
#define SPECIAL_VAR_SYMBOL 03


#define EXIT_SUCCESS 0
#define EOF -1
#define xstrdup strdup
#define error_msg printf

typedef enum {
	PIPE_SEQ = 1,
	PIPE_AND = 2,
	PIPE_OR  = 3,
	PIPE_BG  = 4,
} pipe_style;

/* might eventually control execution */
typedef enum {
	RES_NONE  = 0,
	RES_IF    = 1,
	RES_THEN  = 2,
	RES_ELIF  = 3,
	RES_ELSE  = 4,
	RES_FI    = 5,
	RES_FOR   = 6,
	RES_WHILE = 7,
	RES_UNTIL = 8,
	RES_DO    = 9,
	RES_DONE  = 10,
	RES_XXXX  = 11,
	RES_IN    = 12,
	RES_SNTX  = 13
} reserved_style;
#define FLAG_END   (1<<RES_NONE)
#define FLAG_IF    (1<<RES_IF)
#define FLAG_THEN  (1<<RES_THEN)
#define FLAG_ELIF  (1<<RES_ELIF)
#define FLAG_ELSE  (1<<RES_ELSE)
#define FLAG_FI    (1<<RES_FI)
#define FLAG_FOR   (1<<RES_FOR)
#define FLAG_WHILE (1<<RES_WHILE)
#define FLAG_UNTIL (1<<RES_UNTIL)
#define FLAG_DO    (1<<RES_DO)
#define FLAG_DONE  (1<<RES_DONE)
#define FLAG_IN    (1<<RES_IN)
#define FLAG_START (1<<RES_XXXX)

#define FLAG_EXIT_FROM_LOOP 1
#define FLAG_PARSE_SEMICOLON (1 << 1)	  /* symbol ';' is special for parser */
#define FLAG_REPARSING       (1 << 2)	  /* >=2nd pass */

struct option {
	struct list_head	list;
	char	opt;
	char	*optarg;
};

/* This holds pointers to the various results of parsing */
struct p_context {
	struct child_prog *child;
	struct pipe *list_head;
	struct pipe *pipe;
	reserved_style w;
	int old_flag;				/* for figuring out valid reserved words */
	struct p_context *stack;
	int type;			/* define type of parser : ";$" common or special symbol */
	/* How about quoting status? */

	char **global_argv;
	unsigned int global_argc;

	int options_parsed;
	struct list_head options;
};


struct child_prog {
	char **argv;				/* program name and arguments */
	int    argc;                            /* number of program arguments */
	struct pipe *group;			/* if non-NULL, first in group or subshell */
	int sp;				/* number of SPECIAL_VAR_SYMBOL */
	int type;
	glob_t glob_result;			/* result of parameter globbing */
};

struct pipe {
	int num_progs;				/* total number of programs in job */
	struct child_prog *progs;	/* array of commands in pipe */
	struct pipe *next;			/* to track background commands */
	pipe_style followup;		/* PIPE_BG, PIPE_SEQ, PIPE_OR, PIPE_AND */
	reserved_style r_mode;		/* supports if, for, while, until */
};


static char console_buffer[CONFIG_CBSIZE];		/* console I/O buffer	*/

/* globals, connect us to the outside world
 * the first three support $?, $#, and $1 */
static unsigned int last_return_code;

int shell_get_last_return_code(void)
{
	return last_return_code;
}

/* "globals" within this file */
static uchar *ifs;
static char map[256];

#define B_CHUNK (100)
#define B_NOSPAC 1

typedef struct {
	char *data;
	int length;
	int maxlen;
	int quote;
	int nonnull;
} o_string;
#define NULL_O_STRING {NULL,0,0,0,0}
/* used for initialization:
	o_string foo = NULL_O_STRING; */

/* I can almost use ordinary FILE *.  Is open_memstream() universally
 * available?  Where is it documented? */
struct in_str {
	const char *p;
	int interrupt;
	int promptmode;
	int (*get) (struct in_str *);
	int (*peek) (struct in_str *);
};
#define b_getch(input) ((input)->get(input))
#define b_peek(input) ((input)->peek(input))

#ifdef HUSH_DEBUG
#define hush_debug(fmt, arg...) debug(fmt, ##arg)
#else
#define hush_debug(fmt, arg...)
#endif

#define final_printf hush_debug

static void syntax(void)
{
	printf("syntax error\n");
}

static void syntaxf(const char *fmt, ...)
{
	va_list args;

	printf("syntax error: ");

	va_start(args, fmt);

	vprintf(fmt, args);

	va_end(args);
}

static void syntax_unexpected_token(const char *token)
{
	syntaxf("unexpected token `%s'\n", token);
}

/*   o_string manipulation: */
static int b_check_space(o_string *o, int len);
static int b_addchr(o_string *o, int ch);
static void b_reset(o_string *o);
/*  in_str manipulations: */
static int static_get(struct in_str *i);
static int static_peek(struct in_str *i);
static int file_get(struct in_str *i);
static int file_peek(struct in_str *i);
static void setup_file_in_str(struct in_str *i);
static void setup_string_in_str(struct in_str *i, const char *s);
/*  "run" the final data structures: */
static int free_pipe_list(struct pipe *head, int indent);
static int free_pipe(struct pipe *pi, int indent);
/*  really run the final data structures: */
static int run_list_real(struct p_context *ctx, struct pipe *pi);
static int run_pipe_real(struct p_context *ctx, struct pipe *pi);
/*   extended glob support: */
/*   variable assignment: */
static int is_assignment(const char *s);
/*   data structure manipulation: */
static void initialize_context(struct p_context *ctx);
static void release_context(struct p_context *ctx);
static int done_word(o_string *dest, struct p_context *ctx);
static int done_command(struct p_context *ctx);
static int done_pipe(struct p_context *ctx, pipe_style type);
/*   primary string parsing: */
static const char *lookup_param(char *src);
static char *make_string(char **inp);
static int handle_dollar(o_string *dest, struct p_context *ctx, struct in_str *input);
static int parse_stream(o_string *dest, struct p_context *ctx, struct in_str *input0, int end_trigger);
/*   setup: */
static int parse_stream_outer(struct p_context *ctx, struct in_str *inp, int flag);

static int parse_string_outer(struct p_context *ctx, const char *s, int flag);
/*     local variable support */
static char **make_list_in(char **inp, char *name);
static char *insert_var_value(char *inp);
static int set_local_var(const char *s, int flg_export);
static int execute_script(const char *path, int argc, char *argv[]);
static int source_script(const char *path, int argc, char *argv[]);

static int b_check_space(o_string *o, int len)
{
	/* It would be easy to drop a more restrictive policy
	 * in here, such as setting a maximum string length */
	if (o->length + len > o->maxlen) {
		char *old_data = o->data;
		/* assert (data == NULL || o->maxlen != 0); */
		o->maxlen += max(2*len, B_CHUNK);
		o->data = realloc(o->data, 1 + o->maxlen);
		if (o->data == NULL) {
			free(old_data);
		}
	}
	return o->data == NULL;
}

static int b_addchr(o_string *o, int ch)
{
	hush_debug("%s: %c %d %p\n", __func__, ch, o->length, o);

	if (b_check_space(o, 1))
		return B_NOSPAC;

	o->data[o->length] = ch;
	o->length++;
	o->data[o->length] = '\0';

	return 0;
}

static int b_addstr(o_string *o, const char *str)
{
	int ret;

	while (*str) {
		ret = b_addchr(o, *str++);
		if (ret)
			return ret;
	}

	return 0;
}

static void b_reset(o_string *o)
{
	o->length = 0;
	o->nonnull = 0;

	if (o->data != NULL)
		*o->data = '\0';
}

static void b_free(o_string *o)
{
	b_reset(o);
	free(o->data);
	o->data = NULL;
	o->maxlen = 0;
}

static int b_adduint(o_string *o, unsigned int i)
{
	int r;
	char *p = simple_itoa(i);

	/* no escape checking necessary */
	do {
		r = b_addchr(o, *p++);
	} while (r == 0 && *p);

	return r;
}

static int static_get(struct in_str *i)
{
	int ch = *i->p++;

	if (ch == '\0')
		return EOF;

	return ch;
}


static int static_peek(struct in_str *i)
{
	return *i->p;
}

static char *getprompt(void)
{
	static char prompt[PATH_MAX + 32];

#ifdef CONFIG_HUSH_FANCY_PROMPT
	const char *ps1 = getenv("PS1");

	if (ps1)
		process_escape_sequence(ps1, prompt, PATH_MAX + 32);
	else
#endif
	sprintf(prompt, "%s%s ", CONFIG_PROMPT, getcwd());

	return prompt;
}

static void get_user_input(struct in_str *i)
{
	int n;
	static char the_command[CONFIG_CBSIZE];
	char *prompt;

	if (i->promptmode == 1)
		prompt = getprompt();
	else
		prompt = CONFIG_PROMPT_HUSH_PS2;

	n = readline(prompt, console_buffer, CONFIG_CBSIZE);
	if (n == -1 ) {
		i->interrupt = 1;
		n = 0;
	}

	console_buffer[n] = '\n';
	console_buffer[n + 1]= '\0';

	if (i->promptmode == 1) {
		strcpy(the_command,console_buffer);
		i->p = the_command;
		return;
	}

	if (console_buffer[0] != '\n') {
		if (strlen(the_command) + strlen(console_buffer)
		    < CONFIG_CBSIZE) {
			n = strlen(the_command);
			the_command[n - 1] = ' ';
			strcpy(&the_command[n], console_buffer);
		} else {
			the_command[0] = '\n';
			the_command[1] = '\0';
		}
	}

	if (i->interrupt) {
		the_command[0] = '\n';
		the_command[1] = '\0';
	}

	i->p = console_buffer;
}

/* This is the magic location that prints prompts
 * and gets data back from the user */
static int file_get(struct in_str *i)
{
	int ch;

	ch = 0;

	/* If there is data waiting, eat it up */
	if (i->p && *i->p)
		return *i->p++;

	/* need to double check i->file because we might be doing something
	 * more complicated by now, like sourcing or substituting. */
	while (!i->p  || strlen(i->p) == 0 )
		get_user_input(i);

	i->promptmode = 2;

	if (i->p && *i->p)
		ch = *i->p++;

	hush_debug("%s: got a %d\n", __func__, ch);

	return ch;
}

/* All the callers guarantee this routine will never be
 * used right after a newline, so prompting is not needed.
 */
static int file_peek(struct in_str *i)
{
		return *i->p;
}

static void setup_file_in_str(struct in_str *i)
{
	i->peek = file_peek;
	i->get = file_get;
	i->interrupt = 0;
	i->promptmode = 1;
	i->p = NULL;
}

static void setup_string_in_str(struct in_str *i, const char *s)
{
	i->peek = static_peek;
	i->get = static_get;
	i->interrupt = 0;
	i->promptmode = 1;
	i->p = s;
}

#ifdef CONFIG_CMD_GETOPT
static int builtin_getopt(struct p_context *ctx, struct child_prog *child,
		int argc, char *argv[])
{
	char *optstring, *var;
	int opt, ret = 0;
	char opta[2];
	struct option *o;
	struct getopt_context gc;

	if (argc != 3)
		return -2 - 1;

	optstring = argv[1];
	var = argv[2];

	getopt_context_store(&gc);

	if (!ctx->options_parsed) {
		while ((opt = getopt(ctx->global_argc, ctx->global_argv, optstring)) > 0) {
			o = xzalloc(sizeof(*o));
			o->opt = opt;
			o->optarg = xstrdup(optarg);
			list_add_tail(&o->list, &ctx->options);
		}
		ctx->global_argv += optind - 1;
		ctx->global_argc -= optind - 1;
	}

	ctx->options_parsed = 1;

	if (list_empty(&ctx->options)) {
		ret = -1;
		goto out;
	}

	o = list_first_entry(&ctx->options, struct option, list);

	opta[0] = o->opt;
	opta[1] = 0;
	setenv(var, opta);
	setenv("OPTARG", o->optarg);

	free(o->optarg);
	list_del(&o->list);
	free(o);
out:
	getopt_context_restore(&gc);

	return ret;
}

BAREBOX_MAGICVAR(OPTARG, "optarg for hush builtin getopt");
#else
static int builtin_getopt(struct p_context *ctx, struct child_prog *child,
		int argc, char *argv[])
{
	return -1;
}
#endif

static int builtin_exit(struct p_context *ctx, struct child_prog *child,
		int argc, char *argv[])
{
	int r;

	r = last_return_code;
	if (argc > 1)
		r = simple_strtoul(argv[1], NULL, 0);

	return -r - 2;
}

static void remove_quotes_in_str(char *src)
{
	char *trg = src;

	while (*src) {
		if (*src == '\'') {
			src++;
			while (*src != '\'')
				*trg++ = *src++;
			src++;
			continue;
		}

		/* drop quotes */
		if (*src == '"') {
			src++;
			continue;
		}

		/* replace \" with " */
		if (*src == '\\' && *(src + 1) == '"') {
			*trg++ = '"';
			src += 2;
			continue;
		}

		/* replace \' with ' */
		if (*src == '\\' && *(src + 1) == '\'') {
			*trg++ = '\'';
			src += 2;
			continue;
		}

		/* replace \\ with \ */
		if (*src == '\\' && *(src + 1) == '\\') {
			*trg++ = '\\';
			src += 2;
			continue;
		}

		*trg++ = *src++;
	}
	*trg = 0;
}

static void remove_quotes(int argc, char *argv[])
{
	int i;

	for (i = 0; i < argc; i++)
		remove_quotes_in_str(argv[i]);
}

static int fake_glob(const char *src, int flags,
		int (*errfunc) (const char *epath, int eerrno),
		glob_t *pglob)
{
	int pathc;

	if (!(flags & GLOB_APPEND)) {
		globfree(pglob);
		pglob->gl_pathv = NULL;
		pglob->gl_pathc = 0;
		pglob->gl_offs = 0;
	}
	pathc = ++pglob->gl_pathc;
	pglob->gl_pathv = xrealloc(pglob->gl_pathv, (pathc + 1) * sizeof(*pglob->gl_pathv));
	pglob->gl_pathv[pathc - 1] = xstrdup(src);
	pglob->gl_pathv[pathc] = NULL;

	return 0;
}

#ifdef CONFIG_GLOB
static int do_glob(const char *src, int flags,
		int (*errfunc) (const char *epath, int eerrno),
		glob_t *pglob)
{
	return glob(src, flags, errfunc, pglob);
}
#else
static int do_glob(const char *src, int flags,
		int (*errfunc) (const char *epath, int eerrno),
		glob_t *pglob)
{
	return fake_glob(src, flags, errfunc, pglob);
}
#endif

static void do_glob_in_argv(glob_t *globbuf, int argc, char **argv)
{
	int i;
	int flags;

	globbuf->gl_offs = 0;
	flags = GLOB_DOOFFS | GLOB_NOCHECK;

	for (i = 0; i < argc; i++) {
		int ret;
		ret = do_glob(argv[i], flags, NULL, globbuf);
		flags |= GLOB_APPEND;
	}
}

/* run_pipe_real() starts all the jobs, but doesn't wait for anything
 * to finish.  See checkjobs().
 *
 * return code is normally -1, when the caller has to wait for children
 * to finish to determine the exit status of the pipe.  If the pipe
 * is a simple builtin command, however, the action is done by the
 * time run_pipe_real returns, and the exit code is provided as the
 * return value.
 *
 * The input of the pipe is always stdin, the output is always
 * stdout.  The outpipe[] mechanism in BusyBox-0.48 lash is bogus,
 * because it tries to avoid running the command substitution in
 * subshell, when that is in fact necessary.  The subshell process
 * now has its stdout directed to the input of the appropriate pipe,
 * so this routine is noticeably simpler.
 */
static int run_pipe_real(struct p_context *ctx, struct pipe *pi)
{
	int i;
	int nextin;
	struct child_prog *child;
	char *p;
	glob_t globbuf = {};
	int ret;
	int rcode;
# if __GNUC__
	/* Avoid longjmp clobbering */
	(void) &i;
	(void) &nextin;
	(void) &child;
# endif

	nextin = 0;

	/*
	 * We do not support pipes in barebox, so pi->num_progs can't
	 * be bigger than 1. pi->num_progs == 0 is already catched in
	 * the caller, so everything else than 1 is a bug.
	 */
	BUG_ON(pi->num_progs != 1);

	child = &pi->progs[0];

	if (child->group) {
		hush_debug("non-subshell grouping\n");
		rcode = run_list_real(ctx, child->group);

		return rcode;
	}

	if (!pi->progs[0].argv)
		return -1;

	for (i = 0; is_assignment(child->argv[i]); i++)
		{ /* nothing */ }

	if (i != 0 && child->argv[i] == NULL) {
		/* assignments, but no command: set the local environment */
		for (i = 0; child->argv[i] != NULL; i++) {

			/* Ok, this case is tricky.  We have to decide if this is a
			 * local variable, or an already exported variable.  If it is
			 * already exported, we have to export the new value.  If it is
			 * not exported, we need only set this as a local variable.
			 * This junk is all to decide whether or not to export this
			 * variable. */
			int export_me = 0;
			char *name, *value;

			name = xstrdup(child->argv[i]);
			hush_debug("Local environment set: %s\n", name);
			value = strchr(name, '=');

			if (value)
				*value = 0;

			free(name);
			p = insert_var_value(child->argv[i]);
			rcode = set_local_var(p, export_me);
			if (rcode)
				return 1;

			if (p != child->argv[i])
				free(p);
		}
		return EXIT_SUCCESS;   /* don't worry about errors in set_local_var() yet */
	}
	for (i = 0; is_assignment(child->argv[i]); i++) {
		p = insert_var_value(child->argv[i]);
		rcode = set_local_var(p, 0);
		if (rcode)
			return 1;

		if (p != child->argv[i]) {
			child->sp--;
			free(p);
		}
	}
	if (child->sp) {
		char * str = NULL;
		struct p_context ctx1;

		initialize_context(&ctx1);

		str = make_string((child->argv + i));
		rcode = parse_string_outer(&ctx1, str, FLAG_EXIT_FROM_LOOP | FLAG_REPARSING);
		release_context(&ctx1);
		free(str);

		return rcode;
	}

	do_glob_in_argv(&globbuf, child->argc - i, &child->argv[i]);

	remove_quotes(globbuf.gl_pathc, globbuf.gl_pathv);

	if (!strcmp(globbuf.gl_pathv[0], "getopt") &&
			IS_ENABLED(CONFIG_CMD_GETOPT)) {
		ret = builtin_getopt(ctx, child, globbuf.gl_pathc, globbuf.gl_pathv);
	} else if (!strcmp(globbuf.gl_pathv[0], "exit")) {
		ret = builtin_exit(ctx, child, globbuf.gl_pathc, globbuf.gl_pathv);
	} else {
		ret = execute_binfmt(globbuf.gl_pathc, globbuf.gl_pathv);
		if (ret < 0) {
			printf("%s: %s\n", globbuf.gl_pathv[0], strerror(-ret));
			ret = 127;
		}
	}

	globfree(&globbuf);

	return ret;
}

static int run_list_real(struct p_context *ctx, struct pipe *pi)
{
	char *save_name = NULL;
	char **list = NULL;
	char **save_list = NULL;
	struct pipe *rpipe;
	int flag_rep = 0;
	int rcode=0, flag_skip=1;
	int flag_restore = 0;
	int if_code=0, next_if_code=0;  /* need double-buffer to handle elif */
	reserved_style rmode, skip_more_in_this_rmode = RES_XXXX;

	/* check syntax for "for" */
	for (rpipe = pi; rpipe; rpipe = rpipe->next) {
		if ((rpipe->r_mode == RES_IN ||
		    rpipe->r_mode == RES_FOR) &&
		    (rpipe->next == NULL)) {
			syntax();
			return 1;
		}
		if ((rpipe->r_mode == RES_IN &&
				(rpipe->next->r_mode == RES_IN &&
				rpipe->next->progs->argv != NULL))||
				(rpipe->r_mode == RES_FOR &&
				rpipe->next->r_mode != RES_IN)) {
			syntax();
			return 1;
		}
	}
	for (; pi; pi = (flag_restore != 0) ? rpipe : pi->next) {
		if (pi->r_mode == RES_WHILE || pi->r_mode == RES_UNTIL ||
				pi->r_mode == RES_FOR) {
			/* check Ctrl-C */
			if (ctrlc())
				return 1;
			flag_restore = 0;
			if (!rpipe) {
				flag_rep = 0;
				rpipe = pi;
			}
		}
		rmode = pi->r_mode;
		hush_debug("rmode=%d  if_code=%d  next_if_code=%d skip_more=%d\n",
				rmode, if_code, next_if_code, skip_more_in_this_rmode);
		if (rmode == skip_more_in_this_rmode && flag_skip) {
			if (pi->followup == PIPE_SEQ)
				flag_skip=0;
			continue;
		}

		flag_skip = 1;
		skip_more_in_this_rmode = RES_XXXX;

		if (rmode == RES_THEN || rmode == RES_ELSE)
			if_code = next_if_code;

		if (rmode == RES_THEN &&  if_code)
			continue;

		if (rmode == RES_ELSE && !if_code)
			continue;

		if (rmode == RES_ELIF && !if_code)
			break;

		if (rmode == RES_FOR && pi->num_progs) {
			if (!list) {
				/* if no variable values after "in" we skip "for" */
				if (!pi->next->progs->argv)
					continue;

				/* create list of variable values */
				list = make_list_in(pi->next->progs->argv,
					pi->progs->argv[0]);
				save_list = list;
				save_name = pi->progs->argv[0];
				pi->progs->argv[0] = NULL;
				flag_rep = 1;
			}
			if (!(*list)) {
				free(pi->progs->argv[0]);
				free(save_list);
				list = NULL;
				flag_rep = 0;
				pi->progs->argv[0] = save_name;
				continue;
			} else {
				/* insert new value from list for variable */
				if (pi->progs->argv[0])
					free(pi->progs->argv[0]);
				pi->progs->argv[0] = *list++;
			}
		}
		if (rmode == RES_IN)
			continue;

		if (rmode == RES_DO) {
			if (!flag_rep)
				continue;
		}

		if (rmode == RES_DONE) {
			if (flag_rep) {
				flag_restore = 1;
			} else {
				rpipe = NULL;
			}
		}

		if (pi->num_progs == 0)
			continue;

		rcode = run_pipe_real(ctx, pi);
		hush_debug("run_pipe_real returned %d\n",rcode);

		if (rcode < -1) {
			last_return_code = -rcode - 2;
			return rcode;	/* exit */
		}

		last_return_code = rcode;

		if (rmode == RES_IF || rmode == RES_ELIF )
			next_if_code = rcode;  /* can be overwritten a number of times */

		if (rmode == RES_WHILE)
			flag_rep = !last_return_code;

		if (rmode == RES_UNTIL)
			flag_rep = last_return_code;

		if ((rcode == EXIT_SUCCESS && pi->followup == PIPE_OR) ||
		     (rcode != EXIT_SUCCESS && pi->followup == PIPE_AND) )
			skip_more_in_this_rmode = rmode;
	}
	return rcode;
}

/* broken, of course, but OK for testing */
static __maybe_unused char *indenter(int i)
{
	static char blanks[] = "                                    ";
	return &blanks[sizeof(blanks) - i - 1];
}

/* return code is the exit status of the pipe */
static int free_pipe(struct pipe *pi, int indent)
{
	char **p;
	struct child_prog *child;
	int a, i, ret_code = 0;

	for (i = 0; i < pi->num_progs; i++) {

		child = &pi->progs[i];
		final_printf("%s  command %d:\n", indenter(indent), i);

		if (child->argv) {
			for (a = 0,p = child->argv; *p; a++,p++) {
				final_printf("%s   argv[%d] = %s\n",
						indenter(indent),a,*p);
			}
			globfree(&child->glob_result);
			child->argv = NULL;
		} else if (child->group) {
			ret_code = free_pipe_list(child->group,indent+3);
			final_printf("%s   end group\n",indenter(indent));
		} else {
			final_printf("%s   (nil)\n",indenter(indent));
		}
	}

	free(pi->progs);   /* children are an array, they get freed all at once */
	pi->progs = NULL;

	return ret_code;
}

static int free_pipe_list(struct pipe *head, int indent)
{
	int rcode = 0;   /* if list has no members */
	struct pipe *pi, *next;

	for (pi = head; pi; pi = next) {
		final_printf("%s pipe reserved mode %d\n", indenter(indent), pi->r_mode);
		rcode = free_pipe(pi, indent);
		final_printf("%s pipe followup code %d\n", indenter(indent), pi->followup);
		next = pi->next;
		pi->next = NULL;
		free(pi);
	}
	return rcode;
}

static int xglob(o_string *dest, int flags, glob_t *pglob, int glob_needed)
{
	int gr;

	/* short-circuit for null word */
	/* we can code this better when the debug's are gone */
	if (dest->length == 0) {
		if (dest->nonnull) {
			/* bash man page calls this an "explicit" null */
			gr = fake_glob(dest->data, flags, NULL, pglob);
			hush_debug("globhack returned %d\n",gr);
		} else {
			return 0;
		}
	} else if (glob_needed) {
		gr = do_glob(dest->data, flags, NULL, pglob);
		hush_debug("glob returned %d\n",gr);
	} else {
		gr = fake_glob(dest->data, flags, NULL, pglob);
		hush_debug("globhack returned %d\n",gr);
	}
	if (gr != 0) { /* GLOB_ABORTED ? */
		error_msg("glob(3) error %d",gr);
	}

	/* globprint(glob_target); */

	return gr;
}

/* Select which version we will use */
static int run_list(struct p_context *ctx, struct pipe *pi)
{
	int rcode = 0;

	rcode = run_list_real(ctx, pi);

	/* free_pipe_list has the side effect of clearing memory
	 * In the long run that function can be merged with run_list_real,
	 * but doing that now would hobble the debugging effort. */
	free_pipe_list(pi, 0);

	return rcode;
}

static char *get_dollar_var(char ch);

/* This is used to set local shell variables
   flg_export==0 if only local (not exporting) variable
   flg_export==1 if "new" exporting environ
   flg_export>1  if current startup environ (not call putenv()) */
static int set_local_var(const char *s, int flg_export)
{
	char *name, *value;
	int ret;

	/* might be possible! */
	if (!isalpha(*s))
		return -1;

	name = strdup(s);

	/* Assume when we enter this function that we are already in
	 * NAME=VALUE format.  So the first order of business is to
	 * split 's' on the '=' into 'name' and 'value' */
	value = strchr(name, '=');
	if (!value) {
		free(name);
		return -1;
	}
	*value++ = 0;

	remove_quotes_in_str(value);

	ret = setenv(name, value);
	free(name);

	return ret;
}



static int is_assignment(const char *s)
{
	if (s == NULL)
		return 0;

	if (!isalpha(*s))
		return 0;

	++s;

	while(isalnum(*s) || *s=='_' || *s=='.')
		++s;

	return *s=='=';
}


static struct pipe *new_pipe(void)
{
	return xzalloc(sizeof(struct pipe));
}

static void initialize_context(struct p_context *ctx)
{
	ctx->pipe = NULL;
	ctx->child = NULL;
	ctx->list_head = new_pipe();
	ctx->pipe = ctx->list_head;
	ctx->w = RES_NONE;
	ctx->stack = NULL;
	ctx->old_flag = 0;
	ctx->options_parsed = 0;
	INIT_LIST_HEAD(&ctx->options);
	done_command(ctx);   /* creates the memory for working child */
}

static void release_context(struct p_context *ctx)
{
#ifdef CONFIG_CMD_GETOPT
	struct option *opt, *tmp;

	list_for_each_entry_safe(opt, tmp, &ctx->options, list) {
		free(opt->optarg);
		free(opt);
	}
#endif
}

/* normal return is 0
 * if a reserved word is found, and processed, return 1
 * should handle if, then, elif, else, fi, for, while, until, do, done.
 * case, function, and select are obnoxious, save those for later.
 */
struct reserved_combo {
	char *literal;
	int code;
	long flag;
};
/* Mostly a list of accepted follow-up reserved words.
 * FLAG_END means we are done with the sequence, and are ready
 * to turn the compound list into a command.
 * FLAG_START means the word must start a new compound list.
 */
static struct reserved_combo reserved_list[] = {
	{ "if",    RES_IF,    FLAG_THEN | FLAG_START },
	{ "then",  RES_THEN,  FLAG_ELIF | FLAG_ELSE | FLAG_FI },
	{ "elif",  RES_ELIF,  FLAG_THEN },
	{ "else",  RES_ELSE,  FLAG_FI   },
	{ "fi",    RES_FI,    FLAG_END  },
	{ "for",   RES_FOR,   FLAG_IN   | FLAG_START },
	{ "while", RES_WHILE, FLAG_DO   | FLAG_START },
	{ "until", RES_UNTIL, FLAG_DO   | FLAG_START },
	{ "in",    RES_IN,    FLAG_DO   },
	{ "do",    RES_DO,    FLAG_DONE },
	{ "done",  RES_DONE,  FLAG_END  }
};

static int reserved_word(o_string *dest, struct p_context *ctx)
{
	struct reserved_combo *r;
	int i;

	for (i = 0; i < ARRAY_SIZE(reserved_list); i++) {
		r = &reserved_list[i];

		if (strcmp(dest->data, r->literal))
			continue;

		hush_debug("found reserved word %s, code %d\n",r->literal,r->code);

		if (r->flag & FLAG_START) {
			struct p_context *new = xmalloc(sizeof(struct p_context));

			hush_debug("push stack\n");

			if (ctx->w == RES_IN || ctx->w == RES_FOR) {
				syntax();
				free(new);
				ctx->w = RES_SNTX;
				b_reset(dest);

				return 1;
			}
			*new = *ctx;   /* physical copy */
			initialize_context(ctx);
			ctx->stack = new;
		} else if (ctx->w == RES_NONE || !(ctx->old_flag & (1 << r->code))) {
			syntax_unexpected_token(r->literal);
			ctx->w = RES_SNTX;
			b_reset(dest);
			return 1;
		}

		ctx->w = r->code;
		ctx->old_flag = r->flag;

		if (ctx->old_flag & FLAG_END) {
			struct p_context *old;

			hush_debug("pop stack\n");

			done_pipe(ctx,PIPE_SEQ);
			old = ctx->stack;
			old->child->group = ctx->list_head;
			*ctx = *old;   /* physical copy */
			free(old);
		}

		b_reset(dest);

		return 1;
	}

	return 0;
}

/* normal return is 0.
 * Syntax or xglob errors return 1. */
static int done_word(o_string *dest, struct p_context *ctx)
{
	struct child_prog *child = ctx->child;
	glob_t *glob_target;
	int gr, flags = GLOB_NOCHECK;

	hush_debug("%s: %s %p\n", __func__, dest->data, child);
	if (dest->length == 0 && !dest->nonnull) {
		hush_debug("  true null, ignored\n");
		return 0;
	}
	if (child->group) {
		syntax();
		return 1;  /* syntax error, groups and arglists don't mix */
	}
	if (!child->argv && (ctx->type & FLAG_PARSE_SEMICOLON)) {
		hush_debug("checking %s for reserved-ness\n",dest->data);
		if (reserved_word(dest,ctx))
			return ctx->w == RES_SNTX;
	}

	glob_target = &child->glob_result;
	if (child->argv)
		flags |= GLOB_APPEND;

	gr = xglob(dest, flags, glob_target, ctx->w == RES_IN ? 1 : 0);
	if (gr)
		return 1;

	b_reset(dest);

	child->argv = glob_target->gl_pathv;
	child->argc = glob_target->gl_pathc;

	if (ctx->w == RES_FOR) {
		done_word(dest,ctx);
		done_pipe(ctx,PIPE_SEQ);
	}

	return 0;
}

/* The only possible error here is out of memory, in which case
 * xmalloc exits. */
static int done_command(struct p_context *ctx)
{
	/* The child is really already in the pipe structure, so
	 * advance the pipe counter and make a new, null child.
	 * Only real trickiness here is that the uncommitted
	 * child structure, to which ctx->child points, is not
	 * counted in pi->num_progs. */
	struct pipe *pi = ctx->pipe;
	struct child_prog *prog = ctx->child;

	if (prog && prog->group == NULL && prog->argv == NULL) {
		hush_debug("%s: skipping null command\n", __func__);
		return 0;
	} else if (prog) {
		pi->num_progs++;
		hush_debug("%s: num_progs incremented to %d\n", __func__, pi->num_progs);
	} else {
		hush_debug("%s: initializing\n", __func__);
	}
	pi->progs = xrealloc(pi->progs, sizeof(*pi->progs) * (pi->num_progs + 1));

	prog = pi->progs + pi->num_progs;
	prog->glob_result.gl_pathv = NULL;

	prog->argv = NULL;
	prog->group = NULL;
	prog->sp = 0;
	ctx->child = prog;
	prog->type = ctx->type;

	/* but ctx->pipe and ctx->list_head remain unchanged */
	return 0;
}

static int done_pipe(struct p_context *ctx, pipe_style type)
{
	struct pipe *new_p;

	done_command(ctx);  /* implicit closure of previous command */

	hush_debug("%s: type %d\n", __func__, type);

	ctx->pipe->followup = type;
	ctx->pipe->r_mode = ctx->w;

	new_p = new_pipe();

	ctx->pipe->next = new_p;
	ctx->pipe = new_p;
	ctx->child = NULL;

	done_command(ctx);  /* set up new pipe to accept commands */

	return 0;
}


/* basically useful version until someone wants to get fancier,
 * see the bash man page under "Parameter Expansion" */
static const char *lookup_param(char *src)
{
	if (!src)
		return NULL;

	if (*src == '$')
		return get_dollar_var(src[1]);

	return getenv(src);
}

static int parse_string(o_string *dest, struct p_context *ctx, const char *src)
{
	struct in_str foo;

	setup_string_in_str(&foo, src);

	return parse_stream(dest, ctx, &foo, '\0');
}

static char *get_dollar_var(char ch)
{
	static char buf[40];

	buf[0] = '\0';

	switch (ch) {
		case '?':
			sprintf(buf, "%u", (unsigned int)last_return_code);
			break;
		default:
			return NULL;
	}
	return buf;
}

/* return code: 0 for OK, 1 for syntax error */
static int handle_dollar(o_string *dest, struct p_context *ctx, struct in_str *input)
{
	int advance = 0, i;
	int ch = input->peek(input);  /* first character after the $ */

	hush_debug("%s: ch=%c\n", __func__, ch);

	if (isalpha(ch)) {
		b_addchr(dest, SPECIAL_VAR_SYMBOL);
		ctx->child->sp++;

		while (ch = b_peek(input),isalnum(ch) || ch == '_' || ch == '.') {
			b_getch(input);
			b_addchr(dest,ch);
		}

		b_addchr(dest, SPECIAL_VAR_SYMBOL);

	} else if (isdigit(ch)) {

		i = ch - '0';	/* XXX is $0 special? */
		if (i < ctx->global_argc) {
			parse_string(dest, ctx, ctx->global_argv[i]);        /* recursion */
		}
		advance = 1;
	} else {
		switch (ch) {
		case '?':
			ctx->child->sp++;
			b_addchr(dest, SPECIAL_VAR_SYMBOL);
			b_addchr(dest, '$');
			b_addchr(dest, '?');
			b_addchr(dest, SPECIAL_VAR_SYMBOL);
			advance = 1;
			break;
		case '#':
			b_adduint(dest,ctx->global_argc ? ctx->global_argc-1 : 0);
			advance = 1;
			break;
		case '{':
			b_addchr(dest, SPECIAL_VAR_SYMBOL);
			ctx->child->sp++;
			b_getch(input);

			/* XXX maybe someone will try to escape the '}' */

			while(ch = b_getch(input),ch != EOF && ch != '}') {
				b_addchr(dest,ch);
			}
			if (ch != '}') {
				syntax();
				return 1;
			}
			b_addchr(dest, SPECIAL_VAR_SYMBOL);
			break;
		case '*':
			for (i = 1; i < ctx->global_argc; i++) {
				b_addstr(dest, ctx->global_argv[i]);
				b_addchr(dest, ' ');
			}

			advance = 1;
			break;
		default:
			b_addchr(dest, '$');
		}
	}
	/* Eat the character if the flag was set.  If the compiler
	 * is smart enough, we could substitute "b_getch(input);"
	 * for all the "advance = 1;" above, and also end up with
	 * a nice size-optimized program.  Hah!  That'll be the day.
	 */
	if (advance)
		b_getch(input);

	return 0;
}


/* return code is 0 for normal exit, 1 for syntax error */
static int parse_stream(o_string *dest, struct p_context *ctx,
	struct in_str *input, int end_trigger)
{
	unsigned int ch, m;
	int next;

	/* Only double-quote state is handled in the state variable dest->quote.
	 * A single-quote triggers a bypass of the main loop until its mate is
	 * found.  When recursing, quote state is passed in via dest->quote. */

	hush_debug("%s: end_trigger=%d\n", __func__, end_trigger);

	while ((ch = b_getch(input)) != EOF) {
		m = map[ch];
		if (input->interrupt)
			return 1;
		next = (ch == '\n') ? 0 : b_peek(input);

		hush_debug("%s: ch=%c (%d) m=%d quote=%d - %c\n",
				__func__,
				ch >= ' ' ? ch : '.', ch, m,
				dest->quote, ctx->stack == NULL ? '*' : '.');

		if (m == 0 || ((m == 1 || m == 2) && dest->quote)) {
			b_addchr(dest, ch);
			continue;
		}

		if (m == 2) {  /* unquoted IFS */
			if (done_word(dest, ctx)) {
				return 1;
			}
			/* If we aren't performing a substitution, treat a newline as a
			 * command separator.  */
			if (end_trigger != '\0' && ch == '\n')
				done_pipe(ctx, PIPE_SEQ);
		}

		if (ch == end_trigger && !dest->quote && ctx->w==RES_NONE) {
			hush_debug("%s: leaving (triggered)\n", __func__);
			return 0;
		}

		if (m == 2)
			continue;

		switch (ch) {
		case '#':
			if (dest->length == 0 && !dest->quote) {
				while (ch = b_peek(input), ch != EOF && ch!='\n') {
					b_getch(input);
				}
			} else {
				b_addchr(dest, ch);
			}
			break;
		case '\\':
			if (next == EOF) {
				syntax();
				return 1;
			}
			b_addchr(dest, '\\');
			b_addchr(dest, b_getch(input));
			break;
		case '$':
			if (handle_dollar(dest, ctx, input)!=0)
				return 1;
			break;
		case '\'':
			dest->nonnull = 1;
			b_addchr(dest, '\'');
			while (ch = b_getch(input), ch != EOF && ch != '\'') {
				if (input->interrupt)
					return 1;
				b_addchr(dest,ch);
			}
			b_addchr(dest, '\'');
			if (ch == EOF) {
				syntax();
				return 1;
			}
			break;
		case '"':
			dest->nonnull = 1;
			b_addchr(dest, '"');
			dest->quote = !dest->quote;
			break;
		case ';':
			done_word(dest, ctx);
			done_pipe(ctx, PIPE_SEQ);
			break;
		case '&':
			done_word(dest, ctx);
			if (next == '&') {
				b_getch(input);
				done_pipe(ctx, PIPE_AND);
			} else {
				syntax();
				return 1;
			}
			break;
		case '|':
			done_word(dest, ctx);
			if (next == '|') {
				b_getch(input);
				done_pipe(ctx, PIPE_OR);
			} else {
				/* we could pick up a file descriptor choice here
				 * with redirect_opt_num(), but bash doesn't do it.
				 * "echo foo 2| cat" yields "foo 2". */
				syntax();
				return 1;
			}
			break;
		default:
			syntax();   /* this is really an internal logic error */
			return 1;
		}
	}

	/* complain if quote?  No, maybe we just finished a command substitution
	 * that was quoted.  Example:
	 * $ echo "`cat foo` plus more"
	 * and we just got the EOF generated by the subshell that ran "cat foo"
	 * The only real complaint is if we got an EOF when end_trigger != '\0',
	 * that is, we were really supposed to get end_trigger, and never got
	 * one before the EOF.  Can't use the standard "syntax error" return code,
	 * so that parse_stream_outer can distinguish the EOF and exit smoothly. */
	hush_debug("%s: leaving (EOF)\n", __func__);

	if (end_trigger != '\0')
		return -1;
	return 0;
}

static void mapset(const unsigned char *set, int code)
{
	const unsigned char *s;

	for (s = set; *s; s++)
		map[*s] = code;
}

static void update_ifs_map(void)
{
	/* char *ifs and char map[256] are both globals. */
	ifs = (uchar *)getenv("IFS");
	ifs = NULL;
	if (ifs == NULL)
		ifs = (uchar *)" \t\n";

	/* Precompute a list of 'flow through' behavior so it can be treated
	 * quickly up front.  Computation is necessary because of IFS.
	 * Special case handling of IFS == " \t\n" is not implemented.
	 * The map[] array only really needs two bits each, and on most machines
	 * that would be faster because of the reduced L1 cache footprint.
	 */
	memset(map, 0, sizeof(map));	/* most characters flow through always */
	mapset((uchar *)"\\$'\"", 3);	/* never flow through */
	mapset((uchar *)";&|#", 1);	/* flow through if quoted */
	mapset(ifs, 2);			/* also flow through if quoted */
}

/*
 * shell_expand - Expand shell variables in a string.
 * @str:	The input string containing shell variables like
 *		$var or ${var}
 * Return:	The expanded string. Must be freed with free().
 */
char *shell_expand(char *str)
{
	struct p_context ctx = {};
	o_string o = {};
	char *res, *parsed;

	remove_quotes_in_str(str);

	o.quote = 1;

	initialize_context(&ctx);

	parse_string(&o, &ctx, str);

	parsed = xmemdup(o.data, o.length + 1);
	parsed[o.length] = 0;

	res = insert_var_value(parsed);
	if (res != parsed)
		free(parsed);

	free_pipe_list(ctx.list_head, 0);
	b_free(&o);

	return res;
}

/* most recursion does not come through here, the exeception is
 * from builtin_source() */
static int parse_stream_outer(struct p_context *ctx, struct in_str *inp, int flag)
{
	o_string temp = NULL_O_STRING;
	int rcode;
	int code = 0;

	do {
		ctx->type = flag;
		initialize_context(ctx);
		update_ifs_map();

		if (!(flag & FLAG_PARSE_SEMICOLON) || (flag & FLAG_REPARSING))
			mapset((uchar *)";$&|", 0);

		inp->promptmode = 1;
		rcode = parse_stream(&temp, ctx, inp, '\n');

		if (rcode != 1 && ctx->old_flag != 0) {
			syntax();
			return 1;
		}
		if (rcode != 1 && ctx->old_flag == 0) {
			done_word(&temp, ctx);
			done_pipe(ctx, PIPE_SEQ);
			if (ctx->list_head->num_progs) {
				code = run_list(ctx, ctx->list_head);
			} else {
				free_pipe_list(ctx->list_head, 0);
				continue;
			}
			if (code < -1) {	/* exit */
				b_free(&temp);
				return code;
			}
		} else {
			if (ctx->old_flag != 0) {
				free(ctx->stack);
				b_reset(&temp);
			}
			if (inp->interrupt)
				printf("<INTERRUPT>\n");
			temp.nonnull = 0;
			temp.quote = 0;
			free_pipe_list(ctx->list_head,0);
			b_free(&temp);
			return 1;
		}
		b_free(&temp);
	} while (rcode != -1 && !(flag & FLAG_EXIT_FROM_LOOP));   /* loop on syntax errors, return on EOF */

	return code;
}

static int parse_string_outer(struct p_context *ctx, const char *s, int flag)
{
	struct in_str input;
	char *p;
	const char *cp;
	int rcode;

	if ( !s || !*s)
		return 1;

	cp = strchr(s, '\n');
	if (!cp || *++cp) {
		p = xmalloc(strlen(s) + 2);
		strcpy(p, s);
		strcat(p, "\n");
		setup_string_in_str(&input, p);
		rcode = parse_stream_outer(ctx, &input, flag);
		free(p);
	} else {
		setup_string_in_str(&input, s);
		rcode = parse_stream_outer(ctx, &input, flag);
	}

	return rcode;
}

static char *insert_var_value(char *inp)
{
	int res_str_len = 0;
	int len;
	int done = 0;
	char *p, *res_str = NULL;
	const char *p1;

	while ((p = strchr(inp, SPECIAL_VAR_SYMBOL))) {
		if (p != inp) {
			len = p - inp;
			res_str = xrealloc(res_str, (res_str_len + len));
			strncpy((res_str + res_str_len), inp, len);
			res_str_len += len;
		}
		inp = ++p;
		p = strchr(inp, SPECIAL_VAR_SYMBOL);
		*p = '\0';
		if ((p1 = lookup_param(inp))) {
			len = res_str_len + strlen(p1);
			res_str = xrealloc(res_str, (1 + len));
			strcpy((res_str + res_str_len), p1);
			res_str_len = len;
		}
		*p = SPECIAL_VAR_SYMBOL;
		inp = ++p;
		done = 1;
	}
	if (done) {
		res_str = xrealloc(res_str, (1 + res_str_len + strlen(inp)));
		strcpy((res_str + res_str_len), inp);
		while ((p = strchr(res_str, '\n'))) {
			*p = ' ';
		}
	}
	return (res_str == NULL) ? inp : res_str;
}

static char **make_list_in(char **inp, char *name)
{
	int len, i;
	int name_len = strlen(name);
	int n = 0;
	char **list;
	char *p1, *p2, *p3;

	/* create list of variable values */
	list = xmalloc(sizeof(*list));
	for (i = 0; inp[i]; i++) {
		p3 = insert_var_value(inp[i]);
		p1 = p3;
		while (*p1) {
			if ((*p1 == ' ')) {
				p1++;
				continue;
			}
			if ((p2 = strchr(p1, ' '))) {
				len = p2 - p1;
			} else {
				len = strlen(p1);
				p2 = p1 + len;
			}
			/* we use n + 2 in realloc for list,because we add
			 * new element and then we will add NULL element */
			list = xrealloc(list, sizeof(*list) * (n + 2));
			list[n] = xmalloc(2 + name_len + len);
			strcpy(list[n], name);
			strcat(list[n], "=");
			strncat(list[n], p1, len);
			list[n++][name_len + len + 1] = '\0';
			p1 = p2;
		}
		if (p3 != inp[i])
			free(p3);
	}
	list[n] = NULL;
	return list;
}

/* Make new string for parser */
static char * make_string(char ** inp)
{
	char *p;
	char *str = NULL;
	int n;
	int len = 2;

	for (n = 0; inp[n]; n++) {
		p = insert_var_value(inp[n]);
		str = xrealloc(str, (len + strlen(p)));
		if (n) {
			strcat(str, " ");
		} else {
			*str = '\0';
		}
		strcat(str, p);
		len = strlen(str) + 3;
		if (p != inp[n])
			free(p);
	}
	len = strlen(str);
	*(str + len) = '\n';
	*(str + len + 1) = '\0';
	return str;
}

int run_command(const char *cmd)
{
	struct p_context ctx = {};
	int ret;

	initialize_context(&ctx);

	ret = parse_string_outer(&ctx, cmd, FLAG_PARSE_SEMICOLON);
	release_context(&ctx);

	return ret;
}

static int execute_script(const char *path, int argc, char *argv[])
{
	int ret;

	env_push_context();
	ret = source_script(path, argc, argv);
	env_pop_context();

	return ret;
}

static int source_script(const char *path, int argc, char *argv[])
{
	struct p_context ctx = {};
	char *script;
	int ret;

	initialize_context(&ctx);

	ctx.global_argc = argc;
	ctx.global_argv = argv;

	script = read_file(path, NULL);
	if (!script) {
		perror("sh");
		return 1;
	}

	ret = parse_string_outer(&ctx, script, FLAG_PARSE_SEMICOLON);
	if (ret < -1)
		ret = -ret - 2;

	release_context(&ctx);
	free(script);

	return ret;
}

int run_shell(void)
{
	int rcode;
	struct in_str input;
	struct p_context ctx = {};
	int exit = 0;

	login();

	do {
		setup_file_in_str(&input);
		rcode = parse_stream_outer(&ctx, &input, FLAG_PARSE_SEMICOLON);
		if (rcode < -1) {
			exit = 1;
			rcode = -rcode - 2;
		}
		release_context(&ctx);
	} while (!exit);

	return rcode;
}

static int do_sh(int argc, char *argv[])
{
	if (argc < 2)
		return run_shell();

	return execute_script(argv[1], argc - 1, argv + 1);
}

BAREBOX_CMD_START(sh)
	.cmd		= do_sh,
	BAREBOX_CMD_DESC("execute a shell script")
	BAREBOX_CMD_OPTS("FILE [ARGUMENT...]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
BAREBOX_CMD_END

static int do_source(int argc, char *argv[])
{
	char *path;
	int ret;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	if (strchr(argv[1], '/')) {
		path = xstrdup(argv[1]);
	} else {
		path = find_execable(argv[1]);
		if (!path)
			return 1;
	}

	ret = source_script(path, argc - 1, argv + 1);

	free(path);

	return ret;
}

static const char * const source_aliases[] = { ".", NULL};

BAREBOX_CMD_HELP_START(source)
BAREBOX_CMD_HELP_TEXT("Read and execute commands from FILE in the current shell environment.")
BAREBOX_CMD_HELP_TEXT("and return the exit status of the last command executed.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(source)
	.aliases	= source_aliases,
	.cmd		= do_source,
	BAREBOX_CMD_DESC("execute shell script in current shell environment")
	BAREBOX_CMD_OPTS("FILE [ARGUMENT...]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_source_help)
BAREBOX_CMD_END

static int do_dummy_command(int argc, char *argv[])
{
	/*
	 * This function is never reached. These commands are only here to
	 * provide help texts for the builtins.
	 */
	return 0;
}

BAREBOX_CMD_HELP_START(exit)
BAREBOX_CMD_HELP_TEXT("Exit script with status ERRLVL n. If ERRLVL is omitted, the exit status is")
BAREBOX_CMD_HELP_TEXT("of the last command executed")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(exit)
	.cmd		= do_dummy_command,
	BAREBOX_CMD_DESC("exit script")
	BAREBOX_CMD_OPTS("[ERRLVL]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_exit_help)
BAREBOX_CMD_END

#ifdef CONFIG_CMD_GETOPT
BAREBOX_CMD_HELP_START(getopt)
BAREBOX_CMD_HELP_TEXT("OPTSTRING contains the option letters. Add a colon to an options if this")
BAREBOX_CMD_HELP_TEXT("option has a required argument or two colons for an optional argument. The")
BAREBOX_CMD_HELP_TEXT("current option is saved in VAR, arguments are saved in $OPTARG. Any")
BAREBOX_CMD_HELP_TEXT("non-option arguments can be accessed starting from $1.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(getopt)
	.cmd		= do_dummy_command,
	BAREBOX_CMD_DESC("parse option arguments")
	BAREBOX_CMD_OPTS("OPTSTRING VAR")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_getopt_help)
BAREBOX_CMD_END
#endif

BAREBOX_MAGICVAR(PATH, "colon separated list of paths to search for executables");
#ifdef CONFIG_HUSH_FANCY_PROMPT
BAREBOX_MAGICVAR(PS1, "hush prompt");
#endif

static int binfmt_sh_excute(struct binfmt_hook *b, char *file, int argc, char **argv)
{
	return execute_script(file, argc, argv);
}

static struct binfmt_hook binfmt_sh_hook = {
	.type = filetype_sh,
	.hook = binfmt_sh_excute,
};

static int binfmt_sh_init(void)
{
	return binfmt_register(&binfmt_sh_hook);
}
fs_initcall(binfmt_sh_init);

/**
 * @file
 * @brief A prototype Bourne shell grammar parser
 */

/** @page sh_command Starting shell
 *
 * Usage: sh \<filename\> [\<arguments\>]
 *
 * Execute a shell script named \<filename\> and forward (if given)
 * \<arguments\> to it.
 *
 * Usage: .  \<filename\> [\<arguments\>]
 * or     source \<filename\> [\<arguments\>]
 *
 * Read and execute commands from \<filename\> in the current shell environment,
 * forward (if given) \<arguments\> to it and return the exit status of the last
 * command  executed from filename.
 */
