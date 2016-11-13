#include <pointless/pointless_reader_helpers.h>

typedef int (*check_k)(pointless_t* p, pointless_value_t* v, void* user);
typedef int (*check_v)(pointless_t* p, pointless_value_t* v, void* user, void* out);

typedef struct {
	uint8_t* s;
	size_t n;
} check_string_n_t;

static int check_string(pointless_t* p, pointless_value_t* v, void* user)
{
	uint8_t* key_s = (uint8_t*)user;

	if (v->type == POINTLESS_UNICODE_) {
		uint32_t* s = pointless_reader_unicode_value_ucs4(p, v);
		return (pointless_cmp_string_32_8(s, key_s) == 0);
	} else if (v->type == POINTLESS_STRING_) {
		uint8_t* s = pointless_reader_string_value_ascii(p, v);
		return (pointless_cmp_string_8_8(s, key_s) == 0);
	}

	return 0;
}

static int check_string_n(pointless_t* p, pointless_value_t* v, void* user)
{
	check_string_n_t* key = (check_string_n_t*)user;

	if (v->type == POINTLESS_UNICODE_) {
		uint32_t* s = pointless_reader_unicode_value_ucs4(p, v);
		return (pointless_cmp_string_32_8_n(s, key->s, key->n) == 0);
	} else if (v->type == POINTLESS_STRING_) {
		uint8_t* s = pointless_reader_string_value_ascii(p, v);
		return (pointless_cmp_string_8_8_n(s, key->s, key->n) == 0);
	}

	return 0;
}

static int check_unicode(pointless_t* p, pointless_value_t* v, void* user)
{
	uint32_t* key_s = (uint32_t*)user;

	if (v->type == POINTLESS_UNICODE_) {
		uint32_t* s = pointless_reader_unicode_value_ucs4(p, v);
		return (pointless_cmp_string_32_32(s, key_s) == 0);
	} else if (v->type == POINTLESS_STRING_) {
		uint8_t* s = pointless_reader_string_value_ascii(p, v);
		return (pointless_cmp_string_8_32(s, key_s) == 0);
	}

	return 0;
}

static int check_i64(pointless_t* p, pointless_value_t* v, void* user)
{
	if (pointless_is_integer_type(v->type)) {
		int64_t* user_v = (int64_t*)user;
		int64_t vv = pointless_get_int_as_int64(v->type, &v->data);

		if (*user_v == vv)
			return 1;
	}

	return 0;
}

static int check_and_get_u32(pointless_t* p, pointless_value_t* v, void* user, void* out)
{
	if (pointless_is_integer_type(v->type)) {
		int64_t iv = pointless_get_int_as_int64(v->type, &v->data);

		if (0 <= iv && iv <= UINT32_MAX) {
			uint32_t* p_out = (uint32_t*)out;
			*p_out = (uint32_t)iv;
			return 1;
		}
	}

	return 0;
}

static int check_and_get_i64(pointless_t* p, pointless_value_t* v, void* user, void* out)
{
	if (pointless_is_integer_type(v->type)) {
		int64_t* p_out = (int64_t*)out;
		*p_out = pointless_get_int_as_int64(v->type, &v->data);
		return 1;
	}

	return 0;
}

static int get_value(pointless_t* p, pointless_value_t* v, void* user, void* out)
{
	pointless_value_t* v_out = (pointless_value_t*)out;
	*v_out = *v;
	return 1;
}

static int pointless_get_map_(pointless_t* p, pointless_value_t* map, uint32_t hash, check_k cb_k, void* user_k, check_v cb_v, void* user_v, void* out)
{
	// this must be a map
	assert(map->type == POINTLESS_MAP_VALUE_VALUE);

	// our key/value
	pointless_value_t* kk = 0;
	pointless_value_t* vv = 0;

	// initialize iterator
	pointless_hash_iter_state_t iter_state;
	pointless_reader_map_iter_hash_init(p, map, hash, &iter_state);

	// iterate
	while (pointless_reader_map_iter_hash(p, map, hash, &kk, &vv, &iter_state)) {
		if ((*cb_k)(p, kk, user_k) && (*cb_v)(p, vv, user_v, out))
			return 1;
	}

	return 0;
}

