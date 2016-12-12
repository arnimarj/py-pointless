#ifndef __POINTLESS__CREATE__H__
#define __POINTLESS__CREATE__H__

#include <unistd.h>
#include <assert.h>

#ifndef __cplusplus
	#include <limits.h>
	#include <stdint.h>
#else
	#include <climits>
	#include <cstdint>
#endif

#include <pointless/pointless_defs.h>
#include <pointless/pointless_malloc.h>
#include <pointless/pointless_dynarray.h>
#include <pointless/pointless_hash_table.h>
#include <pointless/pointless_create_cache.h>
#include <pointless/pointless_unicode_utils.h>
#include <pointless/bitutils.h>
#include <pointless/pointless_cycle_marker_wrappers.h>

#include <Judy.h>

// creation
void pointless_create_begin_32(pointless_create_t* c);
void pointless_create_begin_64(pointless_create_t* c);
void pointless_create_end(pointless_create_t* c);
int pointless_create_output_and_end_f(pointless_create_t* c, const char* fname, const char** error);
int pointless_create_output_and_end_b(pointless_create_t* c, void** buf, size_t* buflen, const char** error);

// set the root
void pointless_create_set_root(pointless_create_t* c, uint32_t root);

// inline-values
uint32_t pointless_create_i32(pointless_create_t* c, int32_t v);
uint32_t pointless_create_u32(pointless_create_t* c, uint32_t v);
uint32_t pointless_create_float(pointless_create_t* c, float v);
uint32_t pointless_create_null(pointless_create_t* c);

uint32_t pointless_create_boolean_true(pointless_create_t* c);
uint32_t pointless_create_boolean_false(pointless_create_t* c);
uint32_t pointless_create_empty_slot(pointless_create_t* c);
uint32_t pointless_create_boolean(pointless_create_t* c, int32_t v);

// unicode constructors, using different sources
uint32_t pointless_create_unicode_ucs4(pointless_create_t* c, uint32_t* s);
uint32_t pointless_create_unicode_ucs2(pointless_create_t* c, uint16_t* s);
uint32_t pointless_create_unicode_ascii(pointless_create_t* c, const char* s, const char** error);

// string constructors, using different sources (each value must be at most 8 bits)
uint32_t pointless_create_string_ucs4(pointless_create_t* c, uint32_t* v);
uint32_t pointless_create_string_ucs2(pointless_create_t* c, uint16_t* s);
uint32_t pointless_create_string_ascii(pointless_create_t* c, uint8_t* s);

// bitvectors
uint32_t pointless_create_bitvector(pointless_create_t* c, void* v, uint32_t n_bits);
uint32_t pointless_create_bitvector_no_normalize(pointless_create_t* c, void* v, uint32_t n_bits);
uint32_t pointless_create_bitvector_compressed(pointless_create_t* c, pointless_value_t* v);

// vectors, buffer owned by library
uint32_t pointless_create_vector_value(pointless_create_t* c);
uint32_t pointless_create_vector_i8(pointless_create_t* c);
uint32_t pointless_create_vector_u8(pointless_create_t* c);
uint32_t pointless_create_vector_i16(pointless_create_t* c);
uint32_t pointless_create_vector_u16(pointless_create_t* c);
uint32_t pointless_create_vector_i32(pointless_create_t* c);
uint32_t pointless_create_vector_u32(pointless_create_t* c);
uint32_t pointless_create_vector_i64(pointless_create_t* c);
uint32_t pointless_create_vector_u64(pointless_create_t* c);
uint32_t pointless_create_vector_float(pointless_create_t* c);

uint32_t pointless_create_vector_value_append(pointless_create_t* c, uint32_t vector, uint32_t v);
uint32_t pointless_create_vector_i8_append(pointless_create_t* c, uint32_t vector, int8_t v);
uint32_t pointless_create_vector_u8_append(pointless_create_t* c, uint32_t vector, uint8_t v);
uint32_t pointless_create_vector_i16_append(pointless_create_t* c, uint32_t vector, int16_t v);
uint32_t pointless_create_vector_u16_append(pointless_create_t* c, uint32_t vector, uint16_t v);
uint32_t pointless_create_vector_i32_append(pointless_create_t* c, uint32_t vector, int32_t v);
uint32_t pointless_create_vector_u32_append(pointless_create_t* c, uint32_t vector, uint32_t v);
uint32_t pointless_create_vector_i64_append(pointless_create_t* c, uint32_t vector, int64_t v);
uint32_t pointless_create_vector_u64_append(pointless_create_t* c, uint32_t vector, uint64_t v);
uint32_t pointless_create_vector_float_append(pointless_create_t* c, uint32_t vector, float v);

void pointless_create_vector_value_set(pointless_create_t* c, uint32_t vector, uint32_t i, uint32_t v);

uint32_t pointless_create_vector_u32_transfer(pointless_create_t* c, uint32_t vector, uint32_t* v, uint32_t n);
uint32_t pointless_create_vector_value_transfer(pointless_create_t* c, uint32_t vector, uint32_t* v, uint32_t n);

// vectors, buffer owned by caller
uint32_t pointless_create_vector_i8_owner(pointless_create_t* c, int8_t* items, uint32_t n_items);
uint32_t pointless_create_vector_u8_owner(pointless_create_t* c, uint8_t* items, uint32_t n_items);
uint32_t pointless_create_vector_i16_owner(pointless_create_t* c, int16_t* items, uint32_t n_items);
uint32_t pointless_create_vector_u16_owner(pointless_create_t* c, uint16_t* items, uint32_t n_items);
uint32_t pointless_create_vector_i32_owner(pointless_create_t* c, int32_t* items, uint32_t n_items);
uint32_t pointless_create_vector_u32_owner(pointless_create_t* c, uint32_t* items, uint32_t n_items);
uint32_t pointless_create_vector_i64_owner(pointless_create_t* c, int64_t* items, uint32_t n_items);
uint32_t pointless_create_vector_u64_owner(pointless_create_t* c, uint64_t* items, uint32_t n_items);
uint32_t pointless_create_vector_float_owner(pointless_create_t* c, float* items, uint32_t n_items);

// sets
uint32_t pointless_create_set(pointless_create_t* c);
uint32_t pointless_create_set_add(pointless_create_t* c, uint32_t s, uint32_t k);

// maps
uint32_t pointless_create_map(pointless_create_t* c);
uint32_t pointless_create_map_add(pointless_create_t* c, uint32_t m, uint32_t k, uint32_t v);

#endif
