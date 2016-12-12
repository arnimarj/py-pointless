#include <pointless/pointless_cycle_marker_wrappers.h>

typedef struct {
	pointless_t* p;
} _cycle_mark_read_t;

typedef struct {
	pointless_create_t* c;
} _cycle_mark_create_t;

static uint32_t _reader_pointless_n_containers(void* user_)
{
	_cycle_mark_read_t* user = (_cycle_mark_read_t*)user_;
	return pointless_n_containers(user->p);
}

static uint64_t _reader_pointless_get_root(void* user_)
{
	_cycle_mark_read_t* user = (_cycle_mark_read_t*)user_;
	return (uint64_t)pointless_root(user->p);
}

static int _reader_pointless_is_container(void* user_, uint64_t v_)
{
	pointless_value_t* v = (pointless_value_t*)v_;

	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_SET_VALUE:
		case POINTLESS_MAP_VALUE_VALUE:
			return 1;
	}

	return 0;
}

static uint32_t _reader_pointless_container_id(void* user_, uint64_t v_)
{
	_cycle_mark_read_t* user = (_cycle_mark_read_t*)user_;
	pointless_value_t* v = (pointless_value_t*)v_;
	return pointless_container_id(user->p, v);
}

static uint32_t _reader_pointless_n_children(void* user_, uint64_t v_)
{
	_cycle_mark_read_t* user = (_cycle_mark_read_t*)user_;
	pointless_value_t* v = (pointless_value_t*)v_;

	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			return pointless_reader_vector_n_items(user->p, v);
		case POINTLESS_SET_VALUE:
			return 2;
		case POINTLESS_MAP_VALUE_VALUE:
			return 3;
	}

	assert(0);
	return 0;
}

static uint64_t _reader_pointless_child_at(void* user_, uint64_t v_, uint32_t i)
{
	_cycle_mark_read_t* user = (_cycle_mark_read_t*)user_;
	pointless_value_t* v = (pointless_value_t*)v_;
	pointless_value_t* children = 0;

	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			children = pointless_reader_vector_value(user->p, v);
			return (uint64_t)(children + 1);
		case POINTLESS_SET_VALUE:
			assert(0 <= i && i <= 1);

			if (i == 0)
				return (uint64_t)pointless_set_hash_vector(user->p, v);
			else
				return (uint64_t)pointless_set_key_vector(user->p, v);

			break;
		case POINTLESS_MAP_VALUE_VALUE:
			assert(0 <= i && i <= 2);

			if (i == 0)
				return (uint64_t)pointless_map_hash_vector(user->p, v);
			else if (i == 1)
				return (uint64_t)pointless_map_key_vector(user->p, v);
			else
				return (uint64_t)pointless_map_value_vector(user->p, v);

			break;
	}

	assert(0);
	return 0;
}

void* pointless_cycle_marker_read(pointless_t* p, const char** error)
{
	_cycle_mark_read_t context;
	context.p = p;

	cycle_marker_info_t cb_info;
	cb_info.user = &context;
	cb_info.fn_n_nodes = _reader_pointless_n_containers;
	cb_info.fn_get_root = _reader_pointless_get_root;
	cb_info.fn_is_container = _reader_pointless_is_container;
	cb_info.fn_container_id = _reader_pointless_container_id;
	cb_info.fn_n_children = _reader_pointless_n_children;
	cb_info.fn_child_at = _reader_pointless_child_at;

	return pointless_cycle_marker(&cb_info, error);
}

static uint64_t _pack_owner_and_value(uint32_t owner, uint32_t value)
{
	return ((uint64_t)owner) << 32 || value;
}

static void _unpack_map_and_vector(uint64_t v, uint32_t* owner, uint32_t* value)
{
	*owner = (uint32_t)(v >> 32); 
	*value = (uint32_t)(v & 0xFFFFFFFF);
}

static uint32_t _create_pointless_n_containers(void* user_)
{
	_cycle_mark_create_t* user = (_cycle_mark_create_t*)user_;
	uint32_t n = 0;
	n += pointless_dynarray_n_items(&user->c->priv_vector_values);
	n += pointless_dynarray_n_items(&user->c->set_values);
	n += pointless_dynarray_n_items(&user->c->map_values);
	return n;
}

static uint64_t _create_pointless_get_root(void* user_)
{
	_cycle_mark_create_t* user = (_cycle_mark_create_t*)user_;
	return (uint64_t)user->c->root;
}

static int _create_pointless_is_container(void* user_, uint64_t v_)
{
	pointless_create_t* c = ((_cycle_mark_create_t*)user_)->c;
	uint32_t owner, value;
	_unpack_map_and_vector(v_, &owner, &value);

	switch (cv_value_type(value)) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			if (cv_is_outside_vector(value))
				return 0;

			return 1;
		case POINTLESS_SET_VALUE:
		case POINTLESS_MAP_VALUE_VALUE:
			return 1;
	}

	return 0;
}

