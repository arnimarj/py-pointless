#include <pointless/pointless_hash_table.h>

static uint32_t next_power_of_2(uint32_t n)
{
	uint32_t p = 1;

	while (p < n)
		p <<= 1;

	return p;
}

uint32_t pointless_hash_compute_n_buckets(uint32_t n_items)
{
	// 1 + 1 / 2 == 1, meaning that a set of 1 item would have 1 bucket, which would brake
	// the invariant that all sets must have an empty slot
	if (n_items == 1)
		return 2;

	return next_power_of_2(n_items + n_items / 2);
}

static uint32_t pointless_hash_table_probe_priv(pointless_t* p, uint32_t value_hash, pointless_value_t* value, uint32_t n_buckets, uint32_t* hash_vector, pointless_value_t* key_vector, pointless_eq_cb cb, void* user, const char** error)
{
	// we use the same probing strategy as Python
	// 1) number of buckets is a power-of-2
	// 2) the recurrence used is: j = (5*j) + 1 + perturb
	//    perturb is initialized to hash value, and shifted left by PERTURB_SHIFT each iteration
	//    Since the recurrence j = (5*j) + 1 will repeat, after having visited all 2**i buckets
	//    so will the first recurrence, since perturb will eventually reach zero.
	// 3) PERTURB_SHIFT is set to 5
	uint32_t perturb = value_hash, i = value_hash, mask = n_buckets - 1, bucket;

	while (1) {
		bucket = i & mask;

		// we hit an empty bucket
		if (key_vector[bucket].type == POINTLESS_EMPTY_SLOT)
			return POINTLESS_HASH_TABLE_PROBE_MISS;

		// test hash
		if (value_hash == hash_vector[bucket]) {
			// test key equality
			uint32_t is_equal;

			if (cb) {
				pointless_complete_value_t v_a = pointless_value_to_complete(&key_vector[bucket]);
				is_equal = ((*cb)(p, &v_a, user, error) != 0);
			} else {
				pointless_complete_value_t v_a = pointless_value_to_complete(value);
				pointless_complete_value_t v_b = pointless_value_to_complete(&key_vector[bucket]);
				is_equal = (pointless_cmp_reader(p, &v_a, p, &v_b, error) == 0);
			}

			if (*error)
				return POINTLESS_HASH_TABLE_PROBE_ERROR;

			if (is_equal)
				return bucket;
		}

		// compute recurrence
		i = (i << 2) + i + perturb + 1;
		perturb >>= 5;
	}

	// will never reach here
	assert(0);
	*error = "internal probing error";
	return POINTLESS_HASH_TABLE_PROBE_ERROR;
}

void pointless_hash_table_probe_hash_init(pointless_t* p, uint32_t value_hash, uint32_t n_buckets, pointless_hash_iter_state_t* state)
{
	state->perturb = value_hash;
	state->i = value_hash;
	state->mask = n_buckets - 1;
}

uint32_t pointless_hash_table_probe_hash(pointless_t* p, uint32_t* hash_vector, pointless_value_t* key_vector, pointless_hash_iter_state_t* state, uint32_t* bucket_out)
{
	uint32_t bucket = state->i & state->mask;

	// we're at an empty bucket
	if (key_vector[bucket].type == POINTLESS_EMPTY_SLOT)
		return 0;

	// compute recurrence
	state->i = (state->i << 2) + state->i + state->perturb + 1;
	state->perturb >>= 5;

	// return bucket
	*bucket_out = bucket;
	return 1;
}

uint32_t pointless_hash_table_probe(pointless_t* p, uint32_t value_hash, pointless_value_t* value, uint32_t n_buckets, uint32_t* hash_vector, pointless_value_t* key_vector, const char** error)
{
	return pointless_hash_table_probe_priv(p, value_hash, value, n_buckets, hash_vector, key_vector, 0, 0, error);
}

uint32_t pointless_hash_table_probe_ext(pointless_t* p, uint32_t value_hash, pointless_eq_cb cb, void* user, uint32_t n_buckets, uint32_t* hash_vector, pointless_value_t* key_vector, const char** error)
{
	return pointless_hash_table_probe_priv(p, value_hash, 0, n_buckets, hash_vector, key_vector, cb, user, error);
}

int pointless_hash_table_populate(pointless_create_t* c, uint32_t* hash_vector, uint32_t* keys_vector, uint32_t* values_vector, uint32_t n_keys, uint32_t* hash_serialize, uint32_t* keys_serialize, uint32_t* values_serialize, uint32_t n_buckets, uint32_t empty_slot_handle, const char** error)
{
	uint32_t j;

	if (n_keys > 0 && ((values_vector == 0) != (values_serialize == 0))) {
		*error = "pointless_hash_table_populate(): caller must specify either both of values_vector/values_serialize or neither";
		return 0;
	}

	for (j = 0; j < n_keys; j++) {
		if (keys_vector[j] == empty_slot_handle) {
			*error = "pointless_hash_table_populate(): internal invariant error A";
			return 0;
		}
	}

	for (j = 0; j < n_buckets; j++) {
		if (hash_serialize[j] != 0) {
			*error = "pointless_hash_table_populate(): internal invariant error B";
			return 0;
		}

		if (keys_serialize[j] != empty_slot_handle) {
			*error = "pointless_hash_table_populate(): internal invariant error C";
			return 0;
		}

		if (values_serialize && values_serialize[j] != empty_slot_handle) {
			*error = "pointless_hash_table_populate(): internal invariant error D";
			return 0;
		}
	}

	assert(n_buckets > n_keys);

	int32_t cmp;
	uint32_t value_hash, perturb, bucket, i, mask = n_buckets - 1;

	for (j = 0; j < n_keys; j++) {
		// NOTE: basically same code as above
		value_hash = hash_vector[j];
		perturb = value_hash;
		i = value_hash;

		// find an empty slot
		while (1) {
			bucket = i & mask;

			// we hit an empty bucket
			if (keys_serialize[bucket] == empty_slot_handle) {
				hash_serialize[bucket] = value_hash;
				keys_serialize[bucket] = keys_vector[j];

				if (values_serialize)
					values_serialize[bucket] = values_vector[j];

				break;
			}

			// perhaps, we have an item, which is equal to ours
			if (hash_serialize[bucket] == hash_vector[j]) {
				cmp = pointless_cmp_create(c, keys_serialize[bucket], keys_vector[j], error);

				if (*error)
					return 0;

				if (cmp == 0) {
					*error = "there are duplicate keys in the set/map";
					return 0;
				}
			}

			// probe on
			i = (i << 2) + i + perturb + 1;
			perturb >>= 5;
		}
	}

	return 1;
}
