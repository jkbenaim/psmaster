#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <ulfius.h>
#include "errulfius.h"

extern const char *__progname;

const char *_ulfius_errname(int rc)
{
#define _g(errcode) case errcode: return #errcode; break;
	switch (rc) {
		_g(U_OK)
		_g(U_ERROR)
		_g(U_ERROR_MEMORY)
		_g(U_ERROR_PARAMS)
		_g(U_ERROR_LIBMHD)
		_g(U_ERROR_LIBCURL)
		_g(U_ERROR_NOT_FOUND)
	default: return "(unknown)"; break;
	}
#undef _g
}

void vwarnulfius(int u_rc, const char *fmt, va_list args)
{
	fprintf(stderr, "%s: ", __progname);
	if (fmt) {
		vfprintf(stderr, fmt, args);
		fprintf(stderr, ": ");
	}
	fprintf(stderr, "%s\n", _ulfius_errname(u_rc));
}

noreturn void verrulfius(int eval, int u_rc, const char *fmt, va_list args)
{
	vwarnulfius(u_rc, fmt, args);
	exit(eval);
}

void warnulfius(int u_rc, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vwarnulfius(u_rc, fmt, ap);
	va_end(ap);
}

noreturn void errulfius(int eval, int u_rc, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verrulfius(eval, u_rc, fmt, ap);
	va_end(ap);
}

