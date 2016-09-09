#include <pointless/pointless_walk.h>

static void pointless_walk_priv(pointless_t* p, pointless_value_t* v, uint32_t depth, pointless_walk_cb cb, uint32_t* stop, void* user)
{
	// visit this value
	uint32_t action = (*cb)(p, v, depth, user);

	if (action == POINTLESS_WALK_MOVE_UP)
		return;

	if (action == POINTLESS_WALK_STOP) {
		*stop = 1;
		return;
	}

	assert(action == POINTLESS_WALK_VISIT_CHILDREN);

	// walk through its children, if any

	// vectors
	if (v->type == POINTLESS_VECTOR_VALUE || v->type == POINTLESS_VECTOR_VALUE_HASHABLE) {
		uint32_t i, n_items = pointless_reader_vector_n_items(p, v);
		pointless_value_t* v_items = pointless_reader_vector_value(p, v);

		for (i = 0; i < n_items; i++) {
			pointless_walk_priv(p, &v_items[i], depth + 1, cb, stop, user);

			if (*stop)
				return;
		}
	// sets
	} else if (v->type == POINTLESS_SET_VALUE) {
		pointless_value_t* hash_vector = pointless_set_hash_vector(p, v);
		pointless_value_t* key_vector = pointless_set_key_vector(p, v);

		pointless_walk_priv(p, hash_vector, depth + 1, cb, stop, user);

		if (*stop)
			return;

		pointless_walk_priv(p, key_vector, depth + 1, cb, stop, user);

		if (*stop)
			return;
	// maps
	} else if (v->type == POINTLESS_MAP_VALUE_VALUE) {
		pointless_value_t* hash_vector = pointless_map_hash_vector(p, v);
		pointless_value_t* key_vector = pointless_map_key_vector(p, v);
		pointless_value_t* value_vector = pointless_map_value_vector(p, v);

		pointless_walk_priv(p, hash_vector, depth + 1, cb, stop, user);

		if (*stop)
			return;

		pointless_walk_priv(p, key_vector, depth + 1, cb, stop, user);

		if (*stop)
			return;

		pointless_walk_priv(p, value_vector, depth + 1, cb, stop, user);

		if (*stop)
			return;
	}

	// done
}

void pointless_walk(pointless_t* p, pointless_walk_cb cb, void* user)
{
	uint32_t stop = 0;
	pointless_value_t* root = pointless_root(p);
	pointless_walk_priv(p, root, 0, cb, &stop, user);
}
