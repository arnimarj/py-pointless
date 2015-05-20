#include <pointless/pointless_recreate.h>

typedef struct {
	// *_read_handle -> *_create_handle
	pointless_create_t* c;
	pointless_t* p;

	int normalize_bitvector;

	const char** error;

	uint32_t* string_unicode_r_c_mapping;
	uint32_t* vector_r_c_mapping;
	uint32_t* bitvector_r_c_mapping;
	uint32_t* set_r_c_mapping;
	uint32_t* map_r_c_mapping;
} pointless_recreate_state_t;

static uint32_t* pointless_malloc_uint32_init(uint32_t n_items, uint32_t init_value)
{
	uint32_t i;
	uint32_t* v = (uint32_t*)pointless_malloc(sizeof(uint32_t) * n_items);

	if (v == 0)
		return 0;

	for (i = 0; i < n_items; i++)
		v[i] = init_value;

	return v;
}

#define POINTLESS_RECREATE_FUNC_1(func, param_1) \
handle = func(param_1);\
if (handle == POINTLESS_CREATE_VALUE_FAIL) {\
	*state->error = #func " failure";\
	return POINTLESS_CREATE_VALUE_FAIL;\
}

#define POINTLESS_RECREATE_FUNC_2(func, param_1, param_2) \
handle = func((param_1), (param_2));\
if (handle == POINTLESS_CREATE_VALUE_FAIL) {\
	*state->error = #func " failure";\
	return POINTLESS_CREATE_VALUE_FAIL;\
}

#define POINTLESS_RECREATE_FUNC_3(func, param_1, param_2, param_3) \
handle = func((param_1), (param_2), (param_3));\
if (handle == POINTLESS_CREATE_VALUE_FAIL) {\
	*state->error = #func " failure";\
	return POINTLESS_CREATE_VALUE_FAIL;\
}

