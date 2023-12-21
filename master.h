#pragma once

#include <stdint.h>

enum master_pstype_e {
	MASTER_PSTYPE_PS1 = '1',
	MASTER_PSTYPE_PS2 = '2',
};

enum master_region_e {
	MASTER_REGION_NONE = 0,
	MASTER_REGION_JAPAN = 1,
	MASTER_REGION_USA = 2,
	MASTER_REGION_EUROPE = 4,
	MASTER_REGION_WORLD = 7,
};

__attribute__((packed))
struct master_data_s {
	char product_code[32];
	char producer[32];
	char copyright[32];
	char year[4];	// BCD
	char month[2];	// BCD
	char day[2];	// BCD
	char magic[24];	// "PlayStation Master Disc 2"
	uint8_t region;	// 2 for USA
	uint8_t unk130;	// should be 00h for world or 20h for japan?
	uint8_t disc_type;	// 1 = CD, 2 = DVD
	union {
		char cd_spaces[124];	// fill with 20h
		struct {
			uint8_t dvd_unk132;	// should be 1
			uint8_t dvd_unk133;	// should be 0
			uint32_t dvd_size;	// number of sectors, rounded up to 16, then subtract 1
			uint32_t dvd_unk138;	// should be 0
			char dvd_spaces[114];	// fill with 20h
		};
	};
	uint8_t  unk256;	// should be 1
	uint64_t unk257;	// should be FFFFFFFFFFFFFFFFh
	uint32_t magic2;
	uint8_t  magic1;
	uint16_t unk270;	// should be 0
	uint8_t  unk272;	// should be 1 for world or 2 for japan?
	uint32_t unk273;	// should be 4Bh
	uint32_t unk277;	// should be 104Ah
	uint32_t magic2_2;
	uint8_t  magic1_2;
	uint16_t unk286;	// should be 0
	uint8_t  unk288;	// should be 3
	uint32_t unk289;	// should be 4Bh
	uint32_t unk293;	// should be 104Ah
	uint32_t unk297;	// should be 0
	uint8_t  magic3;
	uint16_t unk302;	// should be 0
	uint32_t unk304;	// should be 0
	uint32_t unk308;	// should be 0
	uint32_t unk312;	// should be 0
	uint32_t unk316;	// should be 0
	uint8_t  unk320[432];	// fill with 00h
	char unk768[48];	// fill with 20h
	char generator[16];	// "CDVDGEN 1.20    "
	char unk828[1215];	// fill with 20h
};

/*
 * what the mechacon actually checks:
 * "PlayStation Master Disc 2" - mandatory, but the character between
 * 	"Disc" and "2" is dontcare, lol.
 */