static int pointless_get_set_(pointless_t* p, pointless_value_t* set, uint32_t hash, check_k cb_k, void* user_k)
{
	// this must be a set
	assert(set->type == POINTLESS_SET_VALUE);

	// our key
	pointless_value_t* kk = 0;

	// initalize iterator
	pointless_hash_iter_state_t iter_state;
	pointless_reader_set_iter_hash_init(p, set, hash, &iter_state);

	// iterate
	while (pointless_reader_set_iter_hash(p, set, hash, &kk, &iter_state)) {
		if ((*cb_k)(p, kk, user_k))
			return 1;
	}

	return 0;
}

int pointless_get_mapping_string_to_u32(pointless_t* p, pointless_value_t* map, char* key, uint32_t* value)
{
	uint32_t hash = pointless_hash_string_v1_32((uint8_t*)key);
	return pointless_get_map_(p, map, hash, check_string, (void*)key, check_and_get_u32, 0, (void*)value);
}

int pointless_get_mapping_string_to_i64(pointless_t* p, pointless_value_t* map, char* key, int64_t* value)
{
	uint32_t hash = pointless_hash_string_v1_32((uint8_t*)key);
	return pointless_get_map_(p, map, hash, check_string, (void*)key, check_and_get_i64, 0, (void*)value);
}

static int pointless_get_mapping_string_to_vector_(pointless_t* p, pointless_value_t* map, char* key, void** value, uint32_t* n_items, uint32_t vector_type)
{
	pointless_value_t v;

	if (!pointless_get_mapping_string_to_value(p, map, key, &v))
		return 0;

	if (v.type == POINTLESS_VECTOR_EMPTY) {
		*value = 0;
		*n_items = 0;
		return 1;
	}

	if (v.type != vector_type)
		return 0;

	switch (v.type) {
		case POINTLESS_VECTOR_I8:
			*value = (void*)pointless_reader_vector_i8(p, &v);
			break;
		case POINTLESS_VECTOR_U8:
			*value = (void*)pointless_reader_vector_u8(p, &v);
			break;
		case POINTLESS_VECTOR_I16:
			*value = (void*)pointless_reader_vector_i16(p, &v);
			break;
		case POINTLESS_VECTOR_U16:
			*value = (void*)pointless_reader_vector_u16(p, &v);
			break;
		case POINTLESS_VECTOR_I32:
			*value = (void*)pointless_reader_vector_i32(p, &v);
			break;
		case POINTLESS_VECTOR_U32:
			*value = (void*)pointless_reader_vector_u32(p, &v);
			break;
		case POINTLESS_VECTOR_I64:
			*value = (void*)pointless_reader_vector_i64(p, &v);
			break;
		case POINTLESS_VECTOR_U64:
			*value = (void*)pointless_reader_vector_u64(p, &v);
			break;
		case POINTLESS_VECTOR_FLOAT:
			*value = (void*)pointless_reader_vector_float(p, &v);
			break;
		default:
			return 0;
	}

	*n_items = pointless_reader_vector_n_items(p, &v);

	return 1;
}

int pointless_get_mapping_string_to_vector(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* v, uint32_t* n_items)
{
	if (!pointless_get_mapping_string_to_value(p, map, key, v))
		return 0;

	switch (v->type) {
		case POINTLESS_VECTOR_EMPTY:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U64:
		case POINTLESS_VECTOR_FLOAT:
			*n_items = pointless_reader_vector_n_items(p, v);
			return 1;
	}

	return 0;
}

