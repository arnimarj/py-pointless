#include <math.h>

#include <pointless/pointless_defs.h>
#include <pointless/pointless_value.h>
#include <pointless/pointless_reader.h>
#include <pointless/pointless_bitvector.h>

#define PYTHON_STRING_HASH_32(s, T)     { T* p = (s); size_t len = 0; uint32_t x = (uint32_t)(*p) << 7; while (*p)        {x = (1000003 * x) ^ (uint32_t)(*p++); len++;} return x ^ len;}
#define PYTHON_STRING_HASH_32_(s, n, T) { T* p = (s); size_t len = 0; uint32_t x = (uint32_t)(*p) << 7; while (len < (n)) {x = (1000003 * x) ^ (uint32_t)(*p++); len++;} return x ^ len;}

uint32_t pointless_hash_string_v1_32(uint8_t* s)
{
	PYTHON_STRING_HASH_32(s, uint8_t)
}

uint32_t pointless_hash_string_v1_32_(uint8_t* s, size_t n)
{
	PYTHON_STRING_HASH_32_(s, n, uint8_t)
}

uint32_t pointless_hash_unicode_ucs2_v1_32(uint16_t* s)
{
	PYTHON_STRING_HASH_32(s, uint16_t)
}

uint32_t pointless_hash_unicode_ucs4_v1_32(uint32_t* s)
{
	PYTHON_STRING_HASH_32(s, uint32_t)
}

#define HASH_UNICODE_SEED 1000000001L

typedef uint32_t (*pointless_hash_reader_32_cb)(pointless_t* p, pointless_value_t* v);
typedef uint32_t (*pointless_hash_create_32_cb)(pointless_create_t* c, pointless_create_value_t* v);

// unicode is easy
static uint32_t pointless_hash_reader_unicode_32(pointless_t* p, pointless_value_t* v)
{
	uint32_t* s = pointless_reader_unicode_value_ucs4(p, v);
	return pointless_hash_unicode_ucs4_v1_32(s);
}

static uint32_t pointless_hash_create_unicode_32(pointless_create_t* c, pointless_create_value_t* v)
{
	uint32_t* s = ((uint32_t*)cv_get_unicode(v) + 1);
	return pointless_hash_unicode_ucs4_v1_32(s);
}

static uint32_t pointless_hash_reader_string_32(pointless_t* p, pointless_value_t* v)
{
	uint8_t* s = pointless_reader_string_value_ascii(p, v);
	return pointless_hash_string_v1_32(s);
}

static uint32_t pointless_hash_create_string_32(pointless_create_t* c, pointless_create_value_t* v)
{
	uint8_t* s = (uint8_t*)((uint32_t*)cv_get_string(v) + 1);
	return pointless_hash_string_v1_32(s);
}


// integers are easy
uint32_t pointless_hash_i32_32(int32_t v)
{
	union {
		int32_t i;
		uint32_t u;
	} h;

	h.i = v;
	return h.u;
}

uint32_t pointless_hash_u32_32(uint32_t v)
{
	return v;
}

uint32_t pointless_hash_i64_32(int64_t v)
{
	union {
		int64_t i;
		uint64_t u;
	} h;

	h.i = v;
	return (uint32_t)(h.u % UINT32_MAX);
}

uint32_t pointless_hash_u64_32(uint64_t v)
{
	return (uint32_t)(v % UINT32_MAX);
}

uint32_t pointless_hash_bool_true_32()
{
	return 1;
}

uint32_t pointless_hash_bool_false_32()
{
	return 0;
}

uint32_t pointless_hash_null_32()
{
	return 0;
}

static uint32_t pointless_hash_int_32(uint32_t t, pointless_value_data_t* v)
{
	switch (t) {
		case POINTLESS_I32:
			return pointless_hash_i32_32(v->data_i32);
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
			return pointless_hash_u32_32(v->data_u32);
	}

	assert(0);
	return 0;
}

static uint32_t pointless_hash_reader_int_32(pointless_t* p, pointless_value_t* v)
{
	return pointless_hash_int_32(v->type, &v->data);
}

static uint32_t pointless_hash_create_int_32(pointless_create_t* c, pointless_create_value_t* v)
{
	return pointless_hash_int_32(v->header.type_29, &v->data);
}

// floats are hard
uint32_t pointless_hash_float_32(float f)
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

static uint32_t pointless_hash_reader_float_32(pointless_t* p, pointless_value_t* v)
{
	return pointless_hash_float_32(v->data.data_f);
}

static uint32_t pointless_hash_create_float_32(pointless_create_t* c, pointless_create_value_t* v)
{
	return pointless_hash_float_32(v->data.data_f);
}

