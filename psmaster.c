#define _GNU_SOURCE
#include <inttypes.h>
#include <iso646.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include <cdio/iso9660.h>
#include <cdio/udf.h>

#include "data.h"
#include "cnfparse.h"
#include "endian.h"
#include "err.h"
#include "mapfile.h"
#include "master.h"
#include "progname.h"
#include "version.h"

#define DEFAULT_PRODUCT_CODE "SLPM-00000"

noreturn static void vtryhelp(const char *fmt, va_list args);
noreturn static void tryhelp(const char *fmt, ...);
noreturn static void usage(void);

const size_t SYSTEMCNF_MAX_SIZE = ISO_BLOCKSIZE;

enum logo_region_e {
	LOGO_REGION_NTSC = 100,
	LOGO_REGION_PAL,
};

struct prodmagic_s get_magic_for_prodid(char *prodid)
{
	struct prodmagic_s retval = {0,};

	strncpy(retval.prodid, prodid, sizeof(retval.prodid));
	retval.prodid[sizeof(retval.prodid)-1] = '\0';
	
	unsigned long num;
	num = strtoul(prodid + 5, NULL, 10);

	unsigned letters;
	letters = (prodid[3]) | (prodid[2]<<7) | (prodid[1]<<14) | (prodid[0]<<21);

	retval.magic1 = ((num & 0x1f) << 3) | ((0x0fffffff & letters) >> 25);
	retval.magic2 = (num >> 10) | ((0x0fffffff & letters) << 7);
	retval.magic3 = ((num & 0x3e0) >> 2) | 0x04;

	return retval;
}

// returns cnf for a given ISO filename
struct systemcnf_s get_systemcnf(const char *filename)
{
	__label__ out_ok, out_error;

	struct systemcnf_s cnf = {0,};
	char systemcnf[SYSTEMCNF_MAX_SIZE + 1];
	size_t systemcnf_size = 0;
	iso9660_t *iso = NULL;
	iso9660_stat_t *s = NULL;
	long int lrc;
	char *zErr = NULL;

	iso = iso9660_open(filename);
	if (!iso) {
		zErr = "couldn't open file";
		goto out_error;
	}
	
	s = iso9660_ifs_stat_translate(iso, "SYSTEM.CNF;1");
	if (!s) {
		zErr = "couldn't find system.cnf";
		goto out_error;
	}
	
	lrc = iso9660_iso_seek_read(iso, systemcnf, s->lsn, 1);
	if (lrc != ISO_BLOCKSIZE) {
		zErr = "while reading from system.cnf";
		goto out_error;
	}
	
	systemcnf_size = s->size;
	if (systemcnf_size > SYSTEMCNF_MAX_SIZE)
		systemcnf_size = SYSTEMCNF_MAX_SIZE;
	systemcnf[systemcnf_size + 1] = '\0';

	cnf = parse_systemcnf(systemcnf, systemcnf_size);

	goto out_ok;

out_error:
	if (zErr) {
		warnx("in get_systemcnf: %s", zErr);
	}
out_ok:
	if (s)
		iso9660_stat_free(s);
	if (iso)
		iso9660_close(iso);
	return cnf;
}

// returns the product id (SCPH-12345) or NULL given an ISO filename
char *get_prodid_from_boot2(const char *bootname)
{
	__label__ out_ok, out_error;

	char *prodid = NULL;
	char *zErr = NULL;

	// validate bootname
	bool bootname_valid = true;
	bool bootname_long_enough = false;
	// cdrom0:\SLPM_123.45;1
	// 012345678901234567890
	for (size_t i = 0; bootname[i]; i++) switch (i) {
		case 0 ... 7:
			if (bootname[i] != "cdrom0:\\"[i]) {
				bootname_valid = false;
			}
			break;
		case 8 ... 11:
			if ((bootname[i] > 'Z') || (bootname[i] < 'A'))
				bootname_valid = false;
			break;
		case 12:
			if (bootname[i] != '_')
				bootname_valid = false;
			break;
		case 13 ... 15:
		case 17 ... 18:
			if ((bootname[i] > '9') || (bootname[i] < '0'))
				bootname_valid = false;
			break;
		case 16:
			if (bootname[i] != '.')
				bootname_valid = false;
			break;
		case 19:
			if (bootname[i] != ';')
				bootname_valid = false;
			break;
		case 20:
			if (bootname[i] != '1') {
				bootname_valid = false;
			}
				bootname_long_enough = true;
			break;
		default:
			// too long
			bootname_valid = false;
			break;
	}

	if (!bootname_long_enough) {
		zErr = "bootname too short";
		goto out_error;
	}

	if (!bootname_valid) {
		printf("'%s'\n", bootname);
		zErr = "bootname invalid";
		goto out_error;
	}

	prodid = malloc(11);
	if (!prodid) {
		zErr = "in malloc";
		goto out_error;
	}

	// copy "cdrom0:\SCPH_123.45;1" --> "SCPH-12345"
	prodid[0] = bootname[0+8];
	prodid[1] = bootname[1+8];
	prodid[2] = bootname[2+8];
	prodid[3] = bootname[3+8];
	prodid[4] = '-';
	prodid[5] = bootname[5+8];
	prodid[6] = bootname[6+8];
	prodid[7] = bootname[7+8];
	prodid[8] = bootname[9+8];
	prodid[9] = bootname[10+8];
	prodid[10] = '\0';

	goto out_ok;

out_error:
	if (zErr) {
		warnx("in get_prodid_from_bootname: %s", zErr);
	}
	free(prodid);
	prodid = NULL;
out_ok:
	return prodid;
}

