#ifndef __POINTLESS__READER__UTILS__H__
#define __POINTLESS__READER__UTILS__H__

#include <pointless/pointless_defs.h>
#include <pointless/pointless_hash_table.h>

// the root value
pointless_value_t* pointless_root(pointless_t* p);

// string/unicode values, and encoders
uint32_t pointless_reader_unicode_len(pointless_t* p, pointless_value_t* v);
uint32_t* pointless_reader_unicode_value_ucs4(pointless_t* p, pointless_value_t* v);

uint32_t pointless_reader_string_len(pointless_t* p, pointless_value_t* v);
uint8_t* pointless_reader_string_value_ascii(pointless_t* p, pointless_value_t* v);

#ifdef POINTLESS_WCHAR_T_IS_4_BYTES
wchar_t* pointless_reader_unicode_value_wchar(pointless_t* p, pointless_value_t* v);
#endif

// note: caller must free() the returned buffers
uint16_t* pointless_reader_unicode_value_ucs2_alloc(pointless_t* p, pointless_value_t* v, const char** error);

// vectors
uint32_t pointless_reader_vector_n_items(pointless_t* p, pointless_value_t* v);
pointless_value_t* pointless_reader_vector_value(pointless_t* p, pointless_value_t* v);
pointless_complete_value_t pointless_reader_complete_vector_value(pointless_t* p, pointless_value_t* v);
int8_t* pointless_reader_vector_i8(pointless_t* p, pointless_value_t* v);
uint8_t* pointless_reader_vector_u8(pointless_t* p, pointless_value_t* v);
int16_t* pointless_reader_vector_i16(pointless_t* p, pointless_value_t* v);
uint16_t* pointless_reader_vector_u16(pointless_t* p, pointless_value_t* v);
int32_t* pointless_reader_vector_i32(pointless_t* p, pointless_value_t* v);
uint32_t* pointless_reader_vector_u32(pointless_t* p, pointless_value_t* v);
int64_t* pointless_reader_vector_i64(pointless_t* p, pointless_value_t* v);
uint64_t* pointless_reader_vector_u64(pointless_t* p, pointless_value_t* v);
float* pointless_reader_vector_float(pointless_t* p, pointless_value_t* v);

// general value fetcher
pointless_complete_value_t pointless_reader_vector_value_case(pointless_t* p, pointless_value_t* v, uint32_t i);

// bitvectors
uint32_t pointless_reader_bitvector_n_bits(pointless_t* p, pointless_value_t* v);
uint32_t pointless_reader_bitvector_is_set(pointless_t* p, pointless_value_t* v, uint32_t bit);
void* pointless_reader_bitvector_buffer(pointless_t* p, pointless_value_t* v);

// sets
uint32_t pointless_reader_set_n_items(pointless_t* p, pointless_value_t* s);
uint32_t pointless_reader_set_n_buckets(pointless_t* p, pointless_value_t* s);
uint32_t pointless_reader_set_iter(pointless_t* p, pointless_value_t* s, pointless_value_t** k, uint32_t* iter_state);
void pointless_reader_set_lookup(pointless_t* p, pointless_value_t* s, pointless_value_t* k, pointless_value_t** kk, const char** error);
void pointless_reader_set_lookup_ext(pointless_t* p, pointless_value_t* s, uint32_t hash, pointless_eq_cb cb, void* user, pointless_value_t** kk, const char** error);

pointless_value_t* pointless_set_hash_vector(pointless_t* p, pointless_value_t* s);
pointless_value_t* pointless_set_key_vector(pointless_t* p, pointless_value_t* s);

// maps
uint32_t pointless_reader_map_n_items(pointless_t* p, pointless_value_t* m);
uint32_t pointless_reader_map_n_buckets(pointless_t* p, pointless_value_t* m);
uint32_t pointless_reader_map_iter(pointless_t* p, pointless_value_t* m, pointless_value_t** k, pointless_value_t** vv, uint32_t* iter_state);
void pointless_reader_map_lookup(pointless_t* p, pointless_value_t* m, pointless_value_t* k, pointless_value_t** kk, pointless_value_t** vv, const char** error);
void pointless_reader_map_lookup_ext(pointless_t* p, pointless_value_t* m, uint32_t hash, pointless_eq_cb cb, void* user, pointless_value_t** kk, pointless_value_t** vv, const char** error);

pointless_value_t* pointless_map_hash_vector(pointless_t* p, pointless_value_t* m);
pointless_value_t* pointless_map_key_vector(pointless_t* p, pointless_value_t* m);
pointless_value_t* pointless_map_value_vector(pointless_t* p, pointless_value_t* m);

// map/set conditional iterators
void pointless_reader_map_iter_hash_init(pointless_t* p, pointless_value_t* m, uint32_t hash, pointless_hash_iter_state_t* iter_state);
uint32_t pointless_reader_map_iter_hash(pointless_t* p, pointless_value_t* m, uint32_t hash, pointless_value_t** kk, pointless_value_t** vv, pointless_hash_iter_state_t* iter_state);

void pointless_reader_set_iter_hash_init(pointless_t* p, pointless_value_t* m, uint32_t hash, pointless_hash_iter_state_t* iter_state);
uint32_t pointless_reader_set_iter_hash(pointless_t* p, pointless_value_t* s, uint32_t hash, pointless_value_t** kk, pointless_hash_iter_state_t* iter_state);

// get ID of container (non-empty vectors, sets and maps)
uint32_t pointless_n_containers(pointless_t* p);
uint32_t pointless_container_id(pointless_t* p, pointless_value_t* c);

#endif