static uint32_t _create_pointless_container_id(void* user_, uint64_t v_)
{
	_cycle_mark_create_t* user = (_cycle_mark_create_t*)user_;
	pointless_create_t* c = ((_cycle_mark_create_t*)user_)->c;
	uint32_t owner, value;
	_unpack_map_and_vector(v_, &owner, &value);

	uint32_t ii = cv_value_data_u32(value);

	switch (cv_value_type(value)) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			assert(!cv_is_outside_vector(value));
			return ii;
		case POINTLESS_SET_VALUE:
			return ii
				+ pointless_dynarray_n_items(&user->c->priv_vector_values);
			;

		case POINTLESS_MAP_VALUE_VALUE:
			return ii
				+ pointless_dynarray_n_items(&user->c->priv_vector_values)
				+ pointless_dynarray_n_items(&user->c->set_values)
			;
	}

	assert(0);
	return 0;
}


static uint32_t _create_pointless_n_children(void* user_, uint64_t v_)
{
	pointless_create_t* c = ((_cycle_mark_create_t*)user_)->c;
	uint32_t owner, value;
	_unpack_map_and_vector(v_, &owner, &value);

	switch (cv_value_type(value)) {
		case POINTLESS_SET_VALUE:
			return 1;
		case POINTLESS_MAP_VALUE_VALUE:
			return 2;
	}

	if (owner == UINT32_MAX) {
		assert(!cv_is_outside_vector(value));
		return pointless_dynarray_n_items(&cv_priv_vector_at(value)->vector);
	}

	switch (cv_value_type(owner)) {
		case POINTLESS_SET_VALUE:
			assert(value == cv_set_at(owner)->serialize_keys);
			return pointless_dynarray_n_items(&cv_set_at(owner)->keys);
		case POINTLESS_MAP_VALUE_VALUE:
			assert(pointless_dynarray_n_items(&cv_map_at(owner)->keys) && pointless_dynarray_n_items(&cv_map_at(owner)->values));
			return pointless_dynarray_n_items(&cv_map_at(owner)->keys);
	}

	assert(0);
	return 0;
}

static uint64_t _create_pointless_child_at(void* user_, uint64_t v_, uint32_t i)
{
	pointless_create_t* c = ((_cycle_mark_create_t*)user_)->c;
	uint32_t owner, value, child;
	_unpack_map_and_vector(v_, &owner, &value);

	switch (cv_value_type(value)) {
		case POINTLESS_SET_VALUE:
			assert(owner == UINT32_MAX);
			assert(i == 0);
			return _pack_owner_and_value(value, cv_set_at(value)->serialize_keys);
		case POINTLESS_MAP_VALUE_VALUE:
			assert(i == 0 || i == 1);
			if (i == 0)
				return _pack_owner_and_value(value, cv_map_at(value)->serialize_keys);
			else
				return _pack_owner_and_value(value, cv_map_at(value)->serialize_values);
	}

	assert(!cv_is_outside_vector(value));

	if (owner == UINT32_MAX) {
		child = pointless_dynarray_ITEM_AT(uint32_t, &cv_priv_vector_at(value)->vector, i);
		return _pack_owner_and_value(UINT32_MAX, child);
	}

	// vector for set/map
	switch (cv_value_type(owner)) {
		case POINTLESS_SET_VALUE:
			assert(i == 0);
			child = pointless_dynarray_ITEM_AT(uint32_t, &cv_set_at(owner)->keys, i);
			return _pack_owner_and_value(UINT32_MAX, child);
		case POINTLESS_MAP_VALUE_VALUE:
			assert(i == 0 || i == 1);
			if (i == 0)
				child = pointless_dynarray_ITEM_AT(uint32_t, &cv_map_at(owner)->keys, i);
			else
				child = pointless_dynarray_ITEM_AT(uint32_t, &cv_map_at(owner)->values, i);
			return _pack_owner_and_value(UINT32_MAX, child);
	}

	assert(0);
	return 0;
}

void* pointless_cycle_marker_create(pointless_create_t* c, const char** error)
{
	_cycle_mark_create_t context;
	context.c = c;

	cycle_marker_info_t cb_info;
	cb_info.user = &context;
	cb_info.fn_n_nodes = _create_pointless_n_containers;
	cb_info.fn_get_root = _create_pointless_get_root;
	cb_info.fn_is_container = _create_pointless_is_container;
	cb_info.fn_container_id = _create_pointless_container_id;
	cb_info.fn_n_children = _create_pointless_n_children;
	cb_info.fn_child_at = _create_pointless_child_at;

	return pointless_cycle_marker(&cb_info, error);
}
