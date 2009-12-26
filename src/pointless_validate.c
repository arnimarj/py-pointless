#include <pointless/pointless_validate.h>

/*
static int pointless_validate_set_complicated(pointless_validate_state_t* state, pointless_value_t* v)
{
	// get header
	assert(v->data.data_u32 < state->p->header->n_set);
	uint32_t offset = state->p->set_offsets[v->data.data_u32];
	pointless_set_header_t* header = (pointless_set_header_t*)((char*)state->p->heap_ptr + offset);

	// vectors must have the same number of items
	uint32_t n_hash = pointless_reader_vector_n_items(state->p, &header->hash_vector);
	uint32_t n_keys = pointless_reader_vector_n_items(state->p, &header->key_vector);

	if (n_hash != n_keys) {
		state->error = "set hash and key vectors do not contain the same number of items";
		return 0;
	}

	// get base pointers for both
	uint32_t* hashes = pointless_reader_vector_u32(state->p, &header->hash_vector);
	pointless_value_t* keys = pointless_reader_vector_value(state->p, &header->key_vector);

	// at this stage, all items have been validated, all that is left is to test the hash-map invariants
	return pointless_hash_table_validate(state->p, header->n_items, n_keys, hashes, keys, 0, &state->error);
}

static int pointless_validate_map_complicated(pointless_validate_state_t* state, pointless_value_t* v)
{
	// get header
	assert(v->data.data_u32 < state->p->header->n_map);
	uint32_t offset = state->p->map_offsets[v->data.data_u32];
	pointless_map_header_t* header = (pointless_map_header_t*)((char*)state->p->heap_ptr + offset);

	// vectors must have the same number of items
	uint32_t n_hash = pointless_reader_vector_n_items(state->p, &header->hash_vector);
	uint32_t n_keys = pointless_reader_vector_n_items(state->p, &header->key_vector);
	uint32_t n_values = pointless_reader_vector_n_items(state->p, &header->value_vector);

	// (a == b && b == c) <=> !(a != b || b != c)
	if (n_hash != n_keys || n_hash != n_values) {
		state->error = "map hash, key and value vectors do not contain the same number of items";
		return 0;

	}

	// get base pointers for both
	uint32_t* hashes = pointless_reader_vector_u32(state->p, &header->hash_vector);
	pointless_value_t* keys = pointless_reader_vector_value(state->p, &header->key_vector);
	pointless_value_t* values = pointless_reader_vector_value(state->p, &header->value_vector);

	// at this stage, all items have been validated, all that is left is to test the hash-map invariants
	return pointless_hash_table_validate(state->p, header->n_items, n_keys, hashes, keys, values, &state->error);
}
*/

typedef struct {
	uint32_t pass;
	const char* error;
	void* cycle_marker;
	void* vector;
	void* set;
	void* map;
} pointless_validate_state_t;

static uint32_t pointless_validate_pass_cb(pointless_t* p, pointless_value_t* v, uint32_t depth, void* user)
{
	pointless_validate_state_t* state = (pointless_validate_state_t*)user;

	assert(state->pass == 1 || state->pass == 2);

	if (depth >= POINTLESS_MAX_DEPTH) {
		state->error = "maximum depth exceeded";
		return POINTLESS_WALK_STOP;
	}

	// if we have validate this container already, stop iterating downwards, otherwise, mark it as visited
	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			if (bm_is_set(state->vector, v->data.data_u32))
				return POINTLESS_WALK_MOVE_UP;

			bm_set(state->vector, v->data.data_u32);

			break;
		case POINTLESS_SET_VALUE:
			if (bm_is_set(state->set, v->data.data_u32))
				return POINTLESS_WALK_MOVE_UP;

			bm_set(state->set, v->data.data_u32);

			break;
		case POINTLESS_MAP_VALUE_VALUE:
			if (bm_is_set(state->map, v->data.data_u32))
				return POINTLESS_WALK_MOVE_UP;

			bm_set(state->map, v->data.data_u32);

			break;
	}

	// pass-1, basic sanity checks
	if (state->pass == 1) {
		if (!pointless_validate_heap_ref(p, v, &state->error))
			return POINTLESS_WALK_STOP;

		if (!pointless_validate_inline_invariants(p, v, &state->error))
			return POINTLESS_WALK_STOP;

		if (!pointless_validate_heap_value(p, v, &state->error))
			return POINTLESS_WALK_STOP;
	// pass-2, cycle validation
	} else if (state->pass == 2) {
		// only hashable vector can not be in a cycle
		if (v->type == POINTLESS_VECTOR_VALUE_HASHABLE) {
			uint32_t container_id = pointless_container_id(p, v);

			if (bm_is_set(state->cycle_marker, container_id)) {
				state->error = "POINTLESS_VECTOR_VALUE_HASHABLE is in a cycle";
				return POINTLESS_WALK_STOP; 
			}
		}
	}

	// visit children
	return POINTLESS_WALK_VISIT_CHILDREN;
}

int pointless_validate(pointless_t* p, const char** error)
{
	// our return value
	int retval = 0;

	// setup the state
	pointless_validate_state_t state;

	state.pass = 1;
	state.error = 0;
	state.cycle_marker = 0;
	state.vector = calloc(ICEIL(p->header->n_vector, 8), 1);
	state.set = calloc(ICEIL(p->header->n_set, 8), 1);
	state.map = calloc(ICEIL(p->header->n_map, 8), 1);

	if (state.vector == 0 || state.set == 0 || state.map == 0) {
		*error = "out of memory";
		goto cleanup;
	}

	// pass 1
	pointless_walk(p, pointless_validate_pass_cb, (void*)&state);

	if (state.error)
		goto cleanup;

	// it is now safe to perform cycle analysis
	state.cycle_marker = pointless_cycle_marker(p, error);

	if (state.cycle_marker == 0)
		goto cleanup;

	// reset visited vector
	memset(state.vector, 0, ICEIL(p->header->n_vector, 8));
	memset(state.set, 0, ICEIL(p->header->n_set, 8));
	memset(state.map, 0, ICEIL(p->header->n_map, 8));

	// pass 2
	state.pass = 2;
	pointless_walk(p, pointless_validate_pass_cb, (void*)&state);

	if (state.error)
		goto cleanup;

	//! TBD: hashability test

	retval = 1;

cleanup:

	free(state.cycle_marker);
	free(state.vector);
	free(state.set);
	free(state.map);

	if (state.error)
		*error = state.error;

	return retval;
}
