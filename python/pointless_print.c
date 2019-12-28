#include "../pointless_ext.h"

// yes I know, this is a global variable, but this is a similar trick to avoid cycles
// as CPython does, since it guarantees that access to these functions is serialized
typedef struct {
	pointless_dynarray_t _string_32;
	uint32_t _depth;
	uint32_t _container_ids[POINTLESS_MAX_DEPTH];
} _pypointless_print_state_t;

static int32_t print_state_has_container(_pypointless_print_state_t* state, uint32_t container_id)
{
	uint32_t i;

	for (i = 0; i < state->_depth; i++) {
		if (state->_container_ids[i] == container_id)
			return 1;
	}

	return 0;
}

static int print_state_push(_pypointless_print_state_t* state, uint32_t container_id)
{
	if (state->_depth >= POINTLESS_MAX_DEPTH) {
		PyErr_SetString(PyExc_ValueError, "maximum recursion depth reached during print");
		return 0;
	}

	state->_container_ids[state->_depth++] = container_id;
	return 1;
}

static void print_state_pop(_pypointless_print_state_t* state)
{
	state->_depth -= 1;
}

static int _pypointless_print_append_ucs4_(_pypointless_print_state_t* state, uint32_t* s, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uint32_t value = (uint32_t)(*(s + i));

		if (!pointless_dynarray_push(&state->_string_32, &value)) {
			PyErr_NoMemory();
			return 0;
		}
	}

	return 1;
}
static int _pypointless_print_append_ucs2_(_pypointless_print_state_t* state, uint16_t* s, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uint32_t value = (uint32_t)(*(s + i));

		if (!pointless_dynarray_push(&state->_string_32, &value)) {
			PyErr_NoMemory();
			return 0;
		}
	}

	return 1;
}
static int _pypointless_print_append_ucs1_(_pypointless_print_state_t* state, uint8_t* s, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uint32_t value = (uint32_t)(*(s + i));

		if (!pointless_dynarray_push(&state->_string_32, &value)) {
			PyErr_NoMemory();
			return 0;
		}
	}

	return 1;
}

static int _pypointless_print_append_8_(_pypointless_print_state_t* state, const char* s)
{
	size_t len = strlen(s);
	for (size_t i = 0; i < len; i++) {
		uint32_t value = (uint32_t)(*(s + i));

		if (!pointless_dynarray_push(&state->_string_32, &value)) {
			PyErr_NoMemory();
			return 0;
		}
	}

	return 1;
}

static int _pypointless_unicode_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state);
static int _pypointless_string_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state);
static int _pypointless_vector_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state, uint32_t slice_i, uint32_t slice_n, int vector_as_tuple);
static int _pypointless_set_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state);
static int _pypointless_map_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state);
static int _pypointless_bitvector_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state);
static int _pypointless_bitvector_str_buffer(void* buffer, uint32_t n_bits, _pypointless_print_state_t* state);

static int _pypointless_str_rec(pointless_t* p, pointless_complete_value_t* v, _pypointless_print_state_t* state, uint32_t vector_slice_i, uint32_t vector_slice_n, int vector_as_tuple)
{
	// convert value
	pointless_value_t _v = pointless_value_from_complete(v);

	// both compressible types
	if (pointless_is_vector_type(v->type))
		return _pypointless_vector_str(p, &_v, state, vector_slice_i, vector_slice_n, vector_as_tuple);

	if (pointless_is_bitvector_type(v->type))
		return _pypointless_bitvector_str(p, &_v, state);

	// enough to hold any number
	char buffer[1024];

	switch (v->type) {
		case POINTLESS_UNICODE_:
			return _pypointless_unicode_str(p, &_v, state);
		case POINTLESS_STRING_:
			return _pypointless_string_str(p, &_v, state);
		case POINTLESS_SET_VALUE:
			return _pypointless_set_str(p, &_v, state);
		case POINTLESS_MAP_VALUE_VALUE:
			return _pypointless_map_str(p, &_v, state);
		case POINTLESS_I32:
		case POINTLESS_I64:
			snprintf(buffer, sizeof(buffer), "%lli", (long long)pointless_complete_value_get_as_i64(v->type, &v->complete_data));
			return _pypointless_print_append_8_(state, buffer);
		case POINTLESS_U32:
		case POINTLESS_U64:
			snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)pointless_complete_value_get_as_u64(v->type, &v->complete_data));
			return _pypointless_print_append_8_(state, buffer);
		case POINTLESS_FLOAT:
			snprintf(buffer, sizeof(buffer), "%f", pointless_complete_value_get_float(v->type, &v->complete_data));
			return _pypointless_print_append_8_(state, buffer);
		case POINTLESS_BOOLEAN:
			if (pointless_value_get_bool(_v.type, &_v.data))
				return _pypointless_print_append_8_(state, "True");
			else
				return _pypointless_print_append_8_(state, "False");
		case POINTLESS_NULL:
			return _pypointless_print_append_8_(state, "None");
	}

	PyErr_SetString(PyExc_ValueError, "internal print error in pypointless_str_rec()");
	return 0;
}

