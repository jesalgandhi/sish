#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

void
usage()
{
	printf("usage: %s [-x] [-c command]\n", getprogname());
}

void
get_options(int argc, char **argv)
{
	int ch;
	extern char *optarg;

	tracing = false;
	c_cmd = NULL;

	while ((ch = getopt(argc, argv, "xc:")) != -1) {
		switch (ch) {
		case 'x':
			tracing = true;
			break;
		case 'c':
			c_cmd = optarg;
			break;
		case '?':
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}
}
