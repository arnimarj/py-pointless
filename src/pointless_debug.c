#include <pointless/pointless_debug.h>

typedef struct {
	FILE* out;
	pointless_t* p;
	const char** error;
	uint32_t stack[POINTLESS_MAX_DEPTH];
	uint32_t stack_depth;
	uint32_t sort_set;
	uint32_t sort_map;
} pointless_debug_state_t;

static uint32_t pointless_print_has_container(pointless_debug_state_t* state, pointless_value_t* v)
{
	uint32_t i, container_id = pointless_container_id(state->p, v);

	for (i = 0; i < state->stack_depth; i++) {
		if (state->stack[i] == container_id)
			return 1;
	}

	return 0;
}

static uint32_t pointless_print_push_container(pointless_debug_state_t* state, pointless_value_t* v)
{
	if (state->stack_depth == POINTLESS_MAX_DEPTH) {
		*state->error = "maximum recursion depth reached, file should not have passed validation";
		return 0;
	}

	uint32_t container_id = pointless_container_id(state->p, v);

	state->stack[state->stack_depth] = container_id;
	state->stack_depth += 1;

	return 1;
}

static void pointless_print_pop_container(pointless_debug_state_t* state)
{
	assert(state->stack_depth != 0);
	state->stack_depth -= 1;
}

static void pointless_print_integer(pointless_debug_state_t* state, pointless_value_t* v)
{
	int64_t iv;

	switch (v->type) {
		case POINTLESS_I32:
			iv = (int64_t)pointless_value_get_i32(v->type, &v->data);
			break;
		case POINTLESS_U32:
			iv = (int64_t)pointless_value_get_u32(v->type, &v->data);
			break;
		default:
			assert(0);
			iv = 0;
			break;
	}

	fprintf(state->out, "%lli", (long long int)iv);
}

static void pointless_print_float(pointless_debug_state_t* state, pointless_value_t* v)
{
	fprintf(state->out, "%f", pointless_value_get_float(v->type, &v->data));
}

static void pointless_print_boolean(pointless_debug_state_t* state, pointless_value_t* v)
{
	fprintf(state->out, pointless_value_get_bool(v->type, &v->data) ? "True" : "False");
}

static void pointless_print_null(pointless_debug_state_t* state, pointless_value_t* v)
{
	assert(v->type == POINTLESS_NULL);
	fprintf(state->out, "NULL");
}

static void pointless_print_vector_other(pointless_debug_state_t* state, pointless_value_t* v)
{
	uint32_t i, n = pointless_reader_vector_n_items(state->p, v);
	int is_float = 0;
	int is_signed = 0;
	int is_unsigned = 0;

	unsigned long long int uu = 0;
	long long int ii = 0;
	float ff = 0.0;

	fprintf(state->out, "H[");

	for (i = 0; i < n; i++) {
		switch (v->type) {
			case POINTLESS_VECTOR_I8:
				ii= (long long int)(pointless_reader_vector_i8(state->p, v)[i]);
				is_signed = 1;
				break;
			case POINTLESS_VECTOR_U8:
				uu = (unsigned long long int)(pointless_reader_vector_u8(state->p, v)[i]);
				is_unsigned = 1;
				break;
			case POINTLESS_VECTOR_I16:
				ii = (long long int)(pointless_reader_vector_i16(state->p, v)[i]);
				is_signed = 1;
				break;
			case POINTLESS_VECTOR_U16:
				uu = (unsigned long long int)(pointless_reader_vector_u16(state->p, v)[i]);
				is_unsigned = 1;
				break;
			case POINTLESS_VECTOR_I32:
				ii = (long long int)(pointless_reader_vector_i32(state->p, v)[i]);
				is_signed = 1;
				break;
			case POINTLESS_VECTOR_U32:
				uu = (unsigned long long int)(pointless_reader_vector_u32(state->p, v)[i]);
				is_unsigned = 1;
				break;
			case POINTLESS_VECTOR_I64:
				ii = (long long int)(pointless_reader_vector_i64(state->p, v)[i]);
				is_signed = 1;
				break;
			case POINTLESS_VECTOR_U64:
				uu = (unsigned long long int)(pointless_reader_vector_u64(state->p, v)[i]);
				is_unsigned = 1;
				break;
			case POINTLESS_VECTOR_FLOAT:
				ff = pointless_reader_vector_float(state->p, v)[i];
				is_float = 1;
				break;
			default:
				assert(0);
				break;
		}

		if (is_unsigned)
			fprintf(state->out, "%llu", uu);
		else if (is_signed)
			fprintf(state->out, "%lli", ii);
		else if (is_float)
			fprintf(state->out, "%.f", ff);
		else
			assert(0);

		if (i + 1 < n)
			fprintf(state->out, ",");
	}

	fprintf(state->out, "]");
}