void validate_structs()
{
	// ensure structs have not been defiled
	bool everything_ok = true;
	if (sizeof(struct record_s) != 16) {
		printf("sizeof(struct record_s) is %zu, should be %zu\n",
			sizeof(struct record_s),
			(size_t)16
		);
		everything_ok = false;
	}
	if (sizeof(struct master_data_s) != 2048) {
		printf("sizeof(struct master_data_s) is %zu, should be %zu\n",
			sizeof(struct master_data_s),
			(size_t)2048
		);
		everything_ok = false;
	}

	if (logo_ntsc_size != LOGO_SIZE) {
		printf("logo_ntsc_size != LOGO_SIZE\n");
		everything_ok = false;
	}
	if (logo_pal_size != LOGO_SIZE) {
		printf("logo_pal_size != LOGO_SIZE\n");
		everything_ok = false;
	}
	
	if (!everything_ok)
		exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	char *filename = NULL;
	int rc;

	progname_init(argc, argv);
	validate_structs();

	enum mode_e {
		MODE_DEFAULT,
		MODE_LIST,
		MODE_BLESS,
	} mode = MODE_DEFAULT;

	char *force_productcode = NULL;
	char *force_date = NULL;
	char *force_producer = NULL;
	long region = 0;

	while ((rc = getopt(argc, argv, "bhld:p:P:r:V")) != -1)
		switch (rc) {
		case 'h':
			usage();
		case 'b':
			if (mode != MODE_DEFAULT)
				tryhelp("too many modes specified");
			mode = MODE_BLESS;
			break;
		case 'l':
			if (mode != MODE_DEFAULT)
				tryhelp("too many modes specified");
			mode = MODE_LIST;
			break;
		case 'd':
			if (force_date)
				tryhelp("too many dates specified");
			force_date = optarg;
			break;
		case 'p':
			if (force_productcode)
				tryhelp("too many product codes specified");
			force_productcode = optarg;
			break;
		case 'P':
			if (force_producer)
				tryhelp("too many producers specified");
			force_producer = optarg;
			break;
		case 'r':
			if (region != 0)
				tryhelp("too many regions specified");
			region = strtol(optarg, NULL, 10);
			switch (region) {
			case LONG_MIN:
			case LONG_MAX:
			case 0:
				errx(1, "invalid region number: %ld", region);
			default:
				break;
			}
			break;
		case 'V':
			fprintf(stderr, "%s\n", PROG_VERSION);
			exit(0);
			break;
		default:
			tryhelp(NULL);
		}
	argc -= optind;
	argv += optind;

	if (*argv != NULL) {
		filename = *argv;
	} else {
		tryhelp("must specify a file");
	}

	// no mode specified?
	if (mode == MODE_DEFAULT)
		mode = MODE_LIST;

	// validate args
	switch (mode) {
	case MODE_LIST:
		if (force_date || force_productcode || force_producer || region)
			errx(1, "can't specify options along with -l");
		break;
	default:
		break;
	}

	// open file
	struct MappedFile_s m;
	m = MappedFile_Open(filename, (mode == MODE_BLESS)?true:false);
	if (!m.data)
		err(1, "couldn't open '%s' for reading", filename);
	
	// isolate existing master data, if any
	struct master_data_s md = {0,};
	memcpy(&md, m.data + 14*2048, 2048);
	if (mode == MODE_LIST)
		print_master(md);
	
	// get cnf
	struct systemcnf_s cnf;
	cnf = get_systemcnf(filename);

	// get prodid
	char *prodid = NULL;
	if (force_productcode) {
		prodid = strdup(force_productcode);
	} else {
		prodid = get_prodid_from_boot2(cnf.boot2);
		if (!prodid) {
			prodid = strdup(DEFAULT_PRODUCT_CODE);
		}
	}
	if (!prodid)
		err(1, "in strdup");

	// get magic values for prodid
	struct prodmagic_s prodmagic = {0,};
	prodmagic = get_magic_for_prodid(prodid);
	free(prodid);
	prodid = NULL;

	if (mode == MODE_LIST) {
		// print magic values
		printf("magic1: %02xh\n", prodmagic.magic1);
		printf("magic2: %04xh\n", prodmagic.magic2);
		printf("magic3: %02xh\n", prodmagic.magic3);

		// validate logo
		uint8_t logo[2048 * 12];
		memcpy(logo, m.data, 2048 * 12);
		for (size_t i = 0; i < 2048*12; i++) {
			logo[i] ^= prodmagic.magic1;
			logo[i] = ((logo[i] >> 5) | (logo[i] << 3));
		}

		if (!memcmp(logo_ntsc, logo, sizeof(logo))) {
			printf("good logo (ntsc)\n");
		} else if (!memcmp(logo_pal,  logo, sizeof(logo))) {
			printf("good logo (pal)\n");
		} else {
			printf("logo is bad\n");
		}
	} else if (mode == MODE_BLESS) {
		// master data
		struct master_data_s md;
		struct master_request_s req = {0,};
		req.prodmagic = prodmagic;
		if (force_producer)
			strncpy(req.producer, force_producer, sizeof(req.producer));
		if (force_date)
			strncpy(req.date, force_date, sizeof(req.date));
		if (region) {
			req.region = region;
		} else if (!strcmp(cnf.vmode, "NTSC")) {
			req.region = MASTER_REGION_JAPAN;
		} else if (!strcmp(cnf.vmode, "PAL")) {
			req.region = MASTER_REGION_EUROPE;
		} else {
			req.region = MASTER_REGION_JAPAN;
		}

		// CD or DVD?
		// We try to open it as UDF, and if that succeeds, we say it's a DVD.
		req.disctype = MASTER_DISCTYPE_CD;
		udf_t *udf;
		udf = udf_open(filename);
		if (udf) {
			req.disctype = MASTER_DISCTYPE_DVD;
			udf_close(udf);
			udf = NULL;
		}
		req.size_in_bytes = m.size;

		md = make_master(req);
		memcpy(m.data + 14 * ISO_BLOCKSIZE, &md, sizeof(md));
		memcpy(m.data + 15 * ISO_BLOCKSIZE, &md, sizeof(md));
		
		// logo
		uint8_t logo[LOGO_SIZE];
		switch (req.region) {
		case MASTER_REGION_EUROPE:
			memcpy(logo, logo_pal, LOGO_SIZE);
			break;
		default:
			memcpy(logo, logo_ntsc, LOGO_SIZE);
			break;
		}
		// logo shift
		for (size_t i = 0; i < LOGO_SIZE; i++) {
			logo[i] = ((logo[i] << 5) | (logo[i] >> 3));
		}
		// logo xor
		for (size_t i = 0; i < LOGO_SIZE; i++) {
			logo[i] ^= prodmagic.magic1;
		}
		memcpy(m.data, logo, LOGO_SIZE);

	}

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
"Given a PlayStation 2 disc image, show or create its master data.\n"
"\n"
"  -h       print this help text\n"
"  -l       print master data from FILE\n"
"  -b       create new master data in FILE\n"
"  -d DATE  use DATE as date (format: YYYYMMDD)\n"
"  -p PCODE use PCODE as product code (format: ABCD-12345)\n"
"  -P NAME  use NAME as producer\n"
"  -r NUM   set disc region number. valid regions:\n"
"               1 - Japan\n"
"               2 - USA\n"
"               4 - Europe\n"
"               8 - China\n"
"\n"
"Please report any bugs to <" PROG_EMAIL ">.\n"
,		__progname
	);
	exit(EXIT_FAILURE);
}