// bitvectors are fairly easy
static uint32_t pointless_hash_reader_bitvector_32(pointless_t* p, pointless_value_t* v)
{
	void* buffer = 0;

	if (v->type == POINTLESS_BITVECTOR)
		buffer = pointless_reader_bitvector_buffer(p, v);

	return pointless_bitvector_hash_32(v->type, &v->data, buffer);
}

static uint32_t pointless_hash_create_bitvector_32(pointless_create_t* c, pointless_create_value_t* v)
{
	void* buffer = 0;

	if (v->header.type_29 == POINTLESS_BITVECTOR)
		buffer = cv_get_bitvector(v);

	return pointless_bitvector_hash_32(v->header.type_29, &v->data, buffer);
}

// nulls and empty_slot always return 0
static uint32_t pointless_hash_reader_null_32(pointless_t* p, pointless_value_t* v)
	{ return 0; }
static uint32_t pointless_hash_create_null_32(pointless_create_t* c, pointless_create_value_t* v)
	{ return 0; }
static uint32_t pointless_hash_reader_empty_slot_32(pointless_t* p, pointless_value_t* v)
	{ return 0; }
static uint32_t pointless_hash_create_empty_slot_32(pointless_create_t* c, pointless_create_value_t* v)
	{ return 0; }

void pointless_vector_hash_init_32(pointless_vector_hash_state_32_t* state, uint32_t len)
{
	state->mult = 1000003;
	state->x = 0x345678;
	state->len = len;
}

void pointless_vector_hash_next_32(pointless_vector_hash_state_32_t* state, uint32_t hash)
{
	state->x = (state->x ^ hash) * state->mult;
	state->mult += (82520 + state->len + state->len);
}

uint32_t pointless_vector_hash_end_32(pointless_vector_hash_state_32_t* state)
{
	state->x += 97531;
	return state->x;
}

static uint32_t pointless_hash_reader_vector_32_priv(pointless_t* p, pointless_value_t* v, uint32_t offset, uint32_t n_items)
{
	uint32_t h, i;
	pointless_vector_hash_state_32_t state;
	pointless_vector_hash_init_32(&state, n_items);

	for (i = offset; i < n_items + offset; i++) {
		switch (v->type) {
			case POINTLESS_VECTOR_VALUE_HASHABLE:
				h = pointless_hash_reader_32(p, pointless_reader_vector_value(p, v) + i);
				break;
			case POINTLESS_VECTOR_I8:
				h = pointless_hash_i32_32((int32_t)(pointless_reader_vector_i8(p, v)[i]));
				break;
			case POINTLESS_VECTOR_U8:
				h = pointless_hash_u32_32((uint32_t)(pointless_reader_vector_u8(p, v)[i]));
				break;
			case POINTLESS_VECTOR_I16:
				h = pointless_hash_i32_32((int32_t)(pointless_reader_vector_i16(p, v)[i]));
				break;
			case POINTLESS_VECTOR_U16:
				h = pointless_hash_u32_32((uint32_t)(pointless_reader_vector_u16(p, v)[i]));
				break;
			case POINTLESS_VECTOR_I32:
				h = pointless_hash_i32_32(pointless_reader_vector_i32(p, v)[i]);
				break;
			case POINTLESS_VECTOR_U32:
				h = pointless_hash_u32_32(pointless_reader_vector_u32(p, v)[i]);
				break;
			case POINTLESS_VECTOR_I64:
				h = pointless_hash_i32_32((int32_t)(pointless_reader_vector_i64(p, v)[i]));
				break;
			case POINTLESS_VECTOR_U64:
				h = pointless_hash_u32_32((uint32_t)(pointless_reader_vector_u64(p, v)[i]));
				break;
			case POINTLESS_VECTOR_FLOAT:
				h = pointless_hash_float_32(pointless_reader_vector_float(p, v)[i]);
				break;
			default:
				h = 0;
				assert(0);
				break;
		}

		pointless_vector_hash_next_32(&state, h);
	}

	return pointless_vector_hash_end_32(&state);
}

static uint32_t pointless_hash_reader_vector_32_(pointless_t* p, pointless_value_t* v)
{
	uint32_t n_items = pointless_reader_vector_n_items(p, v);
	return pointless_hash_reader_vector_32_priv(p, v, 0, n_items);
}


