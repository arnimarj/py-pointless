#include <pointless/pointless_reader.h>
#include <pointless/pointless_bitvector.h>
#include <pointless/pointless_value.h>

static int32_t pointless_cmp_create_rec_priv(pointless_create_t* c, pointless_create_value_t* v_a, pointless_create_value_t* v_b, uint32_t depth, const char** error);

#define POINTLESS_CMP_STRING(a, b) while ((uint32_t)(*(a)) == (uint32_t)(*(b))) { if (*(a) == 0) return 0; (a)++; (b)++;} return SIMPLE_CMP(*(a), *(b));

#ifdef POINTLESS_WCHAR_T_IS_4_BYTES
int32_t pointless_cmp_unicode_wchar_wchar(wchar_t* a, wchar_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
#endif

int32_t pointless_cmp_unicode_ucs4_ucs4(uint32_t* a, uint32_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_unicode_ucs2_ucs4(uint16_t* a, uint32_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_unicode_ucs4_ucs2(uint32_t* a, uint16_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_unicode_ascii_ucs4(uint8_t* a, uint32_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_unicode_ucs4_ascii(uint32_t* a, uint8_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_unicode_ascii_ascii(uint8_t* a, uint8_t* b)
	{ POINTLESS_CMP_STRING(a, b); }

typedef int32_t (*pointless_cmp_reader_cb)(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error);
typedef int32_t (*pointless_cmp_create_cb)(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error);

static int32_t pointless_cmp_reader_rec(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error);
static int32_t pointless_cmp_create_rec(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error);

// null and empty-set comparison are always the same
static int32_t pointless_cmp_reader_null(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
	{ return 0; }
static int32_t pointless_cmp_reader_empty_slot(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
	{ return 0; }
static int32_t pointless_cmp_create_empty_slot(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
	{ return 0; }
static int32_t pointless_cmp_create_null(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
	{ return 0; }

// unicodes are fairly simple
static int32_t pointless_cmp_reader_unicode(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
{
	uint32_t* unicode_a = pointless_reader_unicode_value_ucs4(p_a, a);
	uint32_t* unicode_b = pointless_reader_unicode_value_ucs4(p_b, b);

	return pointless_cmp_unicode_ucs4_ucs4(unicode_a, unicode_b);
}

static int32_t pointless_cmp_create_unicode(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
{
	uint32_t* unicode_a = (uint32_t*)cv_get_unicode(a) + 1;
	uint32_t* unicode_b = (uint32_t*)cv_get_unicode(b) + 1;

	return pointless_cmp_unicode_ucs4_ucs4(unicode_a, unicode_b);
}

// int/float is absolutely trivial
static int64_t pointless_int_value(uint32_t t, pointless_value_data_t* v)
{
	switch (t) {
		case POINTLESS_I32:
			return (int64_t)pointless_value_get_i32(t, v);
		case POINTLESS_U32:
			return (int64_t)pointless_value_get_u32(t, v);
		case POINTLESS_BOOLEAN:
			return (int64_t)pointless_value_get_bool(t, v);
	}

	assert(0);
	return 0;
}

static int32_t pointless_cmp_int_float(uint32_t t_a, pointless_value_data_t* v_a, uint32_t t_b, pointless_value_data_t* v_b)
{
	// float, float
	if (t_a == POINTLESS_FLOAT && t_b == POINTLESS_FLOAT) {
		return SIMPLE_CMP(v_a->data_f, v_b->data_f);
	// float, int
	} else if (t_a == POINTLESS_FLOAT) {
		return SIMPLE_CMP(v_a->data_f, (float)pointless_int_value(t_b, v_b));
	// int, float
	} else if (t_b == POINTLESS_FLOAT) {
		return SIMPLE_CMP((float)pointless_int_value(t_a, v_a), v_b->data_f);
	// int, int
	} else {
		return SIMPLE_CMP(pointless_int_value(t_a, v_a), pointless_int_value(t_b, v_b));
	}
}

static int32_t pointless_cmp_reader_int_float(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
{
	return pointless_cmp_int_float(a->type, &a->data, b->type, &b->data);
}

static int32_t pointless_cmp_create_int_float(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
{
	return pointless_cmp_int_float(a->header.type_29, &a->data, b->header.type_29, &b->data);
}

// bitvectors are fairly simple
static int32_t pointless_cmp_reader_bitvector(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
{
	void* buffer_a = 0;
	void* buffer_b = 0;

	if (a->type == POINTLESS_BITVECTOR)
		buffer_a = pointless_reader_bitvector_buffer(p_a, a);

	if (b->type == POINTLESS_BITVECTOR)
		buffer_b = pointless_reader_bitvector_buffer(p_b, b);

	return pointless_bitvector_cmp_buffer_buffer(a->type, &a->data, buffer_a, b->type, &a->data, buffer_b);
}

static int32_t pointless_cmp_create_bitvector(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
{
	void* buffer_a = 0;
	void* buffer_b = 0;

	if (a->header.type_29 == POINTLESS_BITVECTOR)
		buffer_a = cv_get_bitvector(a);

	if (b->header.type_29 == POINTLESS_BITVECTOR)
		buffer_b = cv_get_bitvector(b);

	return pointless_bitvector_cmp_buffer_buffer(a->header.type_29, &a->data, buffer_a, b->header.type_29, &b->data, buffer_b);
}

// vectors are complicated
static pointless_value_t pointless_cmp_vector_value_reader(pointless_t* p, pointless_value_t* v, uint32_t i)
{
	pointless_value_t vi = pointless_value_create_as_read_null();

	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			return pointless_reader_vector_value(p, v)[i];
		case POINTLESS_VECTOR_I8:
			return pointless_value_create_as_read_i32((int32_t)pointless_reader_vector_i8(p, v)[i]);
		case POINTLESS_VECTOR_U8:
			return pointless_value_create_as_read_u32((uint32_t)pointless_reader_vector_u8(p, v)[i]);
		case POINTLESS_VECTOR_I16:
			return pointless_value_create_as_read_i32((int32_t)pointless_reader_vector_i16(p, v)[i]);
		case POINTLESS_VECTOR_U16:
			return pointless_value_create_as_read_u32((uint32_t)pointless_reader_vector_u16(p, v)[i]);
		case POINTLESS_VECTOR_I32:
			return pointless_value_create_as_read_i32((int32_t)pointless_reader_vector_i32(p, v)[i]);
		case POINTLESS_VECTOR_U32:
			return pointless_value_create_as_read_u32(pointless_reader_vector_u32(p, v)[i]);
		case POINTLESS_VECTOR_FLOAT:
			return pointless_value_create_as_read_float(pointless_reader_vector_float(p, v)[i]);
	}

	assert(0);
	return vi;
}

// vectors are complicated
static pointless_create_value_t pointless_cmp_vector_value_create(pointless_create_t* c, pointless_create_value_t* v, uint32_t i)
{
	pointless_create_value_t vi;

	// outside vectors are the hardest
	if (v->header.is_outside_vector) {
		// reset the value
		vi.header.type_29 = POINTLESS_NULL;
		vi.header.is_outside_vector = v->header.is_outside_vector;
		vi.header.is_set_map_vector = v->header.is_set_map_vector;
		vi.header.is_compressed_vector = v->header.is_compressed_vector;

		vi.data.data_u32 = 0;

		switch (v->header.type_29) {
			case POINTLESS_VECTOR_I8:
				vi = pointless_value_create_i32(((int8_t*)cv_get_outside_vector(v)->items)[i]);
				break;
			case POINTLESS_VECTOR_U8:
				vi = pointless_value_create_u32(((uint8_t*)cv_get_outside_vector(v)->items)[i]);
				break;
			case POINTLESS_VECTOR_I16:
				vi = pointless_value_create_i32(((int16_t*)cv_get_outside_vector(v)->items)[i]);
				break;
			case POINTLESS_VECTOR_U16:
				vi = pointless_value_create_u32(((uint16_t*)cv_get_outside_vector(v)->items)[i]);
				break;
			case POINTLESS_VECTOR_I32:
				vi = pointless_value_create_i32(((int32_t*)cv_get_outside_vector(v)->items)[i]);
				break;
			case POINTLESS_VECTOR_U32:
				vi = pointless_value_create_u32(((uint32_t*)cv_get_outside_vector(v)->items)[i]);
				break;
			case POINTLESS_VECTOR_FLOAT:
				vi = pointless_value_create_float(((float*)cv_get_outside_vector(v)->items)[i]);
				break;
			default:
				assert(0);
				break;
		}
	// everything else is simple
	} else {
		uint32_t j = pointless_dynarray_ITEM_AT(uint32_t, &cv_get_priv_vector(v)->vector, i);
		vi = *cv_value_at(j);
	}

	return vi;
}

static int32_t pointless_cmp_reader_vector(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
{
	uint32_t n_items_a = pointless_reader_vector_n_items(p_a, a);
	uint32_t n_items_b = pointless_reader_vector_n_items(p_b, b);

	uint32_t i, n_items = (n_items_a < n_items_b ? n_items_a : n_items_b);
	int32_t cc;

	for (i = 0; i < n_items; i++) {
		pointless_value_t c_a = pointless_cmp_vector_value_reader(p_a, a, i);
		pointless_value_t c_b = pointless_cmp_vector_value_reader(p_b, b, i);

		cc = pointless_cmp_reader_rec(p_a, &c_a, p_b, &c_b, depth + 1, error);

		if (cc != 0 || *error != 0)
			return cc;
	}

	return SIMPLE_CMP(n_items_a, n_items_b);
}

static int32_t pointless_cmp_create_vector(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
{
	uint32_t n_items_a = a->header.is_outside_vector ? cv_get_outside_vector(a)->n_items : pointless_dynarray_n_items(&cv_get_priv_vector(a)->vector);
	uint32_t n_items_b = b->header.is_outside_vector ? cv_get_outside_vector(b)->n_items : pointless_dynarray_n_items(&cv_get_priv_vector(b)->vector);
	uint32_t i, n_items = (n_items_a < n_items_b ? n_items_a : n_items_b);
	int32_t cc;

	for (i = 0; i < n_items; i++) {
		pointless_create_value_t c_a = pointless_cmp_vector_value_create(c, a, i);
		pointless_create_value_t c_b = pointless_cmp_vector_value_create(c, b, i);

		cc = pointless_cmp_create_rec_priv(c, &c_a, &c_b, depth + 1, error);

		if (cc != 0 || *error != 0)
			return cc;
	}

	return SIMPLE_CMP(n_items_a, n_items_b);
}

// map/set not implemented yet
static int32_t pointless_cmp_reader_set(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
	{ *error = "set comparison not implemented yet"; return 0; }
static int32_t pointless_cmp_reader_map(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
	{ *error = "map comparison not implemented yet"; return 0; }
static int32_t pointless_cmp_create_set(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
	{ *error = "set comparison not implemented yet"; return 0; }
static int32_t pointless_cmp_create_map(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
	{ *error = "map comparison not implemented yet"; return 0; }

static pointless_cmp_reader_cb pointless_cmp_reader_func(uint32_t t)
{
	switch (t) {
		case POINTLESS_UNICODE:
			return pointless_cmp_reader_unicode;
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
		case POINTLESS_FLOAT:
			return pointless_cmp_reader_int_float;
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			return pointless_cmp_reader_bitvector;
		case POINTLESS_NULL:
			return pointless_cmp_reader_null;
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_FLOAT:
		case POINTLESS_VECTOR_EMPTY:
			return pointless_cmp_reader_vector;
		case POINTLESS_SET_VALUE:
			return pointless_cmp_reader_set;
		case POINTLESS_MAP_VALUE_VALUE:
			return pointless_cmp_reader_map;
		case POINTLESS_EMPTY_SLOT:
			return pointless_cmp_reader_empty_slot;
	}

	assert(0);
	return 0;
}

static pointless_cmp_create_cb pointless_cmp_create_func(uint32_t t)
{
	switch (t) {
		case POINTLESS_UNICODE:
			return pointless_cmp_create_unicode;
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
		case POINTLESS_FLOAT:
			return pointless_cmp_create_int_float;
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			return pointless_cmp_create_bitvector;
		case POINTLESS_NULL:
			return pointless_cmp_create_null;
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_FLOAT:
		case POINTLESS_VECTOR_EMPTY:
			return pointless_cmp_create_vector;
		case POINTLESS_SET_VALUE:
			return pointless_cmp_create_set;
		case POINTLESS_MAP_VALUE_VALUE:
			return pointless_cmp_create_map;
		case POINTLESS_EMPTY_SLOT:
			return pointless_cmp_create_empty_slot;
	}

	assert(0);
	return 0;
}

static int32_t pointless_cmp_reader_rec(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, uint32_t depth, const char** error)
{
	// check for maximum depth, but only if values could be cyclic
	if (error && depth >= POINTLESS_MAX_DEPTH) {
		*error = "maximum recursion depth reached during comparison";
		return 0;
	}

	// find the comparison func for these two
	pointless_cmp_reader_cb cmp_a = pointless_cmp_reader_func(a->type);
	pointless_cmp_reader_cb cmp_b = pointless_cmp_reader_func(b->type);

	// different type groups, just cmp on type ID
	if (cmp_a != cmp_b)
		return SIMPLE_CMP(a->type, b->type);

	// otherwise, run the cmp for next depth
	return (*cmp_a)(p_a, a, p_b, b, depth + 1, error);
}

static int32_t pointless_cmp_create_rec_priv(pointless_create_t* c, pointless_create_value_t* v_a, pointless_create_value_t* v_b, uint32_t depth, const char** error)
{
	// check for maximum depth
	if (depth >= POINTLESS_MAX_DEPTH) {
		*error = "maximum recursion depth reached during comparison";
		return 0;
	}

	// find the comparison func for these two
	pointless_cmp_create_cb cmp_a = pointless_cmp_create_func(v_a->header.type_29);
	pointless_cmp_create_cb cmp_b = pointless_cmp_create_func(v_a->header.type_29);

	// different type groups, just cmp on type ID
	if (cmp_a != cmp_b)
		return SIMPLE_CMP(v_a->header.type_29, v_b->header.type_29);

	// otherwise, run the cmp for next depth
	return (*cmp_a)(c, v_a, v_b, depth + 1, error);
}

static int32_t pointless_cmp_create_rec(pointless_create_t* c, pointless_create_value_t* a, pointless_create_value_t* b, uint32_t depth, const char** error)
{
	// get the values
	return pointless_cmp_create_rec_priv(c, a, b, depth, error);
}

int32_t pointless_cmp_reader(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b, const char** error)
{
	return pointless_cmp_reader_rec(p_a, a, p_b, b, 0, error);
}

int32_t pointless_cmp_reader_acyclic(pointless_t* p_a, pointless_value_t* a, pointless_t* p_b, pointless_value_t* b)
{
	return pointless_cmp_reader_rec(p_a, a, p_b, b, 0, 0);
}

int32_t pointless_cmp_create(pointless_create_t* c, uint32_t a, uint32_t b, const char** error)
{
	pointless_create_value_t* v_a = &pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, a);
	pointless_create_value_t* v_b = &pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, b);
	return pointless_cmp_create_rec(c, v_a, v_b, 0, error);
}
