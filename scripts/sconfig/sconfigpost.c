// SPDX-License-Identifier: GPL-2.0-only
// Postprocess Sconfig files

#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>
#include <libgen.h>

static void usage(int exitcode);

static const char *argv0;
static char *symbol_name = "untitled";
static const char *policy_name = "untitled";

#define MAX(x, y) ((x) > (y) ? (x) : (y))

enum print { PRINT_ARRAY, PRINT_ENUM, PRINT_DEFINES, PRINT_STRINGS };

#define panic(fmt, ...) do {                                    \
	fprintf(stderr, "%s: " fmt, argv0, ##__VA_ARGS__);      \
	exit(6);                                                \
} while (0)

#define xasprintf(args...) ({           \
	char *_buf;                     \
	if (asprintf(&_buf, args) < 0)  \
	panic("asprintf: %m\n");        \
	_buf;                           \
})

#define xstrdup(args...) nonnull(strdup(args))
#define xmalloc(args...) nonnull(malloc(args))
#define xrealloc(args...) nonnull(realloc(args))

#ifdef DEBUG
#define debug(args...) fprintf(stderr, args)
#else
#define debug(args...) (void)0
#endif

static inline size_t str_has_prefix(const char *str, const char *prefix)
{
	size_t len = strlen(prefix);
	return strncmp(str, prefix, len) == 0 ? len : 0;
}

static void *pnonnull(void *ptr, const char *ptrname)
{
	if (!ptr)
		panic("%s is unexpectedly NULL\n", ptrname);
	return ptr;
}

#define nonnull(ptr) pnonnull(ptr, #ptr)

static FILE *xfopen(const char *path, const char *mode)
{
	FILE *fp;

	fp = fopen(path, mode);
	if (!fp)
		panic("failed to open \"%s\" with mode '%s': %m\n",
		      path, mode);

	return fp;
}

static void print_global_header(FILE *out, enum print print)
{
	switch (print) {
	case PRINT_ARRAY:
		fprintf(out, "#include <security/defs.h>\n");
		fprintf(out, "const struct security_policy __policy_%s[] = {\n", symbol_name);
		break;
	case PRINT_STRINGS:
		fprintf(out, "#include <security/defs.h>\n");
		break;
	default:
		break;
	}
}

static void print_header(FILE *out, enum print print, bool is_last)
{
	switch (print) {
	case PRINT_ARRAY:
		fprintf(out, "{\n");
		fprintf(out, "\t.name = \"%s\",\n", policy_name);
		fprintf(out, "\t.chained = %d,\n", is_last ? 0 : 1);
		fprintf(out, "\t.policy = {\n");
		break;
	case PRINT_ENUM:
		fprintf(out, "enum security_config_option {\n");
		break;
	case PRINT_STRINGS:
		fprintf(out, "const char *sconfig_names[SCONFIG_NUM] = {\n");
		break;
	default:
		break;
	}
}

static void print_elem(FILE *out, enum print print,
		       const char *config, bool enable, int i)
{
	switch (print) {
	case PRINT_DEFINES:
		fprintf(out, "#define %-40s %u\n", config, i);
		break;
	case PRINT_ARRAY:
		fprintf(out, "\t[%-40s] = %s,\n", config,
			enable ? "1" : "0");
		break;
	case PRINT_ENUM:
		fprintf(out, "\t%s = %i,\n", config, i);
		break;
	case PRINT_STRINGS:
		fprintf(out, "\t[%s] = \"%s\",\n", config, config);
		break;
	}
}

static void print_footer(FILE *out, enum print print, bool is_last, int i)
{
	switch (print) {
	case PRINT_DEFINES:
		return;
	case PRINT_ENUM:
		fprintf(out, "\tSCONFIG_NUM = %i\n};\n", i);
		break;
	case PRINT_ARRAY:
		if (is_last)
			fprintf(out, "\t}}\n");
		else
			fprintf(out, "\t}},\n");
		break;
	case PRINT_STRINGS:
		fprintf(out, "};\n");
		break;
	}
}

static void print_global_footer(FILE *out, enum print print)
{
	switch (print) {
	case PRINT_ARRAY:
		fprintf(out, "};\n");
		break;
	default:
		break;
	}
}

static const char *nextline(char **line, FILE *in)
{
	size_t len = 0;
	ssize_t nread;

	do {
		nread = getline(line, &len, in);
		if (nread < 0)
			return NULL;
	} while (!nread);

	if (nread > 256)
		panic("line \"%s\" exceeds maximum length of 256\n", *line);

	if ((*line)[nread - 1] == '\n')
		(*line)[nread - 1] = '\0';

	return *line;
}