static void pointless_print_unicode(pointless_debug_state_t* state, pointless_value_t* v)
{
	assert(v->type == POINTLESS_UNICODE_);
	uint32_t* s = pointless_reader_unicode_value_ucs4(state->p, v);

	fprintf(state->out, "\"");

	while (*s) {
		if (*s < 128)
			fprintf(state->out, "%c", (char)*s);
		else
			fprintf(state->out, "?");

		s++;
	}

	fprintf(state->out, "\"");
}

static void pointless_print_string(pointless_debug_state_t* state, pointless_value_t* v)
{
	assert(v->type == POINTLESS_STRING_);
	uint8_t* s = pointless_reader_string_value_ascii(state->p, v);

	fprintf(state->out, "\"");

	while (*s) {
		if (*s < 128)
			fprintf(state->out, "%c", (char)*s);
		else
			fprintf(state->out, "?");

		s++;
	}

	fprintf(state->out, "\"");
}
static void pointless_print_bitvector(pointless_debug_state_t* state, pointless_value_t* v)
{
	uint32_t i, n_bits = pointless_reader_bitvector_n_bits(state->p, v);

	fprintf(state->out, "<");

	for (i = 0; i < n_bits; i++)
		fprintf(state->out, "%i", pointless_reader_bitvector_is_set(state->p, v, i) ? 1 : 0);

	fprintf(state->out, ">");
}

static void pointless_print_value(pointless_debug_state_t* state, pointless_value_t* v, uint32_t depth);

static void pointless_print_vector_value(pointless_debug_state_t* state, pointless_value_t* v, uint32_t depth)
{
	assert(v->type == POINTLESS_VECTOR_VALUE || v->type == POINTLESS_VECTOR_VALUE_HASHABLE || v->type == POINTLESS_VECTOR_EMPTY);

	uint32_t i, n_values = pointless_reader_vector_n_items(state->p, v);
	pointless_value_t* values = pointless_reader_vector_value(state->p, v);

	if (v->type == POINTLESS_VECTOR_VALUE_HASHABLE)
		fprintf(state->out, "H");

	fprintf(state->out, "[");

	for (i = 0; i < n_values; i++) {
		pointless_print_value(state, &values[i], depth + 1);

		if (i + 1 < n_values)
			fprintf(state->out, ",");
	}

	fprintf(state->out, "]");
}

typedef struct {
	pointless_t* p;
	pointless_value_t* keys;
	pointless_value_t* values;
	const char** error;
} pv_sort_state_t;

static int pv_cmp(int a, int b, int* c, void* user)
{
	pv_sort_state_t* state = (pv_sort_state_t*)user;
	pointless_complete_value_t v_a = pointless_value_to_complete(&state->keys[a]);
	pointless_complete_value_t v_b = pointless_value_to_complete(&state->keys[b]);
	int32_t v = pointless_cmp_reader(state->p, &v_a, state->p, &v_b, state->error);
	*c = (int)v;
	return (state->error != 0);
}

static void pv_swap(int a, int b, void* user)
{
	pv_sort_state_t* state = (pv_sort_state_t*)user;
	pointless_value_t t = state->keys[a];
	state->keys[a] = state->keys[b];
	state->keys[b] = t;

	if (state->values) {
		t = state->values[a];
		state->values[a] = state->values[b];
		state->values[b] = t;
	}
}

static void pointless_print_set(pointless_debug_state_t* state, pointless_value_t* v, uint32_t depth)
{
	assert(v->type == POINTLESS_SET_VALUE);

	uint32_t i = 0, j = 0, first = 1;
	pointless_value_t* k = 0;

	fprintf(state->out, "set([");

	if (pointless_print_has_container(state, v)) {
		fprintf(state->out, "...");
	} else {
		if (!pointless_print_push_container(state, v))
			return;

		if (state->sort_set) {
			// allocate keys buffer
			uint32_t n_keys = pointless_reader_set_n_items(state->p, v);
			pointless_value_t* keys = (pointless_value_t*)pointless_malloc(sizeof(pointless_value_t) * n_keys);

			if (keys == 0) {
				*state->error = "out of memory";
				return;
			}

			i = 0, j = 0;

			while (pointless_reader_set_iter(state->p, v, &k, &i))
				keys[j++] = *k;

			assert(j == n_keys);

			pv_sort_state_t sort_state;
			sort_state.p = state->p;
			sort_state.keys = keys;
			sort_state.values = 0;
			sort_state.error = state->error;

			// sort keys
			if (!bentley_sort_((int)n_keys, pv_cmp, pv_swap, (void*)&sort_state)) {
				pointless_free(keys);
				return;
			}

			// print them out, in order
			for (i = 0; i < n_keys; i++) {
				if (i > 0)
					fprintf(state->out, ", ");

				pointless_print_value(state, &keys[i], depth + 1);
			}

			pointless_free(keys);
			keys = 0;

		} else {
			while (pointless_reader_set_iter(state->p, v, &k, &i)) {
				if (!first)
					fprintf(state->out, ", ");

				pointless_print_value(state, k, depth + 1);

				first = 0;
			}
		}

		pointless_print_pop_container(state);
	}

	fprintf(state->out, "])");
}

