#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "endian.h"
#include "err.h"
#include "hexdump.h"
#include "master.h"
#include "version.h"

#define ARRAY_SIZE(x) sizeof(x)/(sizeof(*x))

const char *region_to_string(enum master_region_e r)
{
	switch (r) {
	case MASTER_REGION_JAPAN:
		return "Japan";
	case MASTER_REGION_USA:
		return "USA";
	case MASTER_REGION_EUROPE:
		return "Europe";
	case MASTER_REGION_WORLD:
		return "World";
	default:
		return "unknown";
	}
}

const char *disctype_to_string(enum master_disctype_e t)
{
	switch (t) {
	case MASTER_DISCTYPE_CD:
		return "CD";
	case MASTER_DISCTYPE_DVD:
		return "DVD";
	default:
		return "unknown";
	}
}

void print_rec(int record_num, struct record_s rec)
{
	if (rec.a || rec.b || rec.c || rec.d || rec.e) {
		printf("record %d:\n", record_num);
		printf("\ta: \t%8x h\n", rec.a);
		printf("\tb: \t%8x h\n", le32toh(rec.b));
		printf("\tc: \t%8x h\n", le32toh(rec.c));
		printf("\td: \t%8x h\n", le32toh(rec.d));
		printf("\te: \t%8x h\n", rec.e);
		printf("\tf: \t%8x h\n", le16toh(rec.f));
	}
}

// Copy a string without the null terminator, and fill the rest of
// the buffer with spaces.
char *md_strncpy(char *dst, char *src, size_t n)
{
	while (*src && n--) {
		*dst++ = *src++;
	}
	while (n--) {
		*dst++ = ' ';
	}
	return dst;
}

struct master_data_s make_master(struct master_request_s req)
{
	struct master_data_s md;
	memset(&md, 0, sizeof(md));

	// default values
	if (!strlen(req.producer)) {
		strcpy(req.producer, "JRRA");
	}
	if (!strlen(req.copyright)) {
		strcpy(req.copyright, "JRRA");
	}
	if (!strlen(req.date)) {
		time_t t;
		struct tm *tm;
		t = time(NULL);
		tm = localtime(&t);
		if (tm == NULL)
			err(1, "localtime");
		strftime(req.date, sizeof(req.date), "%Y%m%d", tm);
	}
	if (req.region == 0) {
		req.region = MASTER_REGION_JAPAN;
	}
	if (!strlen(req.burner)) {
		strcpy(req.burner, PROG_WEBSITE);
	}
	if (!strlen(req.generator)) {
		strcpy(req.generator, "CDVDGEN 1.20");
	}

	// fill master data
	md_strncpy(md.product_code, req.prodmagic.prodid, sizeof(md.product_code));
	md_strncpy(md.producer, req.producer, sizeof(md.producer));
	md_strncpy(md.copyright, req.copyright, sizeof(md.copyright));
	md_strncpy(md.date, req.date, sizeof(md.date));
	md_strncpy(md.id, "PlayStation Master Disc", sizeof(md.id));
	md.ps_type = '2';
	md.region = req.region;
	md.flags = 0;
	md.disctype = req.disctype;
	switch (req.disctype) {
	case MASTER_DISCTYPE_CD:
		memset(md.cd_spaces, ' ', sizeof(md.cd_spaces));
		break;
	case MASTER_DISCTYPE_DVD:
		md.dvd_numlayers = 1;
		md.dvd_thislayer = 0;
		md.dvd_l0last = htole32((((req.size_in_bytes / 2048) + 15) & ~15) - 1);
		md.dvd_l1last = 0;
		memset(md.dvd_spaces, ' ', sizeof(md.dvd_spaces));
		break;
	default:
		errx(1, "bad disctype");
	}

