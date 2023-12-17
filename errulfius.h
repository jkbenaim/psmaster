#pragma once

#include <stdarg.h>
#include <stdnoreturn.h>
#include <ulfius.h>

void vwarnulfius(int u_rc, const char *fmt, va_list args);
noreturn void verrulfius(int eval, int u_rc, const char *fmt, va_list args);
void warnulfius(int u_rc, const char *fmt, ...);
noreturn void errulfius(int eval, int u_rc, const char *fmt, ...);

