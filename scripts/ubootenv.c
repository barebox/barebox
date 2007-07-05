#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

void *xmalloc(size_t size)
{
	void *p = NULL;

	if (!(p = malloc(size))) {
		printf("ERROR: out of memory\n");
		exit(1);
	}

	return p;
}

#include "../include/envfs.h"
#include "../commands/environment.c"

void usage(char *prgname)
{
	printf( "Usage : %s [OPTION] DIRECTORY FILE\n"
		"Load an u-boot environment sector into a directory or\n"
		"save a directory into an u-boot environment sector\n"
		"\n"
		"options:\n"
		"  -s        save (directory -> environment sector)\n"
		"  -l        load (environment sector -> directory)\n",
		prgname);
}

int main(int argc, char *argv[])
{
	int opt;
	int save = 0, load = 0;
	char *filename = NULL, *dirname = NULL;

	while((opt = getopt(argc, argv, "sl")) != -1) {
		switch (opt) {
		case 's':
			save = 1;
			break;
		case 'l':
			load = 1;
			break;
		}
	}

	if (optind + 1 >= argc) {
		usage(argv[0]);
		exit(1);
	}

	dirname = argv[optind];
	filename = argv[optind + 1];

	if ((!load && !save) || (load && save) || !filename || !dirname) {
		usage(argv[0]);
		exit(1);
	}

	if (load) {
		printf("loading env from file %s to %s\n", filename, dirname);
		envfs_load(filename, dirname);
	}
	if (save) {
		printf("saving contents of %s to file %s\n", dirname, filename);
		envfs_save(filename, dirname);
	}
	exit(0);
}