int pointless_get_mapping_string_to_vector_i8(pointless_t* p, pointless_value_t* map, char* key, int8_t** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_I8); }
int pointless_get_mapping_string_to_vector_u8(pointless_t* p, pointless_value_t* map, char* key, uint8_t** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_U8); }
int pointless_get_mapping_string_to_vector_i16(pointless_t* p, pointless_value_t* map, char* key, int16_t** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_I16); }
int pointless_get_mapping_string_to_vector_u16(pointless_t* p, pointless_value_t* map, char* key, uint16_t** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_U16); }
int pointless_get_mapping_string_to_vector_i32(pointless_t* p, pointless_value_t* map, char* key, int32_t** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_I32); }
int pointless_get_mapping_string_to_vector_u32(pointless_t* p, pointless_value_t* map, char* key, uint32_t** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_U32); }
int pointless_get_mapping_string_to_vector_i64(pointless_t* p, pointless_value_t* map, char* key, int64_t** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_I64); }
int pointless_get_mapping_string_to_vector_u64(pointless_t* p, pointless_value_t* map, char* key, uint64_t** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_U64); }
int pointless_get_mapping_string_to_vector_float(pointless_t* p, pointless_value_t* map, char* key, float** value, uint32_t* n_items)
	{ return pointless_get_mapping_string_to_vector_(p, map, key, (void**)value, n_items, POINTLESS_VECTOR_FLOAT); }

int pointless_get_mapping_string_to_vector_value(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t** value, uint32_t* n_items)
{
	pointless_value_t v;

	if (!pointless_get_mapping_string_to_value(p, map, key, &v))
		return 0;

	if (v.type == POINTLESS_VECTOR_EMPTY) {
		*value = 0;
		*n_items = 0;
		return 1;
	}

	if (v.type != POINTLESS_VECTOR_VALUE && v.type != POINTLESS_VECTOR_VALUE_HASHABLE)
		return 0;

	*value = pointless_reader_vector_value(p, &v);
	*n_items = pointless_reader_vector_n_items(p, &v);

	return 1;
}

int pointless_get_mapping_string_to_value(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* value)
{
	uint32_t hash = pointless_hash_string_v1_32((uint8_t*)key);
	return pointless_get_map_(p, map, hash, check_string, (void*)key, get_value, 0, (void*)value);
}

int pointless_get_mapping_string_n_to_value(pointless_t* p, pointless_value_t* map, char* key, size_t n, pointless_value_t* value)
{
	uint32_t hash = pointless_hash_string_v1_32_((uint8_t*)key, n);

	check_string_n_t user;
	user.s = (uint8_t*)key;
	user.n = n;

	return pointless_get_map_(p, map, hash, check_string_n, (void*)&user, get_value, 0, (void*)value);
}

int pointless_get_mapping_unicode_to_value(pointless_t* p, pointless_value_t* map, uint32_t* key, pointless_value_t* value)
{
	uint32_t hash = pointless_hash_string_v1_32((uint8_t*)key);
	return pointless_get_map_(p, map, hash, check_unicode, (void*)key, get_value, 0, (void*)value);
}

int pointless_get_mapping_unicode_to_u32(pointless_t* p, pointless_value_t* map, uint32_t* key, uint32_t* value)
{
	uint32_t hash = pointless_hash_string_v1_32((uint8_t*)key);
	return pointless_get_map_(p, map, hash, check_unicode, (void*)key, check_and_get_u32, 0, (void*)value);
}


static int pointless_get_mapping_string_to_value_type(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* value, uint32_t type)
{
	uint32_t hash = pointless_hash_string_v1_32((uint8_t*)key);

	pointless_value_t v;

	if (!pointless_get_map_(p, map, hash, check_string, (void*)key, get_value, 0, (void*)&v))
		return 0;

	if (v.type == type) {
		*value = v;
		return 1;
	}

	return 0;
}

int pointless_get_mapping_string_to_set(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* s)
{
	return pointless_get_mapping_string_to_value_type(p, map, key, s, POINTLESS_SET_VALUE);
}

int pointless_get_mapping_string_to_map(pointless_t* p, pointless_value_t* map, char* key, pointless_value_t* m)
{
	return pointless_get_mapping_string_to_value_type(p, map, key, m, POINTLESS_MAP_VALUE_VALUE);
}

int pointless_get_mapping_int_to_value(pointless_t* p, pointless_value_t* map, int64_t i, pointless_value_t* v)
{
	uint32_t hash = pointless_hash_i64_32(i);
	return pointless_get_map_(p, map, hash, check_i64, (void*)&i, get_value, 0, (void*)v);
}