static int _pypointless_value_repr(pointless_t* p, PyObject* u_value, _pypointless_print_state_t* state)
{
	PyObject* s_value = PyObject_Repr(u_value);

	if (s_value == 0)
		return 0;

	assert(PyUnicode_Check(s_value));

	Py_ssize_t len = PyUnicode_GET_LENGTH(s_value);
	void* data = PyUnicode_DATA(s_value);
	int i = 0;

	switch (PyUnicode_KIND(s_value)) {
		case PyUnicode_WCHAR_KIND:
			PyErr_SetString(PyExc_ValueError, "wchar unicode unsupported");
			i = 0;
			break;
		case PyUnicode_1BYTE_KIND:
			i = _pypointless_print_append_ucs1_(state, (uint8_t*)data, len);
			break;
		case PyUnicode_2BYTE_KIND:
			i = _pypointless_print_append_ucs2_(state, (uint16_t*)data, len);
			break;
		case PyUnicode_4BYTE_KIND:
			i = _pypointless_print_append_ucs4_(state, (uint32_t*)data, len);
			break;
		default:
			assert(0);
			PyErr_SetString(PyExc_ValueError, "strange unicode");
			i = 0;
			break;
	}

	Py_DECREF(s_value);
	return i;
}

static int _pypointless_unicode_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state)
{
	PyObject* u_value = pypointless_value_unicode(p, v);

	if (u_value == 0)
		return 0;

	int i = _pypointless_value_repr(p, u_value, state);
	Py_DECREF(u_value);
	return i;
}

static int _pypointless_string_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state)
{
	PyObject* u_value = pypointless_value_string(p, v);

	if (u_value == 0)
		return 0;

	int i = _pypointless_value_repr(p, u_value, state);
	Py_DECREF(u_value);
	return i;
}

static int _pypointless_vector_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state, uint32_t slice_i, uint32_t slice_n, int vector_as_tuple)
{
	uint32_t container_id = pointless_container_id(p, v);

	if (print_state_has_container(state, container_id))
		return _pypointless_print_append_8_(state, "[...]");

	if (vector_as_tuple) {
		if (!_pypointless_print_append_8_(state, "("))
			return 0;
	} else {
		if (!_pypointless_print_append_8_(state, "["))
			return 0;
	}

	if (!print_state_push(state, container_id))
		return 0;

	uint32_t i;

	for (i = 0; i < slice_n; i++) {
		pointless_complete_value_t value = pointless_reader_vector_value_case(p, v, i + slice_i);
		pointless_value_t _value = pointless_value_from_complete(&value);
		uint32_t v_slice_i = 0;
		uint32_t v_slice_n = 0;

		if (pointless_is_vector_type(value.type)) {
			v_slice_i = 0;
			v_slice_n = pointless_reader_vector_n_items(p, &_value);
		}

		if (!_pypointless_str_rec(p, &value, state, v_slice_i, v_slice_n, vector_as_tuple)) {
			print_state_pop(state);
			return 0;
		}

		if (i + 1 < slice_n && !_pypointless_print_append_8_(state, ", ")) {
			print_state_pop(state);
			return 0;
		}
	}

	print_state_pop(state);

	if (vector_as_tuple) {
		if (slice_n == 1) {
			return _pypointless_print_append_8_(state, ",)");
		} else {
			return _pypointless_print_append_8_(state, ")");
		}
	} else {
		return _pypointless_print_append_8_(state, "]");
	}
}

