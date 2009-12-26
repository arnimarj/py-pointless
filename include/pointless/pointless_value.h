#ifndef __POINTLESS__TYPE__H__
#define __POINTLESS__TYPE__H__

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <pointless/pointless_defs.h>

// constructors for "in-line" types
pointless_create_value_t pointless_value_create_i32(int32_t v);
pointless_create_value_t pointless_value_create_u32(uint32_t v);
pointless_create_value_t pointless_value_create_float(float v);

pointless_create_value_t pointless_value_create_bool_false();
pointless_create_value_t pointless_value_create_bool_true();
pointless_create_value_t pointless_value_create_null();
pointless_create_value_t pointless_value_create_empty_slot();

pointless_value_t pointless_value_create_as_read_i32(int32_t v);
pointless_value_t pointless_value_create_as_read_u32(uint32_t v);
pointless_value_t pointless_value_create_as_read_float(float v);

pointless_value_t pointless_value_create_as_read_bool_false();
pointless_value_t pointless_value_create_as_read_bool_true();
pointless_value_t pointless_value_create_as_read_null();
pointless_value_t pointless_value_create_as_read_empty_slot();

int32_t pointless_value_get_i32(uint32_t t, pointless_value_data_t* v);
uint32_t pointless_value_get_u32(uint32_t t, pointless_value_data_t* v);
float pointless_value_get_float(uint32_t t, pointless_value_data_t* v);
uint32_t pointless_value_get_bool(uint32_t t, pointless_value_data_t* v);

int64_t pointless_get_int_as_int64(uint32_t t, pointless_value_data_t* v);

int32_t  pointless_create_value_get_i32(pointless_create_value_t* v);
uint32_t pointless_create_value_get_u32(pointless_create_value_t* v);
float    pointless_create_value_get_float(pointless_create_value_t* v);
uint32_t pointless_create_value_get_bool(pointless_create_value_t* v);

int64_t pointless_create_get_int_as_int64(pointless_create_value_t* v);


#endif