static uint32_t pointless_recreate_convert_rec(pointless_recreate_state_t* state, pointless_value_t* v, uint32_t depth)
{
	// in case of cycles, return the previously created create-time handle
	uint32_t handle = UINT32_MAX, child_handle = UINT32_MAX, key_handle = UINT32_MAX, value_handle = UINT32_MAX;

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
			handle = state->vector_r_c_mapping[v->data.data_u32];
			break;
		case POINTLESS_UNICODE_:
		case POINTLESS_STRING_:
			handle = state->string_unicode_r_c_mapping[v->data.data_u32];
			break;
		case POINTLESS_BITVECTOR:
			handle = state->bitvector_r_c_mapping[v->data.data_u32];
			break;
		case POINTLESS_SET_VALUE:
			handle = state->set_r_c_mapping[v->data.data_u32];
			break;
		case POINTLESS_MAP_VALUE_VALUE:
			handle = state->map_r_c_mapping[v->data.data_u32];
			break;
	}

	if (handle != UINT32_MAX)
		return handle;

	handle = POINTLESS_CREATE_VALUE_FAIL;

	uint32_t n_items = 0, i = 0, n_bits = 0;
	pointless_value_t* child_v = 0;
	pointless_value_t* key = 0;
	pointless_value_t* value = 0;
	void* bits = 0;
	void* source_bits = 0;

	if (pointless_is_vector_type(v->type))
		n_items = pointless_reader_vector_n_items(state->p, v);

	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			handle = pointless_create_vector_value(state->c);

			if (handle == POINTLESS_CREATE_VALUE_FAIL) {
				*state->error = "pointless_create_vector_value() failure";
				return POINTLESS_CREATE_VALUE_FAIL;
			}

			state->vector_r_c_mapping[v->data.data_u32] = handle;

			child_v = pointless_reader_vector_value(state->p, v);

			for (i = 0; i < n_items; i++) {
				child_handle = pointless_recreate_convert_rec(state, &child_v[i], depth + 1);

				if (child_handle == POINTLESS_CREATE_VALUE_FAIL)
					return POINTLESS_CREATE_VALUE_FAIL;

				if (pointless_create_vector_value_append(state->c, handle, child_handle) == POINTLESS_CREATE_VALUE_FAIL) {
					*state->error = "pointless_create_vector_value_append() failure";
					return POINTLESS_CREATE_VALUE_FAIL;
				}
			}

			return handle;
		case POINTLESS_VECTOR_I8:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_i8_owner, state->c, pointless_reader_vector_i8(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_U8:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_u8_owner, state->c, pointless_reader_vector_u8(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_I16:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_i16_owner, state->c, pointless_reader_vector_i16(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_U16:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_u16_owner, state->c, pointless_reader_vector_u16(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_I32:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_i32_owner, state->c, pointless_reader_vector_i32(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_U32:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_u32_owner, state->c, pointless_reader_vector_u32(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_I64:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_i64_owner, state->c, pointless_reader_vector_i64(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_U64:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_u64_owner, state->c, pointless_reader_vector_u64(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_FLOAT:
			POINTLESS_RECREATE_FUNC_3(pointless_create_vector_float_owner, state->c, pointless_reader_vector_float(state->p, v), n_items);
			state->vector_r_c_mapping[v->data.data_u32] = handle;
			return handle;
		case POINTLESS_VECTOR_EMPTY:
			POINTLESS_RECREATE_FUNC_1(pointless_create_vector_value, state->c);
			// note: empty vectors don't have any data on the heap, so there is no
			//       need to mark vector_r_c_mapping
			return handle;
		case POINTLESS_UNICODE_:
			POINTLESS_RECREATE_FUNC_2(pointless_create_unicode_ucs4, state->c, pointless_reader_unicode_value_ucs4(state->p, v));
			state->string_unicode_r_c_mapping[v->data.data_u32] = handle;
			if (handle == POINTLESS_CREATE_VALUE_FAIL)
				*state->error = "out of memory";
			return handle;
		case POINTLESS_STRING_:
			POINTLESS_RECREATE_FUNC_2(pointless_create_string_ascii, state->c, pointless_reader_string_value_ascii(state->p, v));
			state->string_unicode_r_c_mapping[v->data.data_u32] = handle;
			if (handle == POINTLESS_CREATE_VALUE_FAIL)
				*state->error = "out of memory";
			return handle;
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			handle = pointless_create_bitvector_compressed(state->c, v);
			if (handle == POINTLESS_CREATE_VALUE_FAIL)
				*state->error = "out of memory";
			return handle;
	
		case POINTLESS_BITVECTOR:
			n_bits = pointless_reader_bitvector_n_bits(state->p, v);
			bits = pointless_calloc(ICEIL(n_bits, 8), 1);

			if (bits == 0) {
				*state->error = "out of memory";
				return POINTLESS_CREATE_VALUE_FAIL;
			}

			source_bits = (void*)((uint32_t*)pointless_reader_bitvector_buffer(state->p, v) + 1);
			memcpy(bits, source_bits, ICEIL(n_bits, 8));

			if (state->normalize_bitvector)
				handle = pointless_create_bitvector(state->c, bits, n_bits);
			else
				handle = pointless_create_bitvector_no_normalize(state->c, bits, n_bits);

			pointless_free(bits);
			bits = 0;

			if (handle == POINTLESS_CREATE_VALUE_FAIL) {
				*state->error = "pointless_create_bitvector() failure";
				return POINTLESS_CREATE_VALUE_FAIL;
			}

			state->bitvector_r_c_mapping[v->data.data_u32] = handle;

			return handle;
		case POINTLESS_SET_VALUE:
			POINTLESS_RECREATE_FUNC_1(pointless_create_set, state->c);
			state->set_r_c_mapping[v->data.data_u32] = handle;

			i = 0;

			while (pointless_reader_set_iter(state->p, v, &key, &i)) {
				key_handle = pointless_recreate_convert_rec(state, key, depth + 1);

				if (key_handle == POINTLESS_CREATE_VALUE_FAIL)
					return POINTLESS_CREATE_VALUE_FAIL;

				if (pointless_create_set_add(state->c, handle, key_handle) == POINTLESS_CREATE_VALUE_FAIL) {
					*state->error = "pointless_create_set_add() failure";
					return POINTLESS_CREATE_VALUE_FAIL;
				}
			}

			return handle;
		case POINTLESS_MAP_VALUE_VALUE:
			POINTLESS_RECREATE_FUNC_1(pointless_create_map, state->c);
			state->map_r_c_mapping[v->data.data_u32] = handle;

			i = 0;

			while (pointless_reader_map_iter(state->p, v, &key, &value, &i)) {
				key_handle = pointless_recreate_convert_rec(state, key, depth + 1);

				if (key_handle == POINTLESS_CREATE_VALUE_FAIL)
					return POINTLESS_CREATE_VALUE_FAIL;

				value_handle = pointless_recreate_convert_rec(state, value, depth + 1);

				if (value_handle == POINTLESS_CREATE_VALUE_FAIL)
					return POINTLESS_CREATE_VALUE_FAIL;

				if (pointless_create_map_add(state->c, handle, key_handle, value_handle) == POINTLESS_CREATE_VALUE_FAIL) {
					*state->error = "pointless_create_map_add() failure";
					return POINTLESS_CREATE_VALUE_FAIL;
				}
			}

			return handle;
		case POINTLESS_EMPTY_SLOT:
			POINTLESS_RECREATE_FUNC_1(pointless_create_empty_slot, state->c);
			return handle;
		case POINTLESS_I32:
			POINTLESS_RECREATE_FUNC_2(pointless_create_i32, state->c, pointless_value_get_i32(v->type, &v->data));
			return handle;
		case POINTLESS_U32:
			POINTLESS_RECREATE_FUNC_2(pointless_create_u32, state->c, pointless_value_get_u32(v->type, &v->data));
			return handle;
		case POINTLESS_FLOAT:
			POINTLESS_RECREATE_FUNC_2(pointless_create_float, state->c, pointless_value_get_float(v->type, &v->data));
			return handle;
		case POINTLESS_BOOLEAN:
			if (pointless_value_get_bool(v->type, &v->data)) {
				POINTLESS_RECREATE_FUNC_1(pointless_create_boolean_true, state->c);
			} else {
				POINTLESS_RECREATE_FUNC_1(pointless_create_boolean_false, state->c);
			}
			return handle;
		case POINTLESS_NULL:
			POINTLESS_RECREATE_FUNC_1(pointless_create_null, state->c);
			return handle;
	}

	*state->error = "unknown type";
	return POINTLESS_CREATE_VALUE_FAIL;
}

uint32_t pointless_recreate_value(pointless_t* p_in, pointless_value_t* v_in, pointless_create_t* c_out, const char** error)
{
	pointless_recreate_state_t state;
	uint32_t handle = POINTLESS_CREATE_VALUE_FAIL;

	state.p = p_in;
	state.c = c_out;

	state.error = error;

	state.string_unicode_r_c_mapping = pointless_malloc_uint32_init(p_in->header->n_string_unicode, UINT32_MAX);
	state.vector_r_c_mapping = pointless_malloc_uint32_init(p_in->header->n_vector, UINT32_MAX);
	state.bitvector_r_c_mapping = pointless_malloc_uint32_init(p_in->header->n_bitvector, UINT32_MAX);
	state.set_r_c_mapping = pointless_malloc_uint32_init(p_in->header->n_set, UINT32_MAX);
	state.map_r_c_mapping = pointless_malloc_uint32_init(p_in->header->n_map, UINT32_MAX);
	state.normalize_bitvector = 1;

	if (state.string_unicode_r_c_mapping == 0 || state.vector_r_c_mapping == 0 || state.bitvector_r_c_mapping == 0) {
		*error = "out of memory";
		goto error;
	}

	if (state.set_r_c_mapping == 0 || state.map_r_c_mapping == 0) {
		*error = "out of memory";
		goto error;
	}

	handle = pointless_recreate_convert_rec(&state, v_in, 0);

error:

	pointless_free(state.string_unicode_r_c_mapping);
	pointless_free(state.vector_r_c_mapping);
	pointless_free(state.bitvector_r_c_mapping);
	pointless_free(state.set_r_c_mapping);
	pointless_free(state.map_r_c_mapping);

	return handle;
}

static int pointless_recreate_(const char* fname_in, const char* fname_out, const char** error, int n_bits)
{
	// open source
	pointless_t p;

	if (!pointless_open_f(&p, fname_in, 0, error))
		return 0;

	// create destination
	pointless_create_t c;

	if (n_bits == 32)
		pointless_create_begin_32(&c);
	else
		pointless_create_begin_64(&c);

	uint32_t root = pointless_recreate_value(&p, pointless_root(&p), &c, error);

	if (root == POINTLESS_CREATE_VALUE_FAIL) {
		pointless_close(&p);
		pointless_create_end(&c);
		return 0;
	}

	pointless_create_set_root(&c, root);

	if (!pointless_create_output_and_end_f(&c, fname_out, error)) {
		pointless_close(&p);
		pointless_create_end(&c);
		return 0;
	}

	pointless_close(&p);
	return 1;
}

int pointless_recreate_32(const char* fname_in, const char* fname_out, const char** error)
{
	return pointless_recreate_(fname_in, fname_out, error, 32);
}

int pointless_recreate_64(const char* fname_in, const char* fname_out, const char** error)
{
	return pointless_recreate_(fname_in, fname_out, error, 64);
}
