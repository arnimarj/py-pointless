#include <pointless/pointless_reader.h>
#include <pointless/pointless_bitvector.h>
#include <pointless/pointless_value.h>

static int32_t pointless_cmp_create_rec_priv(pointless_create_t* c, pointless_complete_create_value_t* v_a, pointless_complete_create_value_t* v_b, uint32_t depth, const char** error);

#define POINTLESS_CMP_STRING(a, b)         while ((uint32_t)(*(a)) == (uint32_t)(*(b))) { if (*(a) == 0) return 0; (a)++; (b)++;} return SIMPLE_CMP(*(a), *(b));
#define POINTLESS_CMP_STRING__N(a, b, n_b)     \
	size_t n_ = 0;                             \
	while (*(a) && n_ < n_b && *(a) == *(b)) { \
		(a)++; (b)++; n_++;                    \
	}                                          \
	if (*(a) == 0 && n_ == (n_b)) return 0;    \
	if (*(a) == 0)             return -1;      \
	if (n_ == (n_b))           return 1;       \
	return SIMPLE_CMP(*(a), *(b));

#ifdef POINTLESS_WCHAR_T_IS_4_BYTES
int32_t pointless_cmp_wchar_wchar(wchar_t* a, wchar_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
#endif

int32_t pointless_cmp_string_8_8(uint8_t* a, uint8_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_string_8_16(uint8_t* a, uint16_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_string_8_32(uint8_t* a, uint32_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_string_16_8(uint16_t* a, uint8_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_string_16_16(uint16_t* a, uint16_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_string_16_32(uint16_t* a, uint32_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_string_32_8(uint32_t* a, uint8_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_string_32_16(uint32_t* a, uint16_t* b)
	{ POINTLESS_CMP_STRING(a, b); }
int32_t pointless_cmp_string_32_32(uint32_t* a, uint32_t* b)
	{ POINTLESS_CMP_STRING(a, b); }

int32_t pointless_cmp_string_8_8_n(uint8_t* a, uint8_t* b, size_t n_b)
	{ POINTLESS_CMP_STRING__N(a, b, n_b); }
int32_t pointless_cmp_string_32_8_n(uint32_t* a, uint8_t* b, size_t n_b)
	{ POINTLESS_CMP_STRING__N(a, b, n_b); }

typedef int32_t (*pointless_cmp_reader_cb)(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error);
typedef int32_t (*pointless_cmp_create_cb)(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error);

static int32_t pointless_cmp_reader_rec(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error);
static int32_t pointless_cmp_create_rec(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error);

// null and empty-set comparison are always the same
static int32_t pointless_cmp_reader_null(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
	{ return 0; }
static int32_t pointless_cmp_reader_empty_slot(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
	{ return 0; }
static int32_t pointless_cmp_create_empty_slot(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
	{ return 0; }
static int32_t pointless_cmp_create_null(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
	{ return 0; }

// unicodes are fairly simple
static int32_t pointless_cmp_reader_string_unicode(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
{
	pointless_value_t _a = pointless_value_from_complete(a);
	pointless_value_t _b = pointless_value_from_complete(b);

	// uu
	if (a->type == POINTLESS_UNICODE_ && b->type == POINTLESS_UNICODE_) {
		uint32_t* unicode_a = pointless_reader_unicode_value_ucs4(p_a, &_a);
		uint32_t* unicode_b = pointless_reader_unicode_value_ucs4(p_b, &_b);
		return pointless_cmp_string_32_32(unicode_a, unicode_b);
	// us
	} else if (a->type == POINTLESS_UNICODE_ && b->type == POINTLESS_STRING_) {
		uint32_t* unicode_a = pointless_reader_unicode_value_ucs4(p_a, &_a);
		uint8_t* string_b = pointless_reader_string_value_ascii(p_b, &_b);
		return pointless_cmp_string_32_8(unicode_a, string_b);
	// su
	} else if (a->type == POINTLESS_STRING_ && b->type == POINTLESS_UNICODE_) {
		uint8_t* string_a = pointless_reader_string_value_ascii(p_a, &_a);
		uint32_t* unicode_b = pointless_reader_unicode_value_ucs4(p_b, &_b);
		return pointless_cmp_string_8_32(string_a, unicode_b);
	// ss
	} else if (a->type == POINTLESS_STRING_ && b->type == POINTLESS_STRING_) {
		uint8_t* string_a = pointless_reader_string_value_ascii(p_a, &_a);
		uint8_t* string_b = pointless_reader_string_value_ascii(p_b, &_b);
		return pointless_cmp_string_8_8(string_a, string_b);
	}

	assert(0);
	return 0;
}

static int32_t pointless_cmp_create_string_unicode(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
{
	pointless_create_value_t _a = pointless_create_value_from_complete(a);
	pointless_create_value_t _b = pointless_create_value_from_complete(b);

	// uu
	if (_a.header.type_29 == POINTLESS_UNICODE_ && _b.header.type_29 == POINTLESS_UNICODE_) {
		uint32_t* unicode_a = (uint32_t*)cv_get_unicode(&_a) + 1;
		uint32_t* unicode_b = (uint32_t*)cv_get_unicode(&_b) + 1;
		return pointless_cmp_string_32_32(unicode_a, unicode_b);
	// us
	} else if (_a.header.type_29 == POINTLESS_UNICODE_ && _b.header.type_29 == POINTLESS_STRING_) {
		uint32_t* unicode_a = (uint32_t*)cv_get_unicode(&_a) + 1;
		uint8_t* string_b = (uint8_t*)((uint32_t*)cv_get_string(&_b) + 1);
		return pointless_cmp_string_32_8(unicode_a, string_b);
	// su
	} else if (_a.header.type_29 == POINTLESS_STRING_ && _b.header.type_29 == POINTLESS_UNICODE_) {
		uint8_t* string_a = (uint8_t*)((uint32_t*)cv_get_string(&_a) + 1);
		uint32_t* unicode_b = (uint32_t*)cv_get_unicode(&_b) + 1;
		return pointless_cmp_string_8_32(string_a, unicode_b);
	// ss
	} else if (_a.header.type_29 == POINTLESS_STRING_ && _b.header.type_29 == POINTLESS_STRING_) {
		uint8_t* string_a = (uint8_t*)((uint32_t*)cv_get_string(&_a) + 1);
		uint8_t* string_b = (uint8_t*)((uint32_t*)cv_get_string(&_b) + 1);
		return pointless_cmp_string_8_8(string_a, string_b);
	}

	assert(0);
	return 0;
}

// int/float is absolutely trivial
static int64_t pointless_int_value_signed(uint32_t t, pointless_complete_value_data_t* v)
{
	switch (t) {
		case POINTLESS_I32:
		case POINTLESS_I64:
		case POINTLESS_BOOLEAN:
			return (int64_t)pointless_complete_value_get_as_i64(t, v);
	}

	assert(0);
	return 0;
}

static int pointless_is_signed(uint32_t t)
{
	switch (t) {
		case POINTLESS_I32:
		case POINTLESS_I64:
		case POINTLESS_BOOLEAN:
			return 1;
	}

	return 0;
}

static uint64_t pointless_int_value_unsigned(uint32_t t, pointless_complete_value_data_t* v)
{
	switch (t) {
		case POINTLESS_U32:
		case POINTLESS_U64:
		case POINTLESS_BOOLEAN:
			return (uint64_t)pointless_complete_value_get_as_u64(t, v);
	}

	assert(0);
	return 0;
}

static int32_t pointless_cmp_int_float(uint32_t t_a, pointless_complete_value_data_t* v_a, uint32_t t_b, pointless_complete_value_data_t* v_b)
{
	// float, float
	if (t_a == POINTLESS_FLOAT && t_b == POINTLESS_FLOAT) {
		return SIMPLE_CMP(v_a->data_f, v_b->data_f);
	// float, int
	} else if (t_a == POINTLESS_FLOAT) {
		if (pointless_is_signed(t_b))
			return SIMPLE_CMP(v_a->data_f, (float)pointless_int_value_signed(t_b, v_b));
		else
			return SIMPLE_CMP(v_a->data_f, (float)pointless_int_value_unsigned(t_b, v_b));
	// int, float
	} else if (t_b == POINTLESS_FLOAT) {
		if (pointless_is_signed(t_a))
			return SIMPLE_CMP((float)pointless_int_value_signed(t_a, v_a), v_b->data_f);
		else
			return SIMPLE_CMP((float)pointless_int_value_unsigned(t_a, v_a), v_b->data_f);
	// int, int
	} else {
		// ss
		if (pointless_is_signed(t_a) && pointless_is_signed(t_b))
			return SIMPLE_CMP(pointless_int_value_signed(t_a, v_a), pointless_int_value_signed(t_b, v_b));

		// uu
		if (!pointless_is_signed(t_a) && !pointless_is_signed(t_b))
			return SIMPLE_CMP(pointless_int_value_unsigned(t_a, v_a), pointless_int_value_unsigned(t_b, v_b));

		// su
		if (pointless_is_signed(t_a) && !pointless_is_signed(t_b)) {
			int64_t _a = pointless_int_value_signed(t_a, v_a);
			uint64_t _b = pointless_int_value_unsigned(t_b, v_b);

			if (_a < 0)
				return -1;

			return SIMPLE_CMP((uint64_t)_a, _b);
		}

		// us
		{
			uint64_t _a = pointless_int_value_unsigned(t_a, v_a);
			int64_t _b = pointless_int_value_signed(t_b, v_b);

			if (_b < 0)
				return -1;

			return SIMPLE_CMP(_a, (uint64_t)_b);
		}
	}
}

static int32_t pointless_cmp_reader_int_float(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
{
	return pointless_cmp_int_float(a->type, &a->complete_data, b->type, &b->complete_data);
}

static int32_t pointless_cmp_create_int_float(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
{
	return pointless_cmp_int_float(a->header.type_29, &a->complete_data, b->header.type_29, &b->complete_data);
}

// bitvectors are fairly simple
static int32_t pointless_cmp_reader_bitvector(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
{
	pointless_value_t _a = pointless_value_from_complete(a);
	pointless_value_t _b = pointless_value_from_complete(b);

	void* buffer_a = 0;
	void* buffer_b = 0;

	if (a->type == POINTLESS_BITVECTOR)
		buffer_a = pointless_reader_bitvector_buffer(p_a, &_a);

	if (b->type == POINTLESS_BITVECTOR)
		buffer_b = pointless_reader_bitvector_buffer(p_b, &_b);

	return pointless_bitvector_cmp_buffer_buffer(a->type, &_a.data, buffer_a, b->type, &_b.data, buffer_b);
}

static int32_t pointless_cmp_create_bitvector(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
{
	pointless_create_value_t _a = pointless_create_value_from_complete(a);
	pointless_create_value_t _b = pointless_create_value_from_complete(b);

	void* buffer_a = 0;
	void* buffer_b = 0;

	if (a->header.type_29 == POINTLESS_BITVECTOR)
		buffer_a = cv_get_bitvector(&_a);

	if (b->header.type_29 == POINTLESS_BITVECTOR)
		buffer_b = cv_get_bitvector(&_b);

	return pointless_bitvector_cmp_buffer_buffer(a->header.type_29, &_a.data, buffer_a, b->header.type_29, &_b.data, buffer_b);
}

// vectors are complicated
static pointless_complete_value_t pointless_cmp_vector_value_reader(pointless_t* p, pointless_complete_value_t* v, uint32_t i)
{
	pointless_complete_value_t vi = pointless_complete_value_create_as_read_null();
	pointless_value_t _v = pointless_value_from_complete(v);

	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			return pointless_value_to_complete(&(pointless_reader_vector_value(p, &_v)[i]));
		case POINTLESS_VECTOR_I8:
			return pointless_complete_value_create_as_read_i32((int32_t)pointless_reader_vector_i8(p, &_v)[i]);
		case POINTLESS_VECTOR_U8:
			return pointless_complete_value_create_as_read_u32((uint32_t)pointless_reader_vector_u8(p, &_v)[i]);
		case POINTLESS_VECTOR_I16:
			return pointless_complete_value_create_as_read_i32((int32_t)pointless_reader_vector_i16(p, &_v)[i]);
		case POINTLESS_VECTOR_U16:
			return pointless_complete_value_create_as_read_u32((uint32_t)pointless_reader_vector_u16(p, &_v)[i]);
		case POINTLESS_VECTOR_I32:
			return pointless_complete_value_create_as_read_i32((int32_t)pointless_reader_vector_i32(p, &_v)[i]);
		case POINTLESS_VECTOR_U32:
			return pointless_complete_value_create_as_read_u32(pointless_reader_vector_u32(p, &_v)[i]);
		case POINTLESS_VECTOR_I64:
			return pointless_complete_value_create_as_read_i64((int64_t)pointless_reader_vector_i64(p, &_v)[i]);
		case POINTLESS_VECTOR_U64:
			return pointless_complete_value_create_as_read_u64(pointless_reader_vector_u64(p, &_v)[i]);
		case POINTLESS_VECTOR_FLOAT:
			return pointless_complete_value_create_as_read_float(pointless_reader_vector_float(p, &_v)[i]);
	}

	assert(0);
	return vi;
}

// vectors are complicated
static pointless_complete_create_value_t pointless_cmp_vector_value_create(pointless_create_t* c, pointless_complete_create_value_t* v, uint32_t i)
{
	pointless_complete_create_value_t vi;
	pointless_create_value_t _v = pointless_create_value_from_complete(v);

	// outside vectors are the hardest
	if (v->header.is_outside_vector) {
		// reset the value
		vi.header.type_29 = POINTLESS_NULL;
		vi.header.is_outside_vector = v->header.is_outside_vector;
		vi.header.is_set_map_vector = v->header.is_set_map_vector;
		vi.header.is_compressed_vector = v->header.is_compressed_vector;

		vi.complete_data.data_u32 = 0;

		switch (v->header.type_29) {
			case POINTLESS_VECTOR_I8:
				vi = pointless_complete_value_create_i32(((int8_t*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			case POINTLESS_VECTOR_U8:
				vi = pointless_complete_value_create_u32(((uint8_t*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			case POINTLESS_VECTOR_I16:
				vi = pointless_complete_value_create_i32(((int16_t*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			case POINTLESS_VECTOR_U16:
				vi = pointless_complete_value_create_u32(((uint16_t*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			case POINTLESS_VECTOR_I32:
				vi = pointless_complete_value_create_i32(((int32_t*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			case POINTLESS_VECTOR_U32:
				vi = pointless_complete_value_create_u32(((uint32_t*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			case POINTLESS_VECTOR_I64:
				vi = pointless_complete_value_create_i64(((int64_t*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			case POINTLESS_VECTOR_U64:
				vi = pointless_complete_value_create_u64(((uint64_t*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			case POINTLESS_VECTOR_FLOAT:
				vi = pointless_complete_value_create_float(((float*)cv_get_outside_vector(&_v)->items)[i]);
				break;
			default:
				assert(0);
				break;
		}
	// everything else is simple
	} else {
		uint32_t j = pointless_dynarray_ITEM_AT(uint32_t, &cv_get_priv_vector(&_v)->vector, i);
		vi = pointless_create_value_to_complete(cv_value_at(j));
	}

	return vi;
}

static int32_t pointless_cmp_reader_vector(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
{
	pointless_value_t _a = pointless_value_from_complete(a);
	pointless_value_t _b = pointless_value_from_complete(b);

	uint32_t n_items_a = pointless_reader_vector_n_items(p_a, &_a);
	uint32_t n_items_b = pointless_reader_vector_n_items(p_b, &_b);

	uint32_t i, n_items = (n_items_a < n_items_b ? n_items_a : n_items_b);
	int32_t cc;

	for (i = 0; i < n_items; i++) {
		pointless_complete_value_t c_a = pointless_cmp_vector_value_reader(p_a, a, i);
		pointless_complete_value_t c_b = pointless_cmp_vector_value_reader(p_b, b, i);

		cc = pointless_cmp_reader_rec(p_a, &c_a, p_b, &c_b, depth + 1, error);

		if (cc != 0 || *error != 0)
			return cc;
	}

	return SIMPLE_CMP(n_items_a, n_items_b);
}

static int32_t pointless_cmp_create_vector(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
{
	pointless_create_value_t _a = pointless_create_value_from_complete(a);
	pointless_create_value_t _b = pointless_create_value_from_complete(b);

	uint32_t n_items_a = a->header.is_outside_vector ? cv_get_outside_vector(&_a)->n_items : pointless_dynarray_n_items(&cv_get_priv_vector(&_a)->vector);
	uint32_t n_items_b = b->header.is_outside_vector ? cv_get_outside_vector(&_b)->n_items : pointless_dynarray_n_items(&cv_get_priv_vector(&_b)->vector);
	uint32_t i, n_items = (n_items_a < n_items_b ? n_items_a : n_items_b);
	int32_t cc;

	for (i = 0; i < n_items; i++) {
		pointless_complete_create_value_t c_a = pointless_cmp_vector_value_create(c, a, i);
		pointless_complete_create_value_t c_b = pointless_cmp_vector_value_create(c, b, i);

		cc = pointless_cmp_create_rec_priv(c, &c_a, &c_b, depth + 1, error);

		if (cc != 0 || *error != 0)
			return cc;
	}

	return SIMPLE_CMP(n_items_a, n_items_b);
}

// map/set not implemented yet
static int32_t pointless_cmp_reader_set(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
	{ *error = "set comparison not implemented yet"; return 0; }
static int32_t pointless_cmp_reader_map(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
	{ *error = "map comparison not implemented yet"; return 0; }
static int32_t pointless_cmp_create_set(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
	{ *error = "set comparison not implemented yet"; return 0; }
static int32_t pointless_cmp_create_map(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
	{ *error = "map comparison not implemented yet"; return 0; }

static pointless_cmp_reader_cb pointless_cmp_reader_func(uint32_t t)
{
	switch (t) {
		case POINTLESS_UNICODE_:
		case POINTLESS_STRING_:
			return pointless_cmp_reader_string_unicode;
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_I64:
		case POINTLESS_U64:
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
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U64:
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
		case POINTLESS_UNICODE_:
		case POINTLESS_STRING_:
			return pointless_cmp_create_string_unicode;
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
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U64:
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

static int32_t pointless_cmp_reader_rec(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, uint32_t depth, const char** error)
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

static int32_t pointless_cmp_create_rec_priv(pointless_create_t* c, pointless_complete_create_value_t* v_a, pointless_complete_create_value_t* v_b, uint32_t depth, const char** error)
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

static int32_t pointless_cmp_create_rec(pointless_create_t* c, pointless_complete_create_value_t* a, pointless_complete_create_value_t* b, uint32_t depth, const char** error)
{
	// get the values
	return pointless_cmp_create_rec_priv(c, a, b, depth, error);
}

int32_t pointless_cmp_reader(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, const char** error)
{
	return pointless_cmp_reader_rec(p_a, a, p_b, b, 0, error);
}

int32_t pointless_cmp_reader_acyclic(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b)
{
	return pointless_cmp_reader_rec(p_a, a, p_b, b, 0, 0);
}

int32_t pointless_cmp_create(pointless_create_t* c, uint32_t a, uint32_t b, const char** error)
{
	pointless_create_value_t* v_a = &pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, a);
	pointless_create_value_t* v_b = &pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, b);

	pointless_complete_create_value_t _v_a = pointless_create_value_to_complete(v_a);
	pointless_complete_create_value_t _v_b = pointless_create_value_to_complete(v_b);

	return pointless_cmp_create_rec(c, &_v_a, &_v_b, 0, error);
}