static void pointless_print_map(pointless_debug_state_t* state, pointless_value_t* v, uint32_t depth)
{
	assert(v->type == POINTLESS_MAP_VALUE_VALUE);

	uint32_t i = 0, j = 0, is_first = 1;

	fprintf(state->out, "{");

	if (pointless_print_has_container(state, v)) {
		fprintf(state->out, "...");
	} else {
		pointless_value_t* key = 0;
		pointless_value_t* value = 0;

		if (!pointless_print_push_container(state, v))
			return;

		if (state->sort_map) {
			// allocate keys buffer
			uint32_t n_keys = pointless_reader_map_n_items(state->p, v);
			pointless_value_t* keys = (pointless_value_t*)pointless_malloc(sizeof(pointless_value_t) * n_keys);
			pointless_value_t* values = (pointless_value_t*)pointless_malloc(sizeof(pointless_value_t) * n_keys);

			if (keys == 0 || values == 0) {
				pointless_free(keys);
				pointless_free(values);
				*state->error = "out of memory";
				return;
			}

			i = 0, j = 0;

			while (pointless_reader_map_iter(state->p, v, &key, &value, &i)) {
				keys[j] = *key;
				values[j] = *value;
				j += 1;
			}

			assert(j == n_keys);

			pv_sort_state_t sort_state;
			sort_state.p = state->p;
			sort_state.keys = keys;
			sort_state.values = values;
			sort_state.error = state->error;

			// sort keys
			if (!bentley_sort_((int)n_keys, pv_cmp, pv_swap, (void*)&sort_state)) {
				pointless_free(keys);
				pointless_free(values);
				return;
			}

			// print them out, in order
			for (i = 0; i < n_keys; i++) {
				if (i > 0)
					fprintf(state->out, ", ");

				pointless_print_value(state, &keys[i], depth + 1);
				fprintf(state->out, ": ");
				pointless_print_value(state, &values[i], depth + 1);
			}

			pointless_free(keys);
			pointless_free(values);

			keys = 0;
			values = 0;
		} else {
			while (pointless_reader_map_iter(state->p, v, &key, &value, &i)) {
				if (!is_first)
					fprintf(state->out, ", ");

				pointless_print_value(state, key, depth + 1);
				fprintf(state->out, ": ");
				pointless_print_value(state, value, depth + 1);

				is_first = 0;
			}
		}

		pointless_print_pop_container(state);
	}

	fprintf(state->out, "}");
}

static void pointless_print_value(pointless_debug_state_t* state, pointless_value_t* v, uint32_t depth)
{
	switch (v->type) {
		case POINTLESS_UNICODE_:
			pointless_print_unicode(state, v);
			break;
		case POINTLESS_STRING_:
			pointless_print_string(state, v);
			break;
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			assert(v->data.data_u32 < state->p->header->n_vector);
			if (pointless_print_has_container(state, v)) {
				fprintf(state->out, "[...]");
			} else {
				if (!pointless_print_push_container(state, v))
					return;

				pointless_print_vector_value(state, v, depth);
				pointless_print_pop_container(state);
			}
			break;
		case POINTLESS_VECTOR_EMPTY:
			fprintf(state->out, "[]");
			break;
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U64:
		case POINTLESS_VECTOR_FLOAT:
			pointless_print_vector_other(state, v);
			break;
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_PACKED:
			pointless_print_bitvector(state, v);
			break;
		case POINTLESS_I32:
		case POINTLESS_U32:
			pointless_print_integer(state, v);
			break;
		case POINTLESS_FLOAT:
			pointless_print_float(state, v);
			break;
		case POINTLESS_BOOLEAN:
			pointless_print_boolean(state, v);
			break;
		case POINTLESS_NULL:
			pointless_print_null(state, v);
			break;
		case POINTLESS_SET_VALUE:
			assert(v->data.data_u32 < state->p->header->n_set);
			pointless_print_set(state, v, depth);
			break;
		case POINTLESS_MAP_VALUE_VALUE:
			assert(v->data.data_u32 < state->p->header->n_map);
			pointless_print_map(state, v, depth);
			break;
		default:
			// should not have passed validation
			fprintf(state->out, "<UNKNOWN:%u>", (unsigned int)v->type);
			assert(0);
			break;
	}
}

static int pointless_debug_print_priv(pointless_t* p, pointless_value_t* v, FILE* out, const char** error)
{
	pointless_debug_state_t state;
	state.p = p;
	state.out = out;
	state.error = error;
	state.stack_depth = 0;
	state.sort_set = 1;
	state.sort_map = 1;

	pointless_print_value(&state, v, 0);
	fprintf(state.out, "\n");

	return (*state.error == 0);
}

int pointless_debug_print(pointless_t* p, FILE* out, const char** error)
{
	pointless_value_t* root = pointless_root(p);
	return pointless_debug_print_priv(p, root, out, error);
}

int pointless_debug_print_value(pointless_t* p, pointless_value_t* v, FILE* out, const char** error)
{
	return pointless_debug_print_priv(p, v, out, error);
}
