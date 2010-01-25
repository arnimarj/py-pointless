#ifndef __POINTLESS__READER__H__
#define __POINTLESS__READER__H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

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

int pointless_open_f(pointless_t* p, const char* fname, const char** error);
void pointless_close(pointless_t* p);

#endif
