#ifndef __POINTLESS__READER__H__
#define __POINTLESS__READER__H__

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

#include <sys/mman.h>
#include <sys/stat.h>

#include <pointless/bitutils.h>
#include <pointless/pointless_defs.h>
#include <pointless/pointless_value.h>
#include <pointless/pointless_unicode_utils.h>
#include <pointless/pointless_bitvector.h>
#include <pointless/pointless_hash_table.h>
#include <pointless/pointless_validate.h>
#include <pointless/pointless_reader_utils.h>

int pointless_open_f(pointless_t* p, const char* fname, int force_ucs2, const char** error);
int pointless_open_b(pointless_t* p, const void* buffer, size_t n_buffer, int force_ucs2, const char** error);

// use these two with care. they don't perform any validation on the underlying data, which may cause
// segfaults when accessed through the normal pointless-library functions
int pointless_open_f_skip_validate(pointless_t* p, const char* fname, int force_ucs2, const char** error);
int pointless_open_b_skip_validate(pointless_t* p, const void* buffer, size_t n_buffer, int force_ucs2, const char** error);

void pointless_close(pointless_t* p);

#endif
