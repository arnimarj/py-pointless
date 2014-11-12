#ifndef __POINTLESS__HASH__TABLE__H__
#define __POINTLESS__HASH__TABLE__H__

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

#include <pointless/pointless_defs.h>
#include <pointless/pointless_value.h>

#define POINTLESS_HASH_TABLE_PROBE_MISS UINT32_MAX
#define POINTLESS_HASH_TABLE_PROBE_ERROR (UINT32_MAX-1)

typedef struct {
	uint32_t perturb;
	uint32_t i;
	uint32_t mask;
} pointless_hash_iter_state_t;

uint32_t pointless_hash_compute_n_buckets(uint32_t n_items);
uint32_t pointless_hash_table_probe(pointless_t* p, uint32_t value_hash, pointless_value_t* value, uint32_t n_buckets, uint32_t* hash_vector, pointless_value_t* key_vector, const char** error);
uint32_t pointless_hash_table_probe_ext(pointless_t* p, uint32_t value_hash, pointless_eq_cb cb, void* user, uint32_t n_buckets, uint32_t* hash_vector, pointless_value_t* key_vector, const char** error);
int pointless_hash_table_populate(pointless_create_t* c, uint32_t* hash_vector, uint32_t* keys_vector, uint32_t* values_vector, uint32_t n_keys, uint32_t* hash_serialize, uint32_t* keys_serialize, uint32_t* values_serialize, uint32_t n_buckets, uint32_t empty_slot_handle, const char** error);

void pointless_hash_table_probe_hash_init(pointless_t* p, uint32_t value_hash, uint32_t n_buckets, pointless_hash_iter_state_t* state);
uint32_t pointless_hash_table_probe_hash(pointless_t* p, uint32_t* hash_vector, pointless_value_t* key_vector, pointless_hash_iter_state_t* state, uint32_t* bucket_out);

#endif
