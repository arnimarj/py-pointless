#ifndef __POINTLESS__EVAL__H__
#define __POINTLESS__EVAL__H__

#include <pointless/pointless_defs.h>
#include <pointless/pointless_reader_utils.h>
#include <pointless/pointless_reader_helpers.h>

#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

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

// and convencience functions
int pointless_eval_get_as_u32(pointless_t* p, pointless_value_t* root, uint32_t* v, const char* e, ...);
int pointless_eval_get_as_map(pointless_t* p, pointless_value_t* root, pointless_value_t* v, const char* e, ...);
int pointless_eval_get_as_string(pointless_t* p, pointless_value_t* root, uint8_t** v, const char* e, ...);
int pointless_eval_get_as_vector_u8(pointless_t* p, pointless_value_t* root, uint8_t** v, uint32_t* n, const char* e, ...);
int pointless_eval_get_as_vector_u16(pointless_t* p, pointless_value_t* root, uint16_t** v, uint32_t* n, const char* e, ...);
int pointless_eval_get_as_vector_u32(pointless_t* p, pointless_value_t* root, uint32_t** v, uint32_t* n, const char* e, ...);
int pointless_eval_get_as_vector_u64(pointless_t* p, pointless_value_t* root, uint64_t** v, uint32_t* n, const char* e, ...);
int pointless_eval_get_as_vector_f(pointless_t* p, pointless_value_t* root, float** v, uint32_t* n, const char* e, ...);
int pointless_eval_get_as_vector_value(pointless_t* p, pointless_value_t* root, pointless_value_t** v, uint32_t* n, const char* e, ...);
int pointless_eval_get_as_bitvector(pointless_t* p, pointless_value_t* root, pointless_value_t* v, uint32_t* n, const char* e, ...);
int pointless_eval_get_as_boolean(pointless_t* p, pointless_value_t* root, uint32_t* v, const char* e, ...);

#endif
