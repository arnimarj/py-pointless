#include <pointless/pointless_defs.h>
#include <pointless/pointless_value.h>
#include <pointless/pointless_reader.h>
#include <pointless/pointless_bitvector.h>

#define PYTHON_STRING_HASH(s, T) { T* p = (s); size_t len = 0; uint32_t x = (uint32_t)(*p) << 7; while (*p) {x = (1000003 * x) ^ (uint32_t)(*p++); len++;} return x ^ len;}

uint32_t pointless_hash_string_v1(uint8_t* s)
{
	PYTHON_STRING_HASH(s, uint8_t)
}

uint32_t pointless_hash_unicode_ucs2_v1(uint16_t* s)
{
	PYTHON_STRING_HASH(s, uint16_t)
}

uint32_t pointless_hash_unicode_ucs4_v1(uint32_t* s)
{
	PYTHON_STRING_HASH(s, uint32_t)
}

#define HASH_UNICODE_SEED 1000000001L

#define POINTLESS_HASH_STRING(s) uint32_t v = 1; while (*(s)) { v = v + HASH_UNICODE_SEED + *(s); (s) += 1; } return v

uint32_t pointless_hash_unicode_ucs4_v0(uint32_t* s)
{
	POINTLESS_HASH_STRING(s);
}

uint32_t pointless_hash_unicode_ucs2_v0(uint16_t* s)
{
	POINTLESS_HASH_STRING(s);
}

uint32_t pointless_hash_string_v0(uint8_t* s)
{
	POINTLESS_HASH_STRING(s);
}

typedef uint32_t (*pointless_hash_reader_cb)(pointless_t* p, pointless_value_t* v);
typedef uint32_t (*pointless_hash_create_cb)(pointless_create_t* c, pointless_create_value_t* v);

// unicode is easy
static uint32_t pointless_hash_reader_unicode(pointless_t* p, pointless_value_t* v)
{
	uint32_t* s = pointless_reader_unicode_value_ucs4(p, v);
	uint32_t hash = 0;

	switch (p->header->version) {
		case 0: hash = pointless_hash_unicode_ucs4_v0(s); break;
		case 1: hash = pointless_hash_unicode_ucs4_v1(s); break;
	}

	return hash;
}

static uint32_t pointless_hash_create_unicode(pointless_create_t* c, pointless_create_value_t* v)
{
	uint32_t* s = ((uint32_t*)cv_get_unicode(v) + 1);

	uint32_t hash = 0;

	switch (c->version) {
		case 0: hash = pointless_hash_unicode_ucs4_v0(s); break;
		case 1: hash = pointless_hash_unicode_ucs4_v1(s); break;
	}

	return hash;
}

// integers are easy
uint32_t pointless_hash_i32(int32_t v)
{
	union {
		int32_t i;
		uint32_t u;
	} h;

	h.i = v;
	return h.u;
}

uint32_t pointless_hash_u32(uint32_t v)
{
	return v;
}

uint32_t pointless_hash_i64(int64_t v)
{
	union {
		int64_t i;
		uint64_t u;
	} h;

	h.i = v;
	return (uint32_t)(h.u % UINT32_MAX);
}

uint32_t pointless_hash_u64(uint64_t v)
{
	return (uint32_t)(v % UINT32_MAX);
}

uint32_t pointless_hash_bool_true()
{
	return 1;
}

uint32_t pointless_hash_bool_false()
{
	return 0;
}

uint32_t pointless_hash_null()
{
	return 0;
}

static uint32_t pointless_hash_int(uint32_t t, pointless_value_data_t* v)
{
	switch (t) {
		case POINTLESS_I32:
			return pointless_hash_i32(v->data_i32);
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
			return pointless_hash_u32(v->data_u32);
	}

	assert(0);
	return 0;
}

static uint32_t pointless_hash_reader_int(pointless_t* p, pointless_value_t* v)
{
	return pointless_hash_int(v->type, &v->data);
}

static uint32_t pointless_hash_create_int(pointless_create_t* c, pointless_create_value_t* v)
{
	return pointless_hash_int(v->header.type_29, &v->data);
}

// floats are hard
uint32_t pointless_hash_float(float f)
{
	// hash of doubles with .0 fraction must match that of integers
	double d = (double)f, di;
	double frac = modf(d, &di);

	union {
		uint32_t u32;
		int32_t i32;
		float f;
	} hash;

	hash.f = f;

	if (frac == 0.0) {
		if (d < 0.0 && (INT32_MIN <= di && di <= INT32_MAX)) {
			hash.i32 = (int32_t)di;
			return hash.u32;
		} else if (0.0 <= d && di <= UINT32_MAX) {
			hash.u32 = (uint32_t)di;
			return hash.u32;
		}
	}

	return hash.u32;
}

