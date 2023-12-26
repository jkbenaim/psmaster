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
	SCANFOR_VAL_OTHER,
} scanfor = SCANFOR_KEY;

char *boot2 = NULL;

%}
%%
[\t ]+

[a-zA-Z0-9;\\:_.]* {
	switch (scanfor) {
	case SCANFOR_KEY:
		if (!strcmp(yytext, "BOOT2")) {
			scanfor = SCANFOR_VAL_BOOT2;
		}
		break;
	case SCANFOR_VAL_BOOT2:
		if (!boot2)
			boot2 = strdup(yytext);
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

char *get_boot2_from_systemcnf(void *buf, size_t siz)
{
	FILE *f = NULL;

	f = fmemopen(buf, siz, "r");
	if (!f)
		err(1, "couldn't open file");

	boot2 = NULL;

	yyin = f;
	yylex();

	fclose(f);
	f = NULL;
	yylex_destroy();
	return boot2;
}