static int _pypointless_set_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state)
{
	uint32_t container_id = pointless_container_id(p, v);

	if (print_state_has_container(state, container_id))
		return _pypointless_print_append_8_(state, "set([...])");

	if (!_pypointless_print_append_8_(state, "set(["))
		return 0;

	pointless_value_t* value = 0;

	if (!print_state_push(state, container_id))
		return 0;

	uint32_t i = 0, iter_state = 0, n_items = pointless_reader_set_n_items(p, v);

	while (pointless_reader_set_iter(p, v, &value, &iter_state)) {
		uint32_t v_slice_i = 0;
		uint32_t v_slice_n = 0;

		if (pointless_is_vector_type(value->type)) {
			v_slice_i = 0;
			v_slice_n = pointless_reader_vector_n_items(p, value);
		}

		pointless_complete_value_t _value = pointless_value_to_complete(value);

		if (!_pypointless_str_rec(p, &_value, state, v_slice_i, v_slice_n, 1)) {
			print_state_pop(state);
			return 0;
		}

		if (i + 1 < n_items && !_pypointless_print_append_8_(state, ", ")) {
			print_state_pop(state);
			return 0;
		}

		i += 1;
	}

	print_state_pop(state);

	return _pypointless_print_append_8_(state, "])");
}

static int _pypointless_map_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state)
{
	uint32_t container_id = pointless_container_id(p, v);

	if (print_state_has_container(state, container_id))
		return _pypointless_print_append_8_(state, "{...}");

	if (!_pypointless_print_append_8_(state, "{"))
		return 0;

	pointless_value_t* key = 0;
	pointless_value_t* value = 0;

	if (!print_state_push(state, container_id))
		return 0;

	uint32_t i = 0, iter_state = 0, n_items = pointless_reader_map_n_items(p, v);

	while (pointless_reader_map_iter(p, v, &key, &value, &iter_state)) {
		uint32_t v_slice_i_k = 0;
		uint32_t v_slice_n_k = 0;
		uint32_t v_slice_i_v = 0;
		uint32_t v_slice_n_v = 0;

		if (pointless_is_vector_type(key->type)) {
			v_slice_i_k = 0;
			v_slice_n_k = pointless_reader_vector_n_items(p, key);
		}

		if (pointless_is_vector_type(value->type)) {
			v_slice_i_v = 0;
			v_slice_n_v = pointless_reader_vector_n_items(p, value);
		}

		pointless_complete_value_t _key = pointless_value_to_complete(key);
		pointless_complete_value_t _value = pointless_value_to_complete(value);

		if (!_pypointless_str_rec(p, &_key, state, v_slice_i_k, v_slice_n_k, 1)) {
			print_state_pop(state);
			return 0;
		}

		if (!_pypointless_print_append_8_(state, ": ")) {
			print_state_pop(state);
			return 0;
		}

		if (!_pypointless_str_rec(p, &_value, state, v_slice_i_v, v_slice_n_v, 0)) {
			print_state_pop(state);
			return 0;
		}

		if (i + 1 < n_items && !_pypointless_print_append_8_(state, ", ")) {
			print_state_pop(state);
			return 0;
		}

		i += 1;
	}

	print_state_pop(state);

	return _pypointless_print_append_8_(state, "}");
}

