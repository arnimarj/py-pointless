#include <pointless/pointless_validate.h>

static int pointless_require_heap(pointless_validate_context_t* context, uint64_t offset, uint64_t n_bytes)
{
	intop_u64_t _past_last_byte = intop_u64_add(intop_u64_init(offset), intop_u64_init(n_bytes));
	return (!_past_last_byte.is_overflow && _past_last_byte.value <= context->p->heap_len);
}

static int32_t pointless_validate_vector_heap(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
{
	assert(v->data.data_u32 < context->p->header->n_vector);

	uint64_t offset = PC_OFFSET(context->p, vector_offsets, v->data.data_u32);

	if (!pointless_require_heap(context, offset, sizeof(uint32_t))) {
		*error = "vector header too large for heap";
		return 0;
	}

	uint32_t* n_items = (uint32_t*)((char*)context->p->heap_ptr + offset);
	uint32_t item_len = 0;

	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			item_len = sizeof(pointless_value_t);
			break;
		case POINTLESS_VECTOR_I8:
			item_len = sizeof(int8_t);
			break;
		case POINTLESS_VECTOR_U8:
			item_len = sizeof(uint8_t);
			break;
		case POINTLESS_VECTOR_I16:
			item_len = sizeof(int16_t);
			break;
		case POINTLESS_VECTOR_U16:
			item_len = sizeof(uint16_t);
			break;
		case POINTLESS_VECTOR_I32:
			item_len = sizeof(int32_t);
			break;
		case POINTLESS_VECTOR_U32:
			item_len = sizeof(uint32_t);
			break;
		case POINTLESS_VECTOR_I64:
			item_len = sizeof(int64_t);
			break;
		case POINTLESS_VECTOR_U64:
			item_len = sizeof(uint64_t);
			break;
		case POINTLESS_VECTOR_FLOAT:
			item_len = sizeof(float);
			break;
		default:
			assert(0);
			break;
	}

	// sizeof(uint32_t) + (item_len * (*n_items))
	intop_u64_t n_bytes = intop_u64_add(intop_u64_init(sizeof(uint32_t)), intop_u64_mult(intop_u64_init(item_len), intop_u64_init(*n_items)));

	if (n_bytes.is_overflow || !pointless_require_heap(context, offset, n_bytes.value)) {
		*error = "vector body too large for heap";
		return 0;
	}

	return 1;
}

