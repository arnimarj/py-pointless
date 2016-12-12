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



static uint32_t _create_pointless_n_containers(void* user_)
{
	_cycle_mark_create_t* user = (_cycle_mark_create_t*)user_;
	uint32_t n = 0;
	n += pointless_dynarray_n_items(&user->c->priv_vector_values);
	n += pointless_dynarray_n_items(&user->c->set_values);
	n += pointless_dynarray_n_items(&user->c->map_values);
	return n;
}

static void* _create_pointless_get_root(void* user_)
{
	_cycle_mark_create_t* user = (_cycle_mark_create_t*)user_;
	return (void*)user->c->root;
}

static int _create_pointless_is_container(void* user_, void* v_)
{
	pointless_create_t* c = ((_cycle_mark_create_t*)user_)->c;
	uint32_t v = (uint32_t)v_;

	switch (cv_value_type(v)) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			if (cv_is_outside_vector(v))
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
	uint32_t v_sub_value = (((uint64_t)v_) >> 32) & 0xFFFFFFFF;
	uint32_t v           = (((uint64_t)v_) >> 0 ) & 0xFFFFFFFF;
	uint32_t v_effective = v;

	switch (cv_value_type(v)) {
		case POINTLESS_SET_VALUE:
			assert(0 <= v_sub_value && v_sub_value <= 2);

			if (v_sub_value == 1)
				v_effective = cv_set_at(v)->serialize_hash;
			else if (v_sub_value == 2)
				v_effective = cv_set_at(v)->serialize_keys;

			break;
		case POINTLESS_MAP_VALUE_VALUE:
			assert(0 <= v_sub_value && v_sub_value <= 3);

			if (v_sub_value == 1)
				v_effective = cv_map_at(v)->serialize_hash;
			else if (v_sub_value == 2)
				v_effective = cv_map_at(v)->serialize_keys;
			else if (v_sub_value == 3)
				v_effective = cv_map_at(v)->serialize_values;
			break;
	}

	uint32_t ii = cv_value_data_u32(v);

	switch (cv_value_type(v_effective)) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			assert(!cv_is_outside_vector(v_effective));
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
	uint32_t v_sub_value = (((uint64_t)v_) >> 32) & 0xFFFFFFFF;
	uint32_t v           = (((uint64_t)v_) >> 0 ) & 0xFFFFFFFF;

	switch (cv_value_type(v)) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			assert(v_sub_value == 0);
			assert(!cv_is_outside_vector(v));
			return pointless_dynarray_n_items(&cv_priv_vector_at(v)->vector);
		case POINTLESS_SET_VALUE:
			assert(0 <= v_sub_value && v_sub_value <= 2);
			return 2;
		case POINTLESS_MAP_VALUE_VALUE:
			assert(0 <= v_sub_value && v_sub_value <= 3);
			return 3;
	}

	assert(0);
	return 0;
}

static uint64_t _create_pointless_child_at(void* user_, uint64_t v_, uint32_t i)
{
	pointless_create_t* c = ((_cycle_mark_create_t*)user_)->c;

	uint32_t v_sub_value = (((uint64_t)v_) >> 32) & 0xFFFFFFFF;
	uint32_t v           = (((uint64_t)v_) >> 0 ) & 0xFFFFFFFF;

	switch (cv_value_type(v)) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			assert(v_sub_value == 0);
			assert(!cv_is_outside_vector(v));
			return pointless_dynarray_n_items(&cv_priv_vector_at(v)->vector);

		case POINTLESS_SET_VALUE:
			if (v_sub_value == 0) {
				assert(0 <= i <= 1);
				if (i == 0)
					return (uint64_t)cv_set_at(v_effective)->serialize_hash;
				else
					return (uint64_t)cv_set_at(v_effective)->serialize_keys;
			} else {
				xxx
			}
			break;

		case POINTLESS_MAP_VALUE_VALUE:
			assert(0 <= i <= 2);
			if (i == 0)
				return (uint64_t)cv_map_at(v_effective)->serialize_hash;
			else if (i == 1)
				return (uint64_t)cv_map_at(v_effective)->serialize_keys;
			else
				return (uint64_t)cv_map_at(v_effective)->serialize_values;
			break;
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
