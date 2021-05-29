#ifndef __POINTLESS__READER__HELPERS__H__
#define __POINTLESS__READER__HELPERS__H__

#include <pointless/pointless_defs.h>
#include <pointless/pointless_reader.h>

// map operator wrappers
int pointless_get_mapping_string_to_u32(pointless_t* p, pointless_value_t* map, char* key, uint32_t* value);
int pointless_get_mapping_string_to_i64(pointless_t* p, pointless_value_t* map, char* key, int64_t* value);
int pointless_get_mapping_string_to_set(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* s);
int pointless_get_mapping_string_to_map(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* m);
int pointless_get_mapping_string_to_value(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* m);
int pointless_get_mapping_int_to_value(pointless_t* p, pointless_value_t* map, int64_t i, pointless_value_t* v);
int pointless_get_mapping_string_to_value(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* value);
int pointless_get_mapping_string_n_to_value(pointless_t* p, pointless_value_t* map, char* key, size_t n, pointless_value_t* value);

int pointless_get_mapping_unicode_to_value(pointless_t* p, pointless_value_t* map, uint32_t* key, pointless_value_t* value);
int pointless_get_mapping_unicode_to_u32(pointless_t* p, pointless_value_t* map, uint32_t* key, uint32_t* value);

int pointless_get_mapping_string_to_vector(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* v, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_i8(pointless_t* p, pointless_value_t* map, char* key, int8_t** value, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_u8(pointless_t* p, pointless_value_t* map, char* key, uint8_t** value, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_i16(pointless_t* p, pointless_value_t* map, char* key, int16_t** value, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_u16(pointless_t* p, pointless_value_t* map, char* key, uint16_t** value, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_i32(pointless_t* p, pointless_value_t* map, char* key, int32_t** value, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_u32(pointless_t* p, pointless_value_t* map, char* key, uint32_t** value, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_i64(pointless_t* p, pointless_value_t* map, char* key, int64_t** value, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_u64(pointless_t* p, pointless_value_t* map, char* key, uint64_t** value, uint32_t* n_items);

int pointless_get_mapping_string_to_vector_float(pointless_t* p, pointless_value_t* map, char* key, float** value, uint32_t* n_items);
int pointless_get_mapping_string_to_vector_value(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t** value, uint32_t* n_items);

// set/map inclusion wrappers
int pointless_is_int_in_set(pointless_t* p, pointless_value_t* s, int64_t i);
int pointless_is_int_in_map(pointless_t* p, pointless_value_t* m, int64_t i);

// test for set of keys equivalence
int pointless_has_same_keys_set_map(pointless_t* p, pointless_value_t* s, pointless_value_t* m);
int pointless_has_same_keys_map_map(pointless_t* p, pointless_value_t* m_a, pointless_value_t* m_b);

// test for key inclusion, for values which are guaranteed to be acyclic (all containers and values, except non-hashable value-vectors or those containing one)
int pointless_is_in_set_acyclic(pointless_t* p, pointless_value_t* s, pointless_value_t* k);
int pointless_is_in_map_acyclic(pointless_t* p, pointless_value_t* m, pointless_value_t* k);

#endif
