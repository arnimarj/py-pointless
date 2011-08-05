#include <pointless/pointless_validate.h>

int32_t pointless_hash_table_validate(pointless_t* p, uint32_t n_items, uint32_t n_buckets, uint32_t* hash_vector, pointless_value_t* key_vector, pointless_value_t* value_vector, const char** error)
{
	if (pointless_hash_compute_n_buckets(n_items) != n_buckets) {
		*error = "invalid number of buckets in hash table";
		return 0;
	}

	// count empty vs. full slots
	uint32_t n_used = 0, n_empty = 0, i;

	for (i = 0; i < n_buckets; i++) {
		if (key_vector[i].type == POINTLESS_EMPTY_SLOT)
			n_empty += 1;
		else
			n_used += 1;

		// when key vector has an empty slot, so must the value vector
		if (value_vector && key_vector[i].type == POINTLESS_EMPTY_SLOT && value_vector[i].type != POINTLESS_EMPTY_SLOT) {
			*error = "empty slot in key vector does not imply an empty slot in value vector";
			return 0;
		}
	}

	if (n_empty == 0) {
		*error = "there are no empty slots in hash table, wtf";
		return 0;
	}

	if (n_used != n_items) {
		*error = "number of non-empty slots in hash-table, does not match item count";
		return 0;
	}

	// make sure hashes match the given object
	for (i = 0; i < n_buckets; i++) {
		// make sure it is hashable
		if (!pointless_is_hashable(key_vector[i].type)) {
			*error = "key in set/map is not hashable";
			return 0;
		}

		// just compute the hash, even for empty slots
		uint32_t h = pointless_hash_reader_32(p, &key_vector[i]);

		if (h != hash_vector[i]) {
			*error = "hash for object in hash-table does not match hash in slot";
			return 0;
		}
	}

	// right, all the hashes match, now, make sure they are in the right place
	for (i = 0; i < n_buckets; i++) {
		if (key_vector[i].type == POINTLESS_EMPTY_SLOT)
			continue;

		uint32_t probe_i = pointless_hash_table_probe(p, hash_vector[i], &key_vector[i], n_buckets, hash_vector, key_vector, error);

		if (probe_i == POINTLESS_HASH_TABLE_PROBE_ERROR)
			return 0;

		if (probe_i == POINTLESS_HASH_TABLE_PROBE_MISS || probe_i != i) {
			*error = "probing of key in hash-table, does not match the place it is in";
			return 0;
		}
	}

	// we're good
	return 1;
}