static uint32_t pointless_hash_reader_float(pointless_t* p, pointless_value_t* v)
{
	return pointless_hash_float(v->data.data_f);
}

static uint32_t pointless_hash_create_float(pointless_create_t* c, pointless_create_value_t* v)
{
	return pointless_hash_float(v->data.data_f);
}

// bitvectors are fairly easy
static uint32_t pointless_hash_reader_bitvector(pointless_t* p, pointless_value_t* v)
{
	void* buffer = 0;

	if (v->type == POINTLESS_BITVECTOR)
		buffer = pointless_reader_bitvector_buffer(p, v);

	return pointless_bitvector_hash(v->type, &v->data, buffer);
}

static uint32_t pointless_hash_create_bitvector(pointless_create_t* c, pointless_create_value_t* v)
{
	void* buffer = 0;

	if (v->header.type_29 == POINTLESS_BITVECTOR)
		buffer = cv_get_unicode(v);

	return pointless_bitvector_hash(v->header.type_29, &v->data, buffer);
}

// nulls and empty_slot always return 0
static uint32_t pointless_hash_reader_null(pointless_t* p, pointless_value_t* v)
	{ return 0; }
static uint32_t pointless_hash_create_null(pointless_create_t* c, pointless_create_value_t* v)
	{ return 0; }
static uint32_t pointless_hash_reader_empty_slot(pointless_t* p, pointless_value_t* v)
	{ return 0; }
static uint32_t pointless_hash_create_empty_slot(pointless_create_t* c, pointless_create_value_t* v)
	{ return 0; }

void pointless_vector_hash_init(pointless_vector_hash_state_t* state, uint32_t len)
{
	state->mult = 1000003;
	state->x = 0x345678;
	state->len = len;
}

void pointless_vector_hash_next(pointless_vector_hash_state_t* state, uint32_t hash)
{
	state->x = (state->x ^ hash) * state->mult;
	state->mult += (82520 + state->len + state->len);
}

uint32_t pointless_vector_hash_end(pointless_vector_hash_state_t* state)
{
	state->x += 97531;
	return state->x;
}

static uint32_t pointless_hash_reader_vector(pointless_t* p, pointless_value_t* v)
{
	uint32_t h, i, n_items = pointless_reader_vector_n_items(p, v);
	pointless_vector_hash_state_t state;
	pointless_vector_hash_init(&state, n_items);

	for (i = 0; i < n_items; i++) {
		switch (v->type) {
			case POINTLESS_VECTOR_VALUE_HASHABLE:
				h = pointless_hash_reader(p, &pointless_reader_vector_value(p, v)[i]);
				break;
			case POINTLESS_VECTOR_I8:
				h = pointless_hash_i32((int32_t)(pointless_reader_vector_i8(p, v)[i]));
				break;
			case POINTLESS_VECTOR_U8:
				h = pointless_hash_u32((uint32_t)(pointless_reader_vector_u8(p, v)[i]));
				break;
			case POINTLESS_VECTOR_I16:
				h = pointless_hash_i32((int32_t)(pointless_reader_vector_i16(p, v)[i]));
				break;
			case POINTLESS_VECTOR_U16:
				h = pointless_hash_u32((uint32_t)(pointless_reader_vector_u16(p, v)[i]));
				break;
			case POINTLESS_VECTOR_I32:
				h = pointless_hash_i32(pointless_reader_vector_i32(p, v)[i]);
				break;
			case POINTLESS_VECTOR_U32:
				h = pointless_hash_u32(pointless_reader_vector_i32(p, v)[i]);
				break;
			case POINTLESS_VECTOR_FLOAT:
				h = pointless_hash_float(pointless_reader_vector_float(p, v)[i]);
				break;
			default:
				h = 0;
				assert(0);
				break;
		}

		pointless_vector_hash_next(&state, h);
	}

	return pointless_vector_hash_end(&state);
}