static uint32_t pointless_hash_create_vector_32(pointless_create_t* c, pointless_create_value_t* v)
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

	pointless_vector_hash_state_32_t state;
	pointless_vector_hash_init_32(&state, n_items);

	for (i = 0; i < n_items; i++) {
		// if vector was value based, but is now compressed, we must typecast all values
		if (v->header.is_compressed_vector) {
			vv = &pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, ((uint32_t*)items)[i]);

			switch (v->header.type_29) {
				case POINTLESS_VECTOR_I8:
				case POINTLESS_VECTOR_I16:
				case POINTLESS_VECTOR_I32:
					h = pointless_hash_i32_32((int32_t)pointless_get_int_as_int64(vv->header.type_29, &vv->data));
					break;
				case POINTLESS_VECTOR_U8:
				case POINTLESS_VECTOR_U16:
				case POINTLESS_VECTOR_U32:
					h = pointless_hash_u32_32((uint32_t)pointless_get_int_as_int64(vv->header.type_29, &vv->data));
					break;
				case POINTLESS_VECTOR_FLOAT:
					h = pointless_hash_float_32(pointless_value_get_float(vv->header.type_29, &vv->data));
					break;
				// value based vectors can't hold 64-bit values
				case POINTLESS_VECTOR_U64:
				case POINTLESS_VECTOR_I64:
					h = 0;
					assert(0);
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
					h = pointless_hash_create_32(c, vv);
					break;
				case POINTLESS_VECTOR_I8:
					h = pointless_hash_i32_32((int32_t)(((int8_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_U8:
					h = pointless_hash_u32_32((uint32_t)(((uint8_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_I16:
					h = pointless_hash_i32_32((int32_t)(((int16_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_U16:
					h = pointless_hash_u32_32((uint32_t)(((uint16_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_I32:
					h = pointless_hash_i32_32(((int32_t*)items)[i]);
					break;
				case POINTLESS_VECTOR_U32:
					h = pointless_hash_u32_32(((uint32_t*)items)[i]);
					break;
				case POINTLESS_VECTOR_I64:
					h = pointless_hash_i32_32((int32_t)(((int64_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_U64:
					h = pointless_hash_u32_32((uint32_t)(((uint64_t*)items)[i]));
					break;
				case POINTLESS_VECTOR_FLOAT:
					h = pointless_hash_float_32(((float*)items)[i]);
					break;
				default:
					h = 0;
					assert(0);
					break;
			}
		}

		pointless_vector_hash_next_32(&state, h);
	}

	return pointless_vector_hash_end_32(&state);
}

static pointless_hash_reader_32_cb pointless_hash_reader_func_32(uint32_t t)
{
	switch (t) {
		case POINTLESS_UNICODE_:
			return pointless_hash_reader_unicode_32;
		case POINTLESS_STRING_:
			return pointless_hash_reader_string_32;
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
			return pointless_hash_reader_int_32;
		case POINTLESS_FLOAT:
			return pointless_hash_reader_float_32;
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			return pointless_hash_reader_bitvector_32;
		case POINTLESS_NULL:
			return pointless_hash_reader_null_32;
		case POINTLESS_VECTOR_VALUE:
			return 0;
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U64:
		case POINTLESS_VECTOR_FLOAT:
		case POINTLESS_VECTOR_EMPTY:
			return pointless_hash_reader_vector_32_;
		case POINTLESS_SET_VALUE:
		case POINTLESS_MAP_VALUE_VALUE:
			return 0;
		case POINTLESS_EMPTY_SLOT:
			return pointless_hash_reader_empty_slot_32;
	}

	assert(0);
	return 0;
}

static pointless_hash_create_32_cb pointless_hash_create_func_32(uint32_t t)
{
	switch (t) {
		case POINTLESS_UNICODE_:
			return pointless_hash_create_unicode_32;
		case POINTLESS_STRING_:
			return pointless_hash_create_string_32;
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
			return pointless_hash_create_int_32;
		case POINTLESS_FLOAT:
			return pointless_hash_create_float_32;
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			return pointless_hash_create_bitvector_32;
		case POINTLESS_NULL:
			return pointless_hash_create_null_32;
		case POINTLESS_VECTOR_VALUE:
			return 0;
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U64:
		case POINTLESS_VECTOR_FLOAT:
		case POINTLESS_VECTOR_EMPTY:
			return pointless_hash_create_vector_32;
		case POINTLESS_SET_VALUE:
		case POINTLESS_MAP_VALUE_VALUE:
			return 0;
		case POINTLESS_EMPTY_SLOT:
			return pointless_hash_create_empty_slot_32;
	}

	assert(0);
	return 0;
}

uint32_t pointless_is_hashable(uint32_t type)
{
	return (pointless_hash_reader_func_32(type) != 0);
}

uint32_t pointless_hash_reader_32(pointless_t* p, pointless_value_t* v)
{
	assert(pointless_is_hashable(v->type));
	pointless_hash_reader_32_cb cb = pointless_hash_reader_func_32(v->type);
	return (*cb)(p, v);
}

uint32_t pointless_hash_reader_vector_32(pointless_t* p, pointless_value_t* v, uint32_t i, uint32_t n)
{
	return pointless_hash_reader_vector_32_priv(p, v, i, n);
}

uint32_t pointless_hash_create_32(pointless_create_t* c, pointless_create_value_t* v)
{
	assert(pointless_is_hashable(v->header.type_29));
	pointless_hash_create_32_cb cb = pointless_hash_create_func_32(v->header.type_29);
	return (*cb)(c, v);
}