static int32_t pointless_validate_bitvector_heap(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
{
	assert(v->data.data_u32 < context->p->header->n_bitvector);
	uint64_t offset = PC_OFFSET(context->p, bitvector_offsets, v->data.data_u32);

	// uint32_t | bits
	if (!pointless_require_heap(context, offset, sizeof(uint32_t))) {
		*error = "bitvector too large for heap";
		return 0;
	}

	uint32_t* n_bits = (uint32_t*)((char*)context->p->heap_ptr + offset);

	if (!pointless_require_heap(context, offset, sizeof(uint32_t) + ICEIL(*n_bits, 8))) {
		*error = "bitvector too large for heap";
		return 0;
	}

	return 1;
}

static int32_t pointless_validate_unicode_heap(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
{
	assert(v->data.data_u32 < context->p->header->n_string_unicode);
	uint64_t offset = PC_OFFSET(context->p, string_unicode_offsets, v->data.data_u32);

	// uint32_t | pointless_char_t * (len + 1)
	if (!pointless_require_heap(context, offset, sizeof(uint32_t))) {
		*error = "unicode too large for heap";
		return 0;
	}

	uint32_t* s_len = (uint32_t*)((char*)context->p->heap_ptr + offset);

	intop_u64_t n_bytes = intop_u64_add(intop_u64_init(sizeof(uint32_t)), intop_u64_mult(intop_u64_init((uint64_t)*s_len + 1), intop_u64_init(sizeof(pointless_unicode_char_t))));

	if (n_bytes.is_overflow || !pointless_require_heap(context, offset, n_bytes.value)) {
		*error = "unicode too large for heap";
		return 0;
	}

	pointless_unicode_char_t* s = (pointless_unicode_char_t*)(s_len + 1);

	uint64_t i;

	for (i = 0; i < *s_len; i++) {
		if (s[i] == 0) {
			*error = "premature end-of-unicode";
			return 0;
		}
	}

	if (s[i] != 0) {
		*error = "missing end-of-unicode";
		return 0;
	}

	if (context->force_ucs2 && !pointless_is_ucs4_ucs2(s)) {
		*error = "there are strings which don't fit in 16-bits";
		return 0;
	}

	return 1;
}

static int32_t pointless_validate_string_heap(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
{
	assert(v->data.data_u32 < context->p->header->n_string_unicode);
	uint64_t offset = PC_OFFSET(context->p, string_unicode_offsets, v->data.data_u32);

	// uint32_t | pointless_char_t * (len + 1)
	if (!pointless_require_heap(context, offset, sizeof(uint32_t))) {
		*error = "string too large for heap";
		return 0;
	}

	uint32_t* s_len = (uint32_t*)((char*)context->p->heap_ptr + offset);

	intop_u64_t n_bytes = intop_u64_add(intop_u64_init(sizeof(uint32_t)), intop_u64_mult(intop_u64_init((uint64_t)*s_len + 1), intop_u64_init(sizeof(uint8_t))));

	if (n_bytes.is_overflow || !pointless_require_heap(context, offset, n_bytes.value)) {
		*error = "unicode too large for heap";
		return 0;
	}

	uint8_t* s = (uint8_t*)(s_len + 1);

	uint64_t i;

	for (i = 0; i < *s_len; i++) {
		if (s[i] == 0) {
			*error = "premature end-of-string";
			return 0;
		}
	}

	if (s[i] != 0) {
		*error = "missing end-of-string";	
		return 0;
	}

	return 1;
}

static int32_t pointless_validate_set_heap(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
{
	// simple stuff, not allowed to check children
	assert(v->data.data_u32 < context->p->header->n_set);
	uint64_t offset = PC_OFFSET(context->p, set_offsets, v->data.data_u32);

	// get header
	if (!pointless_require_heap(context, offset, sizeof(pointless_set_header_t))) {
		*error = "set header too large for heap";
		return 0;
	}

	pointless_set_header_t* header = (pointless_set_header_t*)((char*)context->p->heap_ptr + offset);

	// hash/key vectors must be of a certain type
	if (header->hash_vector.type != POINTLESS_VECTOR_U32) {
		*error = "set hash vector not of type POINTLESS_VECTOR_U32";
		return 0;
	}

	if (header->key_vector.type != POINTLESS_VECTOR_VALUE_HASHABLE) {
		*error = "set key vector not of type POINTLESS_VECTOR_VALUE_HASHABLE";
		printf("but rather %i\n", (int)header->key_vector.type);
		return 0;
	}

	return 1;
}

static int32_t pointless_validate_map_heap(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
{
	// simple stuff first
	assert(v->data.data_u32 < context->p->header->n_map);
	uint64_t offset = PC_OFFSET(context->p, map_offsets, v->data.data_u32);

	// get header
	if (!pointless_require_heap(context, offset, sizeof(pointless_map_header_t))) {
		*error = "map header too large for heap";
		return 0;
	}

	pointless_map_header_t* header = (pointless_map_header_t*)((char*)context->p->heap_ptr + offset);

	// hash/key vectors must be of a certain type
	if (header->hash_vector.type != POINTLESS_VECTOR_U32) {
		*error = "map hash vector not of type POINTLESS_VECTOR_U32";
		return 0;
	}

	if (header->key_vector.type != POINTLESS_VECTOR_VALUE_HASHABLE) {
		*error = "map key vector not of type POINTLESS_VECTOR_VALUE_HASHABLE";
		printf("but rather %i\n", (int)header->key_vector.type);
		return 0;
	}

	if (header->value_vector.type != POINTLESS_VECTOR_VALUE_HASHABLE && header->value_vector.type != POINTLESS_VECTOR_VALUE) {
		*error = "map key vector not of type POINTLESS_VECTOR_VALUE or POINTLESS_VECTOR_VALUE_HASHABLE";
		return 0;
	}

	return 1;
}

int32_t pointless_validate_heap_value(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
{
	switch (v->type) {
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
			return pointless_validate_vector_heap(context, v, error);
		case POINTLESS_VECTOR_EMPTY:
			break;
		case POINTLESS_UNICODE_:
			return pointless_validate_unicode_heap(context, v, error);
		case POINTLESS_STRING_:
			return pointless_validate_string_heap(context, v, error);
		case POINTLESS_BITVECTOR:
			return pointless_validate_bitvector_heap(context, v, error);
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			break;
		case POINTLESS_SET_VALUE:
			return pointless_validate_set_heap(context, v, error);
		case POINTLESS_MAP_VALUE_VALUE:
			return pointless_validate_map_heap(context, v, error);
		case POINTLESS_EMPTY_SLOT:
			break;
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_FLOAT:
		case POINTLESS_BOOLEAN:
		case POINTLESS_NULL:
			break;
		default:
			*error = "unknown type";
			return 0;
	}

	return 1;
}
