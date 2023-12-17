#pragma once

#include <stdarg.h>
#include <stdnoreturn.h>
#include <sqlite3.h>

void vwarnsql(sqlite3 *db, const char *fmt, va_list args);
noreturn void verrsql(int eval, sqlite3 *db, const char *fmt, va_list args);
void warnsql(sqlite3 *db, const char *fmt, ...);
noreturn void errsql(int eval, sqlite3 *db, const char *fmt, ...);

