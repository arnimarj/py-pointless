#include <pointless/pointless_validate.h>

int32_t pointless_validate_inline_invariants(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
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
		case POINTLESS_BITVECTOR:
		case POINTLESS_SET_VALUE:
		case POINTLESS_MAP_VALUE_VALUE:
			break;
		case POINTLESS_BITVECTOR_PACKED:
			if (v->data.bitvector_packed.n_bits > 27) {
				*error = "packed bitvector has too many bits";
				return 0;
			}

			break;
		case POINTLESS_VECTOR_EMPTY:
			if (v->data.data_u32 != 0) {
				*error = "empty vectors must contain data value 0";
				return 0;
			}

			break;
		case POINTLESS_UNICODE_:
		case POINTLESS_STRING_:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_FLOAT:
			break;
		case POINTLESS_BOOLEAN:
			if (v->data.data_u32 != 0 && v->data.data_u32 != 1) {
				*error = "booleans must contain 0 or 1 in data field";
				return 0;
			}

			break;
		case POINTLESS_NULL:
			if (v->data.data_u32 != 0) {
				*error = "NULLs must contain 0 in the data field";
				return 0;
			}

			break;
		case POINTLESS_EMPTY_SLOT:
			if (v->data.data_u32 != 0) {
				*error = "EMPTY_SLOT must contain 0 in the data field";
				return 0;
			}

			break;
		default:
			*error = "unknown type";
			return 0;
	}

	return 1;
}

int32_t pointless_validate_heap_ref(pointless_validate_context_t* context, pointless_value_t* v, const char** error)
{
	switch (v->type) {
		case POINTLESS_UNICODE_:
		case POINTLESS_STRING_:
			if (v->data.data_u32 >= context->p->header->n_string_unicode) {
				*error = "string/unicode reference out of bounds";
				return 0;
			}

			break;
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
			if (v->data.data_u32 >= context->p->header->n_vector) {
				*error = "vector reference out of bounds";
				return 0;
			}

			break;
		case POINTLESS_BITVECTOR:
			if (v->data.data_u32 >= context->p->header->n_bitvector) {
				*error = "bitvector reference out of bounds";
				return 0;
			}

			break;
		case POINTLESS_SET_VALUE:
			if (v->data.data_u32 >= context->p->header->n_set) {
				*error = "set reference out of bounds";
				return 0;
			}

			break;
		case POINTLESS_MAP_VALUE_VALUE:
			if (v->data.data_u32 >= context->p->header->n_map) {
				*error = "map reference out of bounds";
				return 0;
			}

			break;
		case POINTLESS_VECTOR_EMPTY:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_PACKED:
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_FLOAT:
		case POINTLESS_BOOLEAN:
		case POINTLESS_EMPTY_SLOT:
		case POINTLESS_NULL:
			break;
		default:
			*error = "unknown type X";
			return 0;
	}

	return 1;
}
