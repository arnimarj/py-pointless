#include <pointless/pointless_value.h>

// create-time values
pointless_create_value_t pointless_value_create_i32(int32_t v)
{
	pointless_create_value_t vv;
	vv.header.type_29 = POINTLESS_I32;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.data.data_i32 = v;
	return vv;
}

pointless_create_value_t pointless_value_create_u32(uint32_t v)
{
	pointless_create_value_t vv;
	vv.header.type_29 = POINTLESS_U32;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.data.data_u32 = v;
	return vv;
}

pointless_create_value_t pointless_value_create_float(float v)
{
	pointless_create_value_t vv;
	vv.header.type_29 = POINTLESS_FLOAT;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.data.data_f = v;
	return vv;
}

pointless_create_value_t pointless_value_create_bool_false()
{
	pointless_create_value_t vv;
	vv.header.type_29 = POINTLESS_BOOLEAN;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.data.data_u32 = 0;
	return vv;
}

pointless_create_value_t pointless_value_create_bool_true()
{
	pointless_create_value_t vv;
	vv.header.type_29 = POINTLESS_BOOLEAN;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.data.data_u32 = 1;
	return vv;
}

pointless_create_value_t pointless_value_create_null()
{
	pointless_create_value_t vv;
	vv.header.type_29 = POINTLESS_NULL;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.data.data_u32 = 0;
	return vv;
}

pointless_create_value_t pointless_value_create_empty_slot()
{
	pointless_create_value_t vv;
	vv.header.type_29 = POINTLESS_EMPTY_SLOT;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.data.data_u32 = 0;
	return vv;
}

// read-time values
pointless_value_t pointless_value_create_as_read_i32(int32_t v)
{
	pointless_value_t vv;
	vv.type = POINTLESS_I32;
	vv.data.data_i32 = v;
	return vv;
}

pointless_value_t pointless_value_create_as_read_u32(uint32_t v)
{
	pointless_value_t vv;
	vv.type = POINTLESS_U32;
	vv.data.data_u32 = v;
	return vv;
}

pointless_value_t pointless_value_create_as_read_float(float v)
{
	pointless_value_t vv;
	vv.type = POINTLESS_FLOAT;
	vv.data.data_f = v;
	return vv;
}





pointless_value_t pointless_value_create_as_read_bool_false()
{
	pointless_value_t vv;
	vv.type = POINTLESS_BOOLEAN;
	vv.data.data_u32 = 0;
	return vv;
}

pointless_value_t pointless_value_create_as_read_bool_true()
{
	pointless_value_t vv;
	vv.type = POINTLESS_BOOLEAN;
	vv.data.data_u32 = 1;
	return vv;
}

pointless_value_t pointless_value_create_as_read_null()
{
	pointless_value_t vv;
	vv.type = POINTLESS_NULL;
	vv.data.data_u32 = 0;
	return vv;
}

pointless_value_t pointless_value_create_as_read_empty_slot()
{
	pointless_value_t vv;
	vv.type = POINTLESS_EMPTY_SLOT;
	vv.data.data_u32 = 0;
	return vv;
}

// hash/cmp-time values

// ...create
pointless_complete_create_value_t pointless_complete_value_create_i32(int32_t v)
{
	pointless_create_value_t _v = pointless_value_create_i32(v);
	return pointless_create_value_to_complete(&_v);
}

pointless_complete_create_value_t pointless_complete_value_create_u32(uint32_t v)
{
	pointless_create_value_t _v = pointless_value_create_u32(v);
	return pointless_create_value_to_complete(&_v);
}

pointless_complete_create_value_t pointless_complete_value_create_i64(int64_t v)
{
	pointless_complete_create_value_t vv;
	vv.header.type_29 = POINTLESS_I64;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.complete_data.data_i64= v;
	return vv;
}

pointless_complete_create_value_t pointless_complete_value_create_u64(uint64_t v)
{
	pointless_complete_create_value_t vv;
	vv.header.type_29 = POINTLESS_U64;
	vv.header.is_compressed_vector = 0;
	vv.header.is_outside_vector = 0;
	vv.header.is_set_map_vector = 0;
	vv.complete_data.data_u64 = v;
	return vv;
}

pointless_complete_create_value_t pointless_complete_value_create_float(float v)
{
	pointless_create_value_t _v = pointless_value_create_float(v);
	return pointless_create_value_to_complete(&_v);
}

pointless_complete_create_value_t pointless_complete_value_create_null();

//  ...read
pointless_complete_value_t pointless_complete_value_create_as_read_i32(int32_t v)
{
	pointless_value_t _v = pointless_value_create_as_read_i32(v);
	return pointless_value_to_complete(&_v);
}

pointless_complete_value_t pointless_complete_value_create_as_read_u32(uint32_t v)
{
	pointless_value_t _v = pointless_value_create_as_read_u32(v);
	return pointless_value_to_complete(&_v);
}

pointless_complete_value_t pointless_complete_value_create_as_read_i64(int64_t v)
{
	pointless_complete_value_t vv;
	vv.type = POINTLESS_I64;
	vv.complete_data.data_i64 = v;
	return vv;
}

