#define _GNU_SOURCE
#include <inttypes.h>
#include <iso646.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include <cdio/iso9660.h>

#include "endian.h"
#include "err.h"
#include "hexdump.h"
#include "mapfile.h"
#include "master.h"
#include "progname.h"
#include "sha1.h"
#include "version.h"

noreturn static void vtryhelp(const char *fmt, va_list args);
noreturn static void tryhelp(const char *fmt, ...);
noreturn static void usage(void);

const size_t SYSTEMCNF_MAX_SIZE = 2048;

enum region_e {
	REGION_NTSC = 100,
	REGION_PAL,
};
struct logo_hash_s {
	enum region_e region;
	uint8_t digest[SHA1_DIGEST_SIZE];
} logo_hashes[] = {
	{
		.region = REGION_NTSC,
		.digest = {0xd4, 0x3f, 0x52, 0xb0, 0x5b, 0xb2, 0xf8, 0x2f,
		           0x42, 0xa1, 0x1d, 0x93, 0x2c, 0x49, 0xd9, 0x07,
			   0xba, 0x72, 0x64, 0xd0},
	},
	{
		.region = REGION_PAL,
		.digest = {0xfd, 0xf9, 0x34, 0xca, 0xdb, 0xf9, 0xe1, 0xae,
		           0x3c, 0xd7, 0x93, 0xb8, 0x28, 0x6e, 0x3a, 0xa4,
		           0x4e, 0x07, 0xb2, 0x0d},
	},
	{0,},
};

enum region_e get_region_for_hash(uint8_t digest[SHA1_DIGEST_SIZE])
{
	for (size_t i = 0; logo_hashes[i].region != 0; i++) {
		if (!memcmp(digest, logo_hashes[i].digest, SHA1_DIGEST_SIZE)) {
			return logo_hashes[i].region;
		}
	}
	return 0;
}

struct bootname_s {
	bool valid;
	uint32_t magic2;
	uint8_t  magic1;
	uint8_t  magic3;
	char prodcode[11];
};

struct bootname_s process_bootname(char *bootname)
{
	struct bootname_s retval = {0,};
	retval.valid = true;

	for (size_t i = 0; bootname[i]; i++) switch (i) {
		case 0 ... 3:
			if ((bootname[i] > 'Z') || (bootname[i] < 'A'))
				retval.valid = false;
			break;
		case 4:
			if (bootname[i] != '_')
				retval.valid = false;
			break;
		case 5 ... 7:
		case 9 ... 10:
			if ((bootname[i] > '9') || (bootname[i] < '0'))
				retval.valid = false;
			break;
		case 8:
			if (bootname[i] != '.')
				retval.valid = false;
			break;
		case 11:
			if (bootname[i] != ';')
				retval.valid = false;
			break;
		case 12:
			if (bootname[i] != '1')
				retval.valid = false;
			break;
		default:
			// too long
			retval.valid = false;
			return retval;
			break;
	}
	if (!retval.valid)
		return retval;
	
	char numstring[6];
	memcpy(numstring, bootname + 5, 3);
	memcpy(numstring + 3, bootname + 9, 2);
	numstring[5] = '\0';
	unsigned long num;
	num = strtoul(numstring, NULL, 10);

	unsigned letters;
	letters = (bootname[3]) | (bootname[2]<<7) | (bootname[1]<<14) | (bootname[0]<<21);

	retval.magic1 = ((num & 0x1f) << 3) | ((0x0fffffff & letters) >> 25);
	retval.magic2 = (num >> 10) | ((0x0fffffff & letters) << 7);
	retval.magic3 = ((num & 0x3e0) >> 2) | 0x04;

	memcpy(retval.prodcode, bootname, 4);
	retval.prodcode[4] = '-';
	memcpy(retval.prodcode + 5, bootname+5, 3);
	memcpy(retval.prodcode + 8, bootname+5, 2);
	retval.prodcode[10] = '\0';
	
	return retval;
}

int main(int argc, char *argv[])
{
	char *filename = NULL;
	int rc;
	char systemcnf[SYSTEMCNF_MAX_SIZE + 1];
	size_t systemcnf_size = 0;

	progname_init(argc, argv);
	
	while ((rc = getopt(argc, argv, "h")) != -1)
		switch (rc) {
		case 'h':
			usage();
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

	struct MappedFile_s m;
	m = MappedFile_Open(filename, false);
	if (!m.data)
		err(1, "couldn't open '%s' for reading", filename);
	
	//hexdump(m.data + 14 * 2048, 2048);
	struct master_data_s md = {0,};
	memcpy(&md, m.data + 14*2048, 2048);
	
	iso9660_t *iso;
	iso = iso9660_open(filename);
	if (!iso)
		err(1, "couldn't open '%s' for reading", filename);
	
	iso9660_stat_t *s;
	s = iso9660_ifs_stat_translate(iso, "SYSTEM.CNF;1");
	if (!s)
		errx(1, "couldn't stat file in image: '%s'", "SYSTEM.CNF;1");
	
	//hexdump(m.data + 2048 * (size_t)s->lsn, s->size);
	memcpy(systemcnf, m.data + 2048 * (size_t)s->lsn, SYSTEMCNF_MAX_SIZE);
	systemcnf_size = s->size;
	if (systemcnf_size > SYSTEMCNF_MAX_SIZE)
		systemcnf_size = SYSTEMCNF_MAX_SIZE;
	systemcnf[systemcnf_size + 1] = '\0';

	char *key, *val;
	key = val = NULL;
	rc = sscanf(systemcnf, "%ms = %ms", &key, &val);
	if (rc != 2)
		warnx("sscanf failure %d", rc);
	if (strcmp(key, "BOOT2"))
		errx(1, "first key in system.cnf was not boot2");
	free(key);
	key = NULL;

	char *exename = NULL;
	rc = sscanf(val, "cdrom0:\\%ms;1", &exename);
	if (rc != 1)
		warnx("sscanf failure %d", rc);
	free(val);
	val = NULL;

	// validate exename
	struct bootname_s bn;
	bn = process_bootname(exename);
	if (!bn.valid)
		errx(1, "invalid bootname '%s'", exename);
	
	free(exename);
	exename = NULL;

	// validate logo
	uint8_t logo[2048 * 12];
	memcpy(logo, m.data, 2048 * 12);
	for (size_t i = 0; i < 2048*12; i++) {
		logo[i] ^= bn.magic1;
		logo[i] = ((logo[i] >> 5) | (logo[i] << 3));
	}

	SHA1_CTX sha1;
	uint8_t digest[SHA1_DIGEST_SIZE];

	SHA1_Init(&sha1);
	SHA1_Update(&sha1, logo, 12*2048);
	SHA1_Final(&sha1, digest);

	enum region_e region;
	region = get_region_for_hash(digest);
	if (region == 0) {
		printf("digest:\n");
		hexdump(digest, SHA1_DIGEST_SIZE);
		errx(1, "bad logo digest");
	}

	iso9660_stat_free(s);
	s = NULL;

	iso9660_close(iso);
	iso = NULL;
	m = MappedFile_Close(m);

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