static uint32_t pointless_hash_create_vector(pointless_create_t* c, pointless_create_value_t* v)
{
	uint32_t h, i, n_items;
	void* items;
	pointless_create_value_t* vv;

	if (v->header.is_outside_vector) {
		items = cv_get_outside_vector(v)->items;
		n_items = cv_get_outside_vector(v)->n_items;
	} else {
		// WARNING: using direct pointer to a dynamic array
		items = cv_get_priv_vector(v)->vector._data;
		n_items = pointless_dynarray_n_items(&cv_get_priv_vector(v)->vector);
	}

	pointless_vector_hash_state_t state;
	pointless_vector_hash_init(&state, n_items);

	for (i = 0; i < n_items; i++) {
		// if vector was value based, but is now compressed, we must typecast all values
		if (v->header.is_compressed_vector) {
			vv = &pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, ((uint32_t*)items)[i]);

			switch (v->header.type_29) {
				case POINTLESS_VECTOR_I8:
				case POINTLESS_VECTOR_I16:
				case POINTLESS_VECTOR_I32:
					h = pointless_hash_i32((int32_t)pointless_get_int_as_int64(vv->header.type_29, &vv->data));
					break;
				case POINTLESS_VECTOR_U8:
				case POINTLESS_VECTOR_U16:
				case POINTLESS_VECTOR_U32:
					h = pointless_hash_u32((uint32_t)pointless_get_int_as_int64(vv->header.type_29, &vv->data));
					break;
				case POINTLESS_VECTOR_FLOAT:
					h = pointless_hash_float(pointless_value_get_float(vv->header.type_29, &vv->data));
					break;
				default:
					h = 0;
					assert(0);
					break;
			}
		} else {
			switch (v->header.type_29) {
				case POINTLESS_VECTOR_VALUE_HASHABLE:
					vv = &pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, ((uint32_t*)items)[i]);
					h = pointless_hash_create(c, vv);
					break;
				case POINTLESS_VECTOR_I8:
					h = pointless_hash_i32((int32_t)(((int8_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_U8:
					h = pointless_hash_u32((uint32_t)(((uint8_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_I16:
					h = pointless_hash_i32((int32_t)(((int16_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_U16:
					h = pointless_hash_u32((uint32_t)(((uint16_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_I32:
					h = pointless_hash_i32(((int32_t*)items)[i]);
					break;
				case POINTLESS_VECTOR_U32:
					h = pointless_hash_u32(((uint32_t*)items)[i]);
					break;
				case POINTLESS_VECTOR_FLOAT:
					h = pointless_hash_float(((float*)items)[i]);
					break;
				default:
					h = 0;
					assert(0);
					break;
			}
		}

		pointless_vector_hash_next(&state, h);
	}

	return pointless_vector_hash_end(&state);
}

static pointless_hash_reader_cb pointless_hash_reader_func(uint32_t t)
{
	switch (t) {
		case POINTLESS_UNICODE:
			return pointless_hash_reader_unicode;
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
			return pointless_hash_reader_int;
		case POINTLESS_FLOAT:
			return pointless_hash_reader_float;
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			return pointless_hash_reader_bitvector;
		case POINTLESS_NULL:
			return pointless_hash_reader_null;
		case POINTLESS_VECTOR_VALUE:
			return 0;
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_FLOAT:
		case POINTLESS_VECTOR_EMPTY:
			return pointless_hash_reader_vector;
		case POINTLESS_SET_VALUE:
		case POINTLESS_MAP_VALUE_VALUE:
			return 0;
		case POINTLESS_EMPTY_SLOT:
			return pointless_hash_reader_empty_slot;
	}

	assert(0);
	return 0;
}

static pointless_hash_create_cb pointless_hash_create_func(uint32_t t)
{
	switch (t) {
		case POINTLESS_UNICODE:
			return pointless_hash_create_unicode;
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
			return pointless_hash_create_int;
		case POINTLESS_FLOAT:
			return pointless_hash_create_float;
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			return pointless_hash_create_bitvector;
		case POINTLESS_NULL:
			return pointless_hash_create_null;
		case POINTLESS_VECTOR_VALUE:
			return 0;
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_FLOAT:
		case POINTLESS_VECTOR_EMPTY:
			return pointless_hash_create_vector;
		case POINTLESS_SET_VALUE:
		case POINTLESS_MAP_VALUE_VALUE:
			return 0;
		case POINTLESS_EMPTY_SLOT:
			return pointless_hash_create_empty_slot;
	}

	assert(0);
	return 0;
}

uint32_t pointless_is_hashable(uint32_t type)
{
	return (pointless_hash_reader_func(type) != 0);
}

uint32_t pointless_hash_reader(pointless_t* p, pointless_value_t* v)
{
	assert(pointless_is_hashable(v->type));
	pointless_hash_reader_cb cb = pointless_hash_reader_func(v->type);
	return (*cb)(p, v);
}

uint32_t pointless_hash_create(pointless_create_t* c, pointless_create_value_t* v)
{
	assert(pointless_is_hashable(v->header.type_29));
	pointless_hash_create_cb cb = pointless_hash_create_func(v->header.type_29);
	return (*cb)(c, v);
}
