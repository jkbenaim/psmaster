#pragma once

#include <stdint.h>

#define LOGO_SIZE (12*2048)

#ifdef __MINGW32__

// On Windows we pretend that the symbols here don't have a leading underscore,
// even though they do. The linker will add them for some reason.
extern const size_t    binary_data_logo_ntsc_bin_size;
extern const uint8_t   binary_data_logo_ntsc_bin_start[LOGO_SIZE];
extern const size_t    binary_data_logo_pal_bin_size;
extern const uint8_t   binary_data_logo_pal_bin_start[LOGO_SIZE];
#define logo_ntsc_size ((size_t) &binary_data_logo_ntsc_bin_size)
#define logo_ntsc      binary_data_logo_ntsc_bin_start
#define logo_pal_size  ((size_t) &binary_data_logo_pal_bin_size)
#define logo_pal       binary_data_logo_pal_bin_start

#else

extern const size_t    _binary_data_logo_ntsc_bin_size;
extern const uint8_t   _binary_data_logo_ntsc_bin_start[LOGO_SIZE];
extern const size_t    _binary_data_logo_pal_bin_size;
extern const uint8_t   _binary_data_logo_pal_bin_start[LOGO_SIZE];
#define logo_ntsc_size ((size_t) &_binary_data_logo_ntsc_bin_size)
#define logo_ntsc      _binary_data_logo_ntsc_bin_start
#define logo_pal_size  ((size_t) &_binary_data_logo_pal_bin_size)
#define logo_pal       _binary_data_logo_pal_bin_start

#endif
