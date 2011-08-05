#ifndef __POINTLESS__BITVECTOR__H__
#define __POINTLESS__BITVECTOR__H__

#include <pointless/bitutils.h>
#include <pointless/pointless_defs.h>

uint32_t pointless_bitvector_is_any_set(uint32_t t, pointless_value_data_t* v, void* bits);

uint32_t pointless_bitvector_n_bits(uint32_t t, pointless_value_data_t* v, void* buffer);
uint32_t pointless_bitvector_is_set(uint32_t t, pointless_value_data_t* v, void* buffer, uint32_t bit);

uint32_t pointless_bitvector_hash_32(uint32_t t, pointless_value_data_t* v, void* buffer);
uint64_t pointless_bitvector_hash_64(uint32_t t, pointless_value_data_t* v, void* buffer);

int32_t pointless_bitvector_cmp_buffer_buffer(uint32_t t_a, pointless_value_data_t* v_a, void* buffer_a, uint32_t t_b, pointless_value_data_t* v_b, void* buffer_b);
int32_t pointless_bitvector_cmp_bits_buffer(uint32_t n_bits_a, void* bits_a, pointless_value_t* v_b, void* buffer_b);
int32_t pointless_bitvector_cmp_buffer_bits(pointless_value_t* v_a, void* buffer_a, uint32_t n_bits_b, void* bits_b);

uint32_t pointless_bitvector_hash_buffer_32(void* buffer);
uint64_t pointless_bitvector_hash_buffer_64(void* buffer);

uint32_t pointless_bitvector_hash_n_bits_bits_32(uint32_t n_bits, void* bits);
uint64_t pointless_bitvector_hash_n_bits_bits_64(uint32_t n_bits, void* bits);

int32_t pointless_bitvector_cmp_buffer(void* a, void* b);

#endif