#define CONFIG_VAL_SKIP		(void *)1

static const char *parse_config_val(char *line, bool *enable)
{
	static char config[256];
	static char name[256];
	char end[2];
	int val_offset;
	int ret;

	if ((ret = sscanf(line, "# %255[A-Za-z0-9_] is not se%1[t]",
			  config, end)) == 2) {
		if (str_has_prefix(config, "SCONFIG_POLICY_"))
			return CONFIG_VAL_SKIP;
		if (enable)
			*enable = false;
	} else if ((ret = sscanf(line, "SCONFIG_POLICY_%255[A-Za-z0-9_]=%255s",
				 config, name)) == 2) {
		if (!strcmp(config, "NAME")) {
			policy_name = name + 1;
			*strchrnul(policy_name, '"') = '\0';
		}
		return CONFIG_VAL_SKIP;
	} else if ((ret = sscanf(line, "%255[A-Za-z0-9_]=%n", config,
				 &val_offset)) == 1) {
		if (strcmp(&line[val_offset], "y"))
			panic("\"%s\": bool data type expected\n", line);

		if (enable)
			*enable = true;
	} else {
		return NULL;
	}

	return config;
}

static void strsanitize(char *str)
{
	size_t i;

	while ((i = strcspn(str, "-."))) {
		switch (str[i]) {
		case '-':
			str[i] = '_';
			break;
		case '.':
			str[i] = '\0';
			/* fallthrough */
		case '\0':
			return;
		}

		str += i + 1;
	}
}

static int parse_ext(const struct dirent *dir)
{
	const char *ext;

	if (!dir || dir->d_type == DT_DIR)
		return 0;

	ext = strrchr(dir->d_name, '.');
	if (!ext || ext == dir->d_name)
		return 0;

	return strcmp(ext, ".sconfig") == 0;
}

static time_t newest_mtime;

static void stat_inputfile(const char *path, struct stat *st)
{
	if (stat(path, st))
		panic("Input file '%s' doesn't exist: %m\n", path);

	if (!S_ISDIR(st->st_mode))
		newest_mtime = MAX(newest_mtime, st->st_mtime);
}

static char **collect_input(int argc, char *argv[])
{
	char **buf;
	int i = 0;

	argc++;
	buf = xmalloc(argc * sizeof(*buf));

	for (char **arg = argv; *arg; arg++) {
		char **newbuf;
		struct stat st;
		struct dirent **namelist;
		int n;

		stat_inputfile(*arg, &st);

		if (!S_ISDIR(st.st_mode)) {
			buf[i++] = *arg;
			continue;
		}

		n = scandir(*arg, &namelist, parse_ext, alphasort);
		if (n < 0)
			panic("scandir: %m\n");

		argc += n;

		newbuf = xrealloc(buf, argc * sizeof(*newbuf));
		buf = newbuf;

		for (int j = 0; j < n; j++) {
			buf[i + j] = xasprintf("%s/%s", *arg, namelist[j]->d_name);
			free(namelist[j]);

			/* update newest_times */
			stat_inputfile(buf[i + j], &st);
		}

		free(namelist);
		i += n;
	}

	buf[i] = NULL;
	return buf;
}

static long fsize(FILE *fp)
{
	long size;

	fseek(fp, 0, SEEK_END);

	size = ftell(fp);
	if (size < 0)
		panic("ftell: %m\n");

	fseek(fp, 0, SEEK_SET);

	return size;
}

static bool fidentical(FILE *fp1, FILE *fp2)
{
	int ch1, ch2;
	if (fsize(fp1) != fsize(fp2)) {
		debug("file size mismatch\n");
		return false;
	}

	do {
		ch1 = getc(fp1);
		ch2 = getc(fp2);
		if (ch1 != ch2)
			return false;
	} while (ch1 != EOF && ch2 != EOF);

	return true;
}

static char *make_tmp_path(const char *path, FILE **fp)
{

	const char *filename, *slash;
	char *tmpfilepath;

	slash = strrchr(path, '/');
	filename = slash ? slash + 1 : path;

	if (slash)
		tmpfilepath = xasprintf("%.*s/.%s.tmp", (int)(slash - path), path,
					filename);
	else
		tmpfilepath = xasprintf(".%s.tmp", filename);

	*fp = xfopen(tmpfilepath, "w+");

	return tmpfilepath;
}

static void append_dependency(FILE *depfile, const char *path)
{
	char *abspath;

	if (!depfile)
		return;

	if (!path) {
		fprintf(depfile, "\n");
		return;
	}

	debug("appending dependency for %s\n", path);

	abspath = nonnull(realpath(path, NULL));

	fprintf(depfile, "\t%s \\\n", abspath);

	free(abspath);
}

