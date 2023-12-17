#define _GNU_SOURCE
#include <inttypes.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include "endian.h"
#include "err.h"
#include "errsql.h"
#include "hexdump.h"
#include "mapfile.h"
#include "progname.h"
#include "version.h"

noreturn static void vtryhelp(const char *fmt, va_list args);
noreturn static void tryhelp(const char *fmt, ...);
noreturn static void usage(void);

int main(int argc, char *argv[])
{
	char *filename = NULL;
	int rc;
	
	while ((rc = getopt(argc, argv, "hf:")) != -1)
		switch (rc) {
		case 'h':
			usage();
		case 'f':
			if (filename)
				usage();
			filename = optarg;
			break;
		default:
			tryhelp(NULL);
		}
	argc -= optind;
	argv += optind;

#if 0
	if (not filename)
		usage();
	if (*argv != NULL)
		usage();
#else
	if (*argv != NULL) {
		filename = *argv;
	} else {
		tryhelp("must specify a file");
	}
#endif


	return EXIT_SUCCESS;
}

noreturn static void vtryhelp(const char *fmt, va_list args)
{
	if (fmt) {
		fprintf(stderr, "%s: ", __progname);
		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "Try `%s -h' for more information.\n", __progname);
	exit(EXIT_FAILURE);
}

noreturn static void tryhelp(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vtryhelp(fmt, ap);
	va_end(ap);
}

noreturn static void usage(void)
{
	(void)fprintf(stderr,
"Usage: %s [OPTION] [FILE]\n"
"Perform some action on FILE.\n"
"\n"
"  -h       print this help text\n"
"  -f FILE  use this file\n"
"\n"
"Please report any bugs to <" PROG_EMAIL ">.\n"
,		__progname
	);
	exit(EXIT_FAILURE);
}