	unsigned num = 0;
	// create records
	// record A
	md.rec[num].a = 1;
	md.rec[num].b = htole32(0xffffffff);
	md.rec[num].c = htole32(0xffffffff);
	md.rec[num].d = htole32(req.prodmagic.magic2);
	md.rec[num].e = req.prodmagic.magic1;
	md.rec[num].f = htole16(0);
	num++;
	// record B
	if (md.flags & 0x10) {
		md.rec[num].a = 2;
		md.rec[num].b = htole32(0xffffffff);
		md.rec[num].c = htole32(0xffffffff);
		md.rec[num].d = htole32(0);
		md.rec[num].e = 0x80;
		md.rec[num].f = htole16(0);
		num++;
	}
	// record C
	md.rec[num].a = 1;
	md.rec[num].b = htole32(0x4b);
	md.rec[num].c = htole32(0x14a);
	md.rec[num].d = htole32(req.prodmagic.magic2);
	md.rec[num].e = req.prodmagic.magic1;
	md.rec[num].f = htole16(0);
	num++;
	// record D or E
	md.rec[num].a = 3;
	md.rec[num].b = htole32(0x4b);
	md.rec[num].c = htole32(0x104a);
	md.rec[num].d = htole32(0);
	md.rec[num].e = req.prodmagic.magic3;
	if (md.flags & 0x20) {
		// record D
		md.rec[num].f = htole16(0x8000);
	} else {
		// record E
		md.rec[num].f = htole16(0);
	}
	num++;

	md_strncpy(md.burner, req.burner, sizeof(md.burner));
	md_strncpy(md.generator, req.generator, sizeof(md.generator));
	memset(md.unk828, ' ', sizeof(md.unk828));

	return md;
}

void print_master(struct master_data_s m)
{
	char buf[4096];
	memcpy(buf, m.product_code, sizeof(m.product_code));
	buf[32] = '\0';
	printf("product_code:  \t'%32s'\n", buf);
	memcpy(buf, m.producer, sizeof(m.producer));
	printf("producer:  \t'%32s'\n", buf);
	memcpy(buf, m.copyright, sizeof(m.copyright));
	printf("copyright:  \t'%32s'\n", buf);
	
	memset(buf, '\0', sizeof(buf));
	memcpy(buf, m.date, sizeof(m.date));
	printf("date:  \t\t'%s'\n", buf);

	memset(buf, '\0', sizeof(buf));
	memcpy(buf, m.id, sizeof(m.id));
	printf("id:  \t\t'%s'\n", buf);

	printf("ps_type: \t'%c'\n", m.ps_type);
	printf("region: \t%xh (%s)\n", m.region, region_to_string(m.region));
	printf("flags: \t\t%xh\n", m.flags);
	printf("disctype:\t%xh (%s)\n", m.disctype, disctype_to_string(m.disctype));

	bool all_spaces = true;
	switch (m.disctype) {
	case MASTER_DISCTYPE_CD:
		for (size_t i = 0; i < sizeof(m.cd_spaces); i++) {
			if (m.cd_spaces[i] != ' ')
				all_spaces = false;
		}
		if (all_spaces) {
			printf("cd region filled with all spaces (ok)\n");
		} else {
			printf("cd region NOT filled with all spaces (bad)\n");
			hexdump(m.cd_spaces, sizeof(m.cd_spaces));
		}
		break;
	case MASTER_DISCTYPE_DVD:
		printf("dvd_numlayers: \t%xh\n", m.dvd_numlayers);
		printf("dvd_thislayer: \t%xh\n", m.dvd_thislayer);
		printf("dvd_l0last: \t%xh\n", le32toh(m.dvd_l0last));
		printf("dvd_l1last: \t%xh\n", le32toh(m.dvd_l1last));
		for (size_t i = 0; i < sizeof(m.dvd_spaces); i++) {
			if (m.dvd_spaces[i] != ' ')
				all_spaces = false;
		}
		if (all_spaces) {
			printf("dvd pad region filled with all spaces (ok)\n");
		} else {
			printf("dvd pad region NOT filled with all spaces (bad)\n");
			hexdump(m.dvd_spaces, sizeof(m.dvd_spaces));
		}
		break;
	default:
		printf("invalid disc type\n");
		break;
	}

	for (int idx = 0; idx < ARRAY_SIZE(m.rec); idx++) {
		print_rec(idx, m.rec[idx]);
	}

	// burner
	memset(buf, '\0', sizeof(buf));
	memcpy(buf, m.burner, sizeof(m.burner));
	printf("burner: \t'%s'\n", buf);

	// generator
	memset(buf, '\0', sizeof(buf));
	memcpy(buf, m.generator, sizeof(m.generator));
	printf("generator: \t'%s'\n", buf);

	// unk828
	all_spaces = true;
	for (size_t i = 0; i < sizeof(m.unk828); i++) {
		if (m.unk828[i] != ' ')
			all_spaces = false;
	}
	if (all_spaces) {
		printf("unk828: \tall spaces (ok)\n");
	} else {
		printf("unk828: \tNOT all spaces (bad)\n");
		//hexdump(m.unk828, sizeof(m.unk828));
	}
}