int main(int argc, char *argv[])
{
	char *outfilepath = NULL;
	char *tmpfilepath = NULL;
	char **infilepaths;
	enum print print = PRINT_ARRAY;
	FILE *in = stdin, *out = stdout, *out_final = NULL, *depfile = NULL;
	char *line = NULL;
	int opt;

	argv0 = argv[0];

	while ((opt = getopt(argc, argv, "esdD:o:h")) > 0) {
		switch (opt) {
		case 'e':
			print = PRINT_ENUM;
			break;
		case 's':
			print = PRINT_STRINGS;
			break;
		case 'd':
			print = PRINT_DEFINES;
			break;
		case 'D':
			depfile = xfopen(optarg, "w");
			break;
		case 'o':
			outfilepath = optarg;
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	if (depfile && !outfilepath)
		panic("can't generate depfile without -o argument\n");

	if (argc - optind > 1 && print != PRINT_ARRAY)
		panic("processing multiple files at once only possible for array mode\n");

	if (argc == optind && depfile)
		panic("can't generate depfile while reading stdin\n");

	if (outfilepath) {
		out_final = fopen(outfilepath, "r+");
		if (out_final) {
			tmpfilepath = make_tmp_path(outfilepath, &out);
		} else if (outfilepath) {
			out = xfopen(outfilepath, "w");
		}

		symbol_name = xasprintf("%s", basename(outfilepath));
		strsanitize(symbol_name);
	}

	infilepaths = collect_input(argc - optind, &argv[optind]);

	print_global_header(out, print);

	if (depfile)
		fprintf(depfile, "%s: \\\n", outfilepath);

	for (char **infilepath = infilepaths; *infilepath; infilepath++) {
		bool is_last = infilepath[1] == NULL;
		bool hdr_printed = false;
		const char *comment_prefix = "";
		int option_idx = 0;

		in = xfopen(*infilepath, "r");

		while (nextline(&line, in)) {
			const char *config;
			bool enable;

			config = parse_config_val(line, &enable);
			if (config == CONFIG_VAL_SKIP)
				continue;
			if (!config) {
				if (line[0] == '#')
					fprintf(out, "%s// %s\n", comment_prefix, line + 1);

				continue;
			}

			if (!hdr_printed) {
				print_header(out, print, is_last);
				if (print != PRINT_DEFINES)
					comment_prefix = "\t";
				hdr_printed = true;
			}

			print_elem(out, print, config, enable, option_idx++);
		}

		print_footer(out, print, is_last, option_idx);
		fputs("\n", out);

		if (print != PRINT_ARRAY) {
			rewind(in);

			while (nextline(&line, in)) {
				const char *config = parse_config_val(line, NULL);
				if (config && config != CONFIG_VAL_SKIP)
					fprintf(out, "#define\t%s_DEFINED 1\n", config);
			}
		}

		free(line);
		fclose(in);

		append_dependency(depfile, *infilepath);
	}

	print_global_footer(out, print);

	fflush(out);

	if (out_final) {
		if (fidentical(out, out_final)) {
			struct stat st;

			debug("removing %s\n", tmpfilepath);
			remove(tmpfilepath);

			if (stat(outfilepath, &st))
				panic("Output file '%s' doesn't exist?? %m\n", outfilepath);
			if (st.st_mtime <= newest_mtime)
				utime(outfilepath, NULL);
		} else {
			debug("renaming %s to %s\n", tmpfilepath, outfilepath);
			rename(tmpfilepath, outfilepath);
		}
	}

	append_dependency(depfile, "include/generated/autoconf.h");
	if (print == PRINT_ARRAY || print == PRINT_STRINGS)
		append_dependency(depfile, "include/generated/security_autoconf.h");
	append_dependency(depfile, NULL);

	return 0;
}

static void usage(int exitcode)
{
	FILE *fp = exitcode ? stderr : stdout;

	fprintf(fp,
		"usage: %s [-o FILE] [-d DEPFILE] [-edph] [config]\n"
		"This script postprocess a barebox Sconfig (Security config)\n"
		"in the Kconfig .config format for further use inside barebox.\n"
		"If no positional argument is specified, the script operates on stdin\n"
		"Options:\n"
		"  -e             Output policy as C enumeration\n"
		"  -s             Output policy option names as C array of strings\n"
		"  -d             Output policy as C preprocessor defines\n"
		"  -D <depfile>   Write depedencies into <depfile>\n"
		"  -o <outfile>   Write output into <outfile> instead of stdout\n"
		"  -h		  This help text\n", argv0);

	exit(exitcode);
}