pointless_complete_value_t pointless_complete_value_create_as_read_u64(uint64_t v)
{
	pointless_complete_value_t vv;
	vv.type = POINTLESS_U64;
	vv.complete_data.data_u64 = v;
	return vv;
}

pointless_complete_value_t pointless_complete_value_create_as_read_float(float v)
{
	pointless_value_t _v = pointless_value_create_as_read_float(v);
	return pointless_value_to_complete(&_v);
}

pointless_complete_value_t pointless_complete_value_create_as_read_null()
{
	pointless_value_t _v = pointless_value_create_as_read_null();
	return pointless_value_to_complete(&_v);
}

// read-time accessors
int32_t pointless_value_get_i32(uint32_t t, pointless_value_data_t* v)
{
	assert(t == POINTLESS_I32);
	return v->data_i32;
}

uint32_t pointless_value_get_u32(uint32_t t, pointless_value_data_t* v)
{
	assert(t == POINTLESS_U32);
	return v->data_u32;
}

float pointless_value_get_float(uint32_t t, pointless_value_data_t* v)
{
	assert(t == POINTLESS_FLOAT);
	return v->data_f;
}

uint32_t pointless_value_get_bool(uint32_t t, pointless_value_data_t* v)
{
	assert(t == POINTLESS_BOOLEAN);
	return v->data_u32;
}

int64_t pointless_get_int_as_int64(uint32_t t, pointless_value_data_t* v)
{
	assert(t == POINTLESS_I32 || t == POINTLESS_U32);

	if (t == POINTLESS_I32)
		return (int64_t)pointless_value_get_i32(t, v);
	else
		return (int64_t)pointless_value_get_u32(t, v);
}

// create-time accessor
int64_t pointless_create_get_int_as_int64(pointless_create_value_t* v)
{
	assert(v->header.type_29 == POINTLESS_I32 || v->header.type_29 == POINTLESS_U32);

	if (v->header.type_29 == POINTLESS_I32)
		return (int64_t)pointless_create_value_get_i32(v);
	else
		return (int64_t)pointless_create_value_get_u32(v);
}

int32_t pointless_create_value_get_i32(pointless_create_value_t* v)
{
	assert(v->header.type_29 == POINTLESS_I32);
	return (v->data.data_i32);
}

uint32_t pointless_create_value_get_u32(pointless_create_value_t* v)
{
	assert(v->header.type_29 == POINTLESS_U32);
	return (v->data.data_u32);
}

float pointless_create_value_get_float(pointless_create_value_t* v)
{
	return (v->data.data_f);
}

// hash-time accessor
int64_t pointless_complete_value_get_as_i64(uint32_t t, pointless_complete_value_data_t* v)
{
	switch (t) {
		case POINTLESS_I32:
			return v->data_i32;
		case POINTLESS_BOOLEAN:
			return v->data_u32;
		case POINTLESS_I64:
			return v->data_i64;
	}

	assert(0);
	return 0;
}

uint64_t pointless_complete_value_get_as_u64(uint32_t t, pointless_complete_value_data_t* v)
{
	switch (t) {
		case POINTLESS_U32:
		case POINTLESS_BOOLEAN:
			return v->data_u32;
		case POINTLESS_U64:
			return v->data_u64;
	}

	assert(0);
	return 0;
}

float pointless_complete_value_get_float(uint32_t t, pointless_complete_value_data_t* v)
{
	assert(t == POINTLESS_FLOAT);
	return (v->data_f);
}


// conversions
pointless_value_t pointless_value_from_complete(pointless_complete_value_t* a)
{
	// normal value has no space for 64-bit integers
	assert(a->type != POINTLESS_I64 && a->type != POINTLESS_U64);

	pointless_value_t v;
	v.type = a->type;
	v.data.data_u32 = a->complete_data.data_u32;
	return v;
}

pointless_complete_value_t pointless_value_to_complete(pointless_value_t* a)
{
	pointless_complete_value_t v;
	v.type = a->type;
	v.complete_data.data_u64 = 0;
	v.complete_data.data_u32 = a->data.data_u32;
	return v;
}

pointless_create_value_t pointless_create_value_from_complete(pointless_complete_create_value_t* a)
{
	// normal value has no space for 64-bit integers
	assert(a->header.type_29 != POINTLESS_I64 && a->header.type_29 != POINTLESS_U64);

	pointless_create_value_t v;
	v.header = a->header;
	v.data.data_u32 = a->complete_data.data_u32;
	return v;
}

pointless_complete_create_value_t pointless_create_value_to_complete(pointless_create_value_t* a)
{
	pointless_complete_create_value_t v;
	v.header = a->header;
	v.complete_data.data_u64 = 0;
	v.complete_data.data_u32 = a->data.data_u32;
	return v;
}

int32_t pointless_is_vector_type(uint32_t type)
{
	switch (type) {
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
			return 1;
	}

	return 0;
}

int32_t pointless_is_integer_type(uint32_t type)
{
	switch (type) {
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_I64:
		case POINTLESS_U64:
			return 1;
	}

	return 0;
}

int32_t pointless_is_bitvector_type(uint32_t type)
{
	switch (type) {
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			return 1;
	}

	return 0;
}
