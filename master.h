#pragma once

#include <stdint.h>

enum master_disctype_e {
	MASTER_DISCTYPE_CD = 1,
	MASTER_DISCTYPE_DVD = 2,
};

enum master_region_e {
	MASTER_REGION_JAPAN = 1,
	MASTER_REGION_USA = 2,
	MASTER_REGION_EUROPE = 4,
	MASTER_REGION_CHINA = 8,
};

enum record_type_e {
	RECORD_TYPE_A = 0,
	RECORD_TYPE_B,
	RECORD_TYPE_C,
	RECORD_TYPE_D,
	RECORD_TYPE_E,
	RECORD_TYPE_LAST,
};

struct prodmagic_s {
	uint32_t magic2;
	char prodid[11];	// SLPM-12345
	uint8_t  magic1;
	uint8_t  magic3;
};

struct master_request_s {
	struct prodmagic_s prodmagic;
	char producer[33];
	char copyright[33];
	char date[9];
	enum master_region_e region;
	enum master_disctype_e disctype;
	size_t size_in_bytes;
	char burner[49];
	char generator[17];
};

struct record_s {
	uint8_t  a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint8_t  e;
	uint16_t f;
} __attribute__((packed));

struct master_data_s {
	char product_code[32];
	char producer[32];
	char copyright[32];
	char date[8];
	char id[24];	// "PlayStation Master Disc "
	char ps_type;	// always '2'
	uint8_t region;	// 2 for USA
	uint8_t flags;	// 00h: has ACE records, 10h: has ABCE records, 30h: has ABCD records
	uint8_t disctype;	// 1 = CD, 2 = DVD
	union __attribute__((packed)) {
		char cd_spaces[124];	// fill with 20h
		struct __attribute__((packed)) {
			uint8_t dvd_numlayers;
			uint8_t dvd_thislayer;
			uint32_t dvd_l0last;	// last sector on layer 0
			uint32_t dvd_l1last;	// last sector on layer 1
			char dvd_spaces[114];	// fill with 20h
		};
	};
	struct record_s rec[32];
	char burner[48];
	char generator[16];	// "CDVDGEN 1.20    "
	char unk828[1216];	// fill with 20h
} __attribute__((packed));

/*
 * what the mechacon actually checks:
 * "PlayStation Master Disc 2" - mandatory, but the character between
 * 	"Disc" and "2" is dontcare, lol.
 */

extern struct master_data_s make_master(struct master_request_s req);
extern void print_master(struct master_data_s m);