static int _pypointless_bitvector_str(pointless_t* p, pointless_value_t* v, _pypointless_print_state_t* state)
{
	int32_t n_bits = (int32_t)pointless_reader_bitvector_n_bits(p, v);
	int32_t i;

	for (i = 0; i < n_bits; i++) {
		if (pointless_reader_bitvector_is_set(p, v, n_bits - i - 1)) {
			if (!_pypointless_print_append_8_(state, "1"))
				return 0;
		} else {
			if (!_pypointless_print_append_8_(state, "0"))
				return 0;
		}
	}

	return _pypointless_print_append_8_(state, "b");
}

static int _pypointless_bitvector_str_buffer(void* buffer, uint32_t n_bits, _pypointless_print_state_t* state)
{
	uint32_t i;

	for (i = 0; i < n_bits; i++) {
		if (bm_is_set_(buffer, n_bits - i - 1)) {
			if (!_pypointless_print_append_8_(state, "1"))
				return 0;
		} else {
			if (!_pypointless_print_append_8_(state, "0"))
				return 0;
		}
	}

	return _pypointless_print_append_8_(state, "b");
}

static PyObject* PyPointless_string_from_buffer_32(pointless_dynarray_t* s)
{
	// if 7-bit, string, otherwise unicode
	uint32_t* buffer = pointless_dynarray_buffer(s);
	size_t n_chars = pointless_dynarray_n_items(s);
	return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, buffer, n_chars);
}

PyObject* PyPointless_str(PyObject* py_object)
{
	// setup a state
	_pypointless_print_state_t state;
	state._depth = 0;
	pointless_dynarray_init(&state._string_32, 4);

	// buffer based bitvectors handled seperately
	if (PyPointlessBitvector_Check(py_object)) {
		PyPointlessBitvector* b = (PyPointlessBitvector*)py_object;

		if (!b->is_pointless) {
			if (b->allow_print) {
				int i = _pypointless_bitvector_str_buffer(b->primitive_bits, b->primitive_n_bits, &state);

				PyObject* s = 0;

				if (i)
					s = PyPointless_string_from_buffer_32(&state._string_32);

				pointless_dynarray_destroy(&state._string_32);

				return s;
			} else {
				pointless_dynarray_destroy(&state._string_32);
				return PyUnicode_FromFormat("<%s object at %p>", Py_TYPE(py_object)->tp_name, (void*)py_object);
			}
		}
	}

	// the pointless handle and pointless value
	PyPointless* pp = 0;
	pointless_value_t* v = 0;
	uint32_t vector_slice_i = 0;
	uint32_t vector_slice_n = 0;

	if (PyPointlessBitvector_Check(py_object)) {
		PyPointlessBitvector* b = (PyPointlessBitvector*)py_object;
		pp = b->pointless_pp;
		v = b->pointless_v;
	} else if (PyPointlessVector_Check(py_object)) {
		PyPointlessVector* vector = (PyPointlessVector*)py_object;
		pp = vector->pp;
		v = vector->v;
		vector_slice_i = vector->slice_i;
		vector_slice_n = vector->slice_n;
	} else if (PyPointlessSet_Check(py_object)) {
		PyPointlessSet* s = (PyPointlessSet*)py_object;
		pp = s->pp;
		v = s->v;
	} else if (PyPointlessMap_Check(py_object)) {
		PyPointlessMap* m = (PyPointlessMap*)py_object;
		pp = m->pp;
		v = m->v;
	}

	if (pp == 0) {
		PyErr_SetString(PyExc_ValueError, "internal error in PyPointless_str/repr()");
		return 0;
	}

	if (!pp->allow_print) {
		return PyUnicode_FromFormat("<%s object at %p>", Py_TYPE(py_object)->tp_name, (void*)py_object);
	}

	pointless_complete_value_t _v = pointless_value_to_complete(v);
	int i = _pypointless_str_rec(&pp->p, &_v, &state, vector_slice_i, vector_slice_n, 0);
	PyObject* s = 0;

	if (i)
		s = PyPointless_string_from_buffer_32(&state._string_32);

	pointless_dynarray_destroy(&state._string_32);

	return s;
}

// just hack this for now
PyObject* PyPointless_repr(PyObject* py_object)
{
	return PyPointless_str(py_object);
}
