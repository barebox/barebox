
char *basename (char *path)
{
	char *fname;

	fname = path + strlen(path) - 1;
	while (fname >= path) {
		if (*fname == '/') {
			fname++;
			break;
		}
		fname--;
	}
	return fname;
}

char *dirname (char *path)
{
	char *fname;

	fname = basename (path);
	--fname;
	*fname = '\0';
	return path;
}

