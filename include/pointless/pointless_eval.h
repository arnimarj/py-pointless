#ifndef __POINTLESS__EVAL__H__
#define __POINTLESS__EVAL__H__

#include <pointless/pointless_defs.h>
#include <pointless/pointless_reader_utils.h>
#include <pointless/pointless_reader_helpers.h>

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

// a simple utility to evaluate simple expressions on arbitrary pointless-values
//
// it supports map/vector/bitvector values, indexed using integers or strings
//
// example: ["some_key_in_map"][10][10]
//
// it can also fill in values, printf-style supporting these format specifiers:
//   %s:    const char*
//   %u32:  uint32_t
//   %i32:  int32_t
//   %u64:  uint64_t
//   %i64:  int64_t
int pointless_eval_get(pointless_t* p, pointless_value_t* root, pointless_value_t* v, const char* e, ...);

#endif
