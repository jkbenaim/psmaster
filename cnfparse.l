%option noyywrap
%{
#include "cnfparse.h"
#include "endian.h"
#include "err.h"
#include <stdio.h>
#include <stdlib.h>

enum scanfor_e {
	SCANFOR_KEY,
	SCANFOR_VAL_BOOT2,
	SCANFOR_VAL_VER,
	SCANFOR_VAL_VMODE,
	SCANFOR_VAL_HDDUNITPOWER,
	SCANFOR_VAL_OTHER,
} scanfor = SCANFOR_KEY;

struct systemcnf_s cnf = {0,};

%}
%%
[\t ]+

[a-zA-Z0-9;\\:_.]* {
	switch (scanfor) {
	case SCANFOR_KEY:
		if (!strcmp(yytext, "BOOT2")) {
			scanfor = SCANFOR_VAL_BOOT2;
		} else if (!strcmp(yytext, "VER")) {
			scanfor = SCANFOR_VAL_VER;
		} else if (!strcmp(yytext, "VMODE")) {
			scanfor = SCANFOR_VAL_VMODE;
		} else if (!strcmp(yytext, "HDDUNITPOWER")) {
			scanfor = SCANFOR_VAL_HDDUNITPOWER;
		} else {
			scanfor = SCANFOR_VAL_OTHER;
		}
		break;
	case SCANFOR_VAL_BOOT2:
		if (!cnf.boot2)
			cnf.boot2 = strdup(yytext);
		break;
	case SCANFOR_VAL_VER:
		if (!cnf.ver)
			cnf.ver = strdup(yytext);
		break;
	case SCANFOR_VAL_VMODE:
		if (!cnf.vmode)
			cnf.vmode = strdup(yytext);
		break;
	case SCANFOR_VAL_HDDUNITPOWER:
		if (!cnf.hddunitpower)
			cnf.hddunitpower = strdup(yytext);
		break;
	case SCANFOR_VAL_OTHER:
		/* fall thru */
	default:
		break;
	}
}

= {
	if (scanfor == SCANFOR_KEY) {
		scanfor = SCANFOR_VAL_OTHER;
	}
}

. {
	printf("unknown thing: '%s'\n", yytext);
}

[\r\n]+ {
	scanfor = SCANFOR_KEY;
}
%%

struct systemcnf_s parse_systemcnf(void *buf, size_t siz)
{

	FILE *f = NULL;

	f = fmemopen(buf, siz, "r");
	if (!f)
		err(1, "couldn't open file");

	yyin = f;
	yylex();

	fclose(f);
	f = NULL;
	yylex_destroy();

	return cnf;
}

void free_systemcnf(struct systemcnf_s cnf)
{
	free(cnf.boot2);
	free(cnf.ver);
	free(cnf.vmode);
	free(cnf.hddunitpower);
}