int pointless_is_int_in_set(pointless_t* p, pointless_value_t* set, int64_t i)
{
	uint32_t hash = pointless_hash_i64_32(i);
	return pointless_get_set_(p, set, hash, check_i64, (void*)&i);
}

int pointless_is_int_in_map(pointless_t* p, pointless_value_t* map, int64_t i)
{
	uint32_t hash = pointless_hash_i64_32(i);
	pointless_value_t v;
	return pointless_get_map_(p, map, hash, check_i64, (void*)&i, get_value, 0, (void*)&v);
}

int pointless_has_same_keys_set_map(pointless_t* p, pointless_value_t* s, pointless_value_t* m)
{
	assert(s->type == POINTLESS_SET_VALUE);
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);

	// trivial rejection, when number of keys is not the same
	if (pointless_reader_set_n_items(p, s) != pointless_reader_map_n_items(p, m))
		return 0;

	// we do two iterations, and then do a query in the other data structure
	uint32_t iter_state;
	pointless_value_t* kk = 0;
	pointless_value_t* vv = 0;

	iter_state = 0;

	while (pointless_reader_set_iter(p, s, &kk, &iter_state)) {
		if (!pointless_is_in_map_acyclic(p, m, kk))
			return 0;
	}

	iter_state = 0;

	while (pointless_reader_map_iter(p, m, &kk, &vv, &iter_state)) {
		if (!pointless_is_in_set_acyclic(p, s, kk))
			return 0;
	}

	return 1;
}

int pointless_has_same_keys_map_map(pointless_t* p, pointless_value_t* m_a, pointless_value_t* m_b)
{
	assert(m_a->type == POINTLESS_MAP_VALUE_VALUE);
	assert(m_b->type == POINTLESS_MAP_VALUE_VALUE);

	// trivial rejection, when number of keys is not the same
	if (pointless_reader_map_n_items(p, m_a) != pointless_reader_map_n_items(p, m_b))
		return 0;

	// we do two iterations, and then do a query in the other data structure
	uint32_t iter_state;
	pointless_value_t* kk = 0;
	pointless_value_t* vv = 0;

	iter_state = 0;

	while (pointless_reader_map_iter(p, m_a, &kk, &vv, &iter_state)) {
		if (!pointless_is_in_map_acyclic(p, m_b, kk))
			return 0;
	}

	iter_state = 0;

	while (pointless_reader_map_iter(p, m_b, &kk, &vv, &iter_state)) {
		if (!pointless_is_in_map_acyclic(p, m_a, kk))
			return 0;
	}

	return 1;
}

int pointless_is_in_set_acyclic(pointless_t* p, pointless_value_t* s, pointless_value_t* k)
{
	// get hash (guaranteed possible since value is acyclic)
	uint32_t hash = pointless_hash_reader_32(p, k);

	// start the iteration
	pointless_value_t* kk = 0;

	pointless_complete_value_t _k = pointless_value_to_complete(k);
	pointless_complete_value_t _kk;

	pointless_hash_iter_state_t iter_state;
	pointless_reader_set_iter_hash_init(p, s, hash, &iter_state);

	while (pointless_reader_set_iter_hash(p, s, hash, &kk, &iter_state)) {
		_kk = pointless_value_to_complete(kk);

		if (pointless_cmp_reader_acyclic(p, &_kk, p, &_k) == 0)
			return 1;
	}

	return 0;
}

int pointless_is_in_map_acyclic(pointless_t* p, pointless_value_t* m, pointless_value_t* k)
{
	// get hash (guaranteed possible since value is acyclic)
	uint32_t hash = pointless_hash_reader_32(p, k);

	// start the iteration
	pointless_value_t* kk = 0;
	pointless_value_t* vv = 0;

	pointless_complete_value_t _k = pointless_value_to_complete(k);
	pointless_complete_value_t _kk;

	pointless_hash_iter_state_t iter_state;
	pointless_reader_map_iter_hash_init(p, m, hash, &iter_state);

	while (pointless_reader_map_iter_hash(p, m, hash, &kk, &vv, &iter_state)) {
		_kk = pointless_value_to_complete(kk);
		if (pointless_cmp_reader_acyclic(p, &_kk, p, &_k) == 0)
			return 1;
	}

	return 0;
}
