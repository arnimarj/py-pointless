#ifndef __POINTLESS__DEBUG__H__
#define __POINTLESS__DEBUG__H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <pointless/bitutils.h>
#include <pointless/custom_sort.h>

#include <pointless/pointless_defs.h>
#include <pointless/pointless_reader.h>
#include <pointless/pointless_value.h>

int pointless_debug_print(pointless_t* p, FILE* out, const char** error);
int pointless_debug_print_value(pointless_t* p, pointless_value_t* v, FILE* out, const char** error);

#endif
