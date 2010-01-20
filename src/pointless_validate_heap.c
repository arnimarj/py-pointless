#include <pointless/pointless_validate.h>

#define POINTLESS_REQUIRE_HEAP(offset, n_bytes, error_msg) if (offset + n_bytes > p->heap_len) { *error = error_msg; return 0; }

static int32_t pointless_validate_vector_heap(pointless_t* p, pointless_value_t* v, const char** error)
{
	assert(v->data.data_u32 < p->header->n_vector);

	uint32_t offset = p->vector_offsets[v->data.data_u32];

	POINTLESS_REQUIRE_HEAP(offset, sizeof(uint32_t), "vector header too large for heap");

	uint32_t* n_items = (uint32_t*)((char*)p->heap_ptr + offset);
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
		case POINTLESS_VECTOR_FLOAT:
			item_len = sizeof(float);
			break;
		default:
			assert(0);
			break;
	}

	POINTLESS_REQUIRE_HEAP(offset, sizeof(uint32_t) + (uint64_t)item_len * (uint64_t)(*n_items), "vector body too large for heap");

	return 1;
}

static int32_t pointless_validate_bitvector_heap(pointless_t* p, pointless_value_t* v, const char** error)
{
	assert(v->data.data_u32 < p->header->n_vector);
	uint32_t offset = p->bitvector_offsets[v->data.data_u32];

	// uint32_t | bits
	POINTLESS_REQUIRE_HEAP(offset, sizeof(uint32_t), "bitvector too large for heap");

	uint32_t* n_bits = (uint32_t*)((char*)p->heap_ptr + offset);

	POINTLESS_REQUIRE_HEAP(offset, sizeof(uint32_t) + ICEIL(*n_bits, 8), "bitvector too large for heap");

	return 1;
}

static int32_t pointless_validate_unicode_heap(pointless_t* p, pointless_value_t* v, const char** error)
{
	assert(v->data.data_u32 < p->header->n_unicode);
	uint32_t offset = p->unicode_offsets[v->data.data_u32];

	// uint32_t | pointless_char_t * (len + 1)
	POINTLESS_REQUIRE_HEAP(offset, sizeof(uint32_t), "unicode too large for heap");

	uint32_t* s_len = (uint32_t*)((char*)p->heap_ptr + offset);

	POINTLESS_REQUIRE_HEAP(offset, sizeof(uint32_t) + (uint64_t)(*s_len + 1) * (uint64_t)sizeof(pointless_char_t), "unicode too large for heap");

	pointless_char_t* s = (pointless_char_t*)(s_len + 1);

	uint32_t i;

	for (i = 0; i < *s_len; i++) {
		if (s[i] == 0) {
			*error = "unicode premature end-of-string";
			return 0;
		}
	}

	if (s[i] != 0) {
		*error = "unicode missing end-of-string";
		return 0;
	}

	// right, try all the conversions
	int retval = 1;

	uint16_t* ucs2 = pointless_unicode_ucs4_to_ucs2(s, error);

	if (ucs2 == 0)
		goto cleanup;

	retval = 1;

cleanup:

	free(ucs2);

	return retval;
}

static int32_t pointless_validate_set_heap(pointless_t* p, pointless_value_t* v, const char** error)
{
	// simple stuff, not allowed to check children
	assert(v->data.data_u32 < p->header->n_set);
	uint32_t offset = p->set_offsets[v->data.data_u32];

	// get header
	POINTLESS_REQUIRE_HEAP(offset, sizeof(pointless_set_header_t), "set header too large for heap");
	pointless_set_header_t* header = (pointless_set_header_t*)((char*)p->heap_ptr + offset);

	// hash/key vectors must be of a certain type
	if (header->hash_vector.type != POINTLESS_VECTOR_U32) {
		*error = "set hash vector not of type POINTLESS_VECTOR_U32";
		return 0;
	}

	if (header->key_vector.type != POINTLESS_VECTOR_VALUE_HASHABLE) {
		*error = "set key vector not of type POINTLESS_VECTOR_VALUE_HASHABLE";
		return 0;
	}

	return 1;
}

static int32_t pointless_validate_map_heap(pointless_t* p, pointless_value_t* v, const char** error)
{
	// simple stuff first
	assert(v->data.data_u32 < p->header->n_map);
	uint32_t offset = p->map_offsets[v->data.data_u32];

	// get header
	POINTLESS_REQUIRE_HEAP(offset, sizeof(pointless_map_header_t), "map header too large for heap");
	pointless_map_header_t* header = (pointless_map_header_t*)((char*)p->heap_ptr + offset);

	// hash/key vectors must be of a certain type
	if (header->hash_vector.type != POINTLESS_VECTOR_U32) {
		*error = "map hash vector not of type POINTLESS_VECTOR_U32";
		return 0;
	}

	if (header->key_vector.type != POINTLESS_VECTOR_VALUE_HASHABLE) {
		*error = "map key vector not of type POINTLESS_VECTOR_VALUE_HASHABLE";
		return 0;
	}

	if (header->value_vector.type != POINTLESS_VECTOR_VALUE_HASHABLE && header->value_vector.type != POINTLESS_VECTOR_VALUE) {
		*error = "map key vector not of type POINTLESS_VECTOR_VALUE or POINTLESS_VECTOR_VALUE_HASHABLE";
		return 0;
	}

	return 1;
}

int32_t pointless_validate_heap_value(pointless_t* p, pointless_value_t* v, const char** error)
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
		case POINTLESS_VECTOR_FLOAT:
			return pointless_validate_vector_heap(p, v, error);
		case POINTLESS_VECTOR_EMPTY:
			break;
		case POINTLESS_UNICODE:
			return pointless_validate_unicode_heap(p, v, error);
		case POINTLESS_BITVECTOR:
			return pointless_validate_bitvector_heap(p, v, error);
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			break;
		case POINTLESS_SET_VALUE:
			return pointless_validate_set_heap(p, v, error);
		case POINTLESS_MAP_VALUE_VALUE:
			return pointless_validate_map_heap(p, v, error);
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
