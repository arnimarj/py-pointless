#ifndef __POINTLESS__TYPE__H__
#define __POINTLESS__TYPE__H__

#include <stdlib.h>
#include <assert.h>

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

#include <pointless/pointless_defs.h>

// create-time values
pointless_create_value_t pointless_value_create_i32(int32_t v);
pointless_create_value_t pointless_value_create_u32(uint32_t v);
pointless_create_value_t pointless_value_create_float(float v);

pointless_create_value_t pointless_value_create_bool_false();
pointless_create_value_t pointless_value_create_bool_true();
pointless_create_value_t pointless_value_create_null();
pointless_create_value_t pointless_value_create_empty_slot();

// read-time values
pointless_value_t pointless_value_create_as_read_i32(int32_t v);
pointless_value_t pointless_value_create_as_read_u32(uint32_t v);
pointless_value_t pointless_value_create_as_read_float(float v);

pointless_value_t pointless_value_create_as_read_bool_false();
pointless_value_t pointless_value_create_as_read_bool_true();
pointless_value_t pointless_value_create_as_read_null();
pointless_value_t pointless_value_create_as_read_empty_slot();

// hash/cmp-time values

// ...create
pointless_complete_create_value_t pointless_complete_value_create_i32(int32_t v);
pointless_complete_create_value_t pointless_complete_value_create_u32(uint32_t v);
pointless_complete_create_value_t pointless_complete_value_create_i64(int64_t v);
pointless_complete_create_value_t pointless_complete_value_create_u64(uint64_t v);
pointless_complete_create_value_t pointless_complete_value_create_float(float v);
pointless_complete_create_value_t pointless_complete_value_create_null();

//  ...read
pointless_complete_value_t pointless_complete_value_create_as_read_i32(int32_t v);
pointless_complete_value_t pointless_complete_value_create_as_read_u32(uint32_t v);
pointless_complete_value_t pointless_complete_value_create_as_read_i64(int64_t v);
pointless_complete_value_t pointless_complete_value_create_as_read_u64(uint64_t v);
pointless_complete_value_t pointless_complete_value_create_as_read_float(float v);
pointless_complete_value_t pointless_complete_value_create_as_read_null();

// read-time accessors
int32_t pointless_value_get_i32(uint32_t t, pointless_value_data_t* v);
uint32_t pointless_value_get_u32(uint32_t t, pointless_value_data_t* v);
float pointless_value_get_float(uint32_t t, pointless_value_data_t* v);
uint32_t pointless_value_get_bool(uint32_t t, pointless_value_data_t* v);
int64_t pointless_get_int_as_int64(uint32_t t, pointless_value_data_t* v);

// create-time accessor
int64_t pointless_create_get_int_as_int64(pointless_create_value_t* v);
int32_t pointless_create_value_get_i32(pointless_create_value_t* v);
uint32_t pointless_create_value_get_u32(pointless_create_value_t* v);
float pointless_create_value_get_float(pointless_create_value_t* v);

// hash-time accessor
int64_t pointless_complete_value_get_as_i64(uint32_t t, pointless_complete_value_data_t* v);
uint64_t pointless_complete_value_get_as_u64(uint32_t t, pointless_complete_value_data_t* v);
float pointless_complete_value_get_float(uint32_t t, pointless_complete_value_data_t* v);

// conversions
pointless_value_t pointless_value_from_complete(pointless_complete_value_t* a);
pointless_complete_value_t pointless_value_to_complete(pointless_value_t* a);

pointless_create_value_t pointless_create_value_from_complete(pointless_complete_create_value_t* a);
pointless_complete_create_value_t pointless_create_value_to_complete(pointless_create_value_t* a);

// utilities
int32_t pointless_is_vector_type(uint32_t type);
int32_t pointless_is_integer_type(uint32_t type);

#endif
