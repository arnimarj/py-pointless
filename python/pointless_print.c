#include "../pointless_ext.h"

// yes I know, this is a global variable, but this is a similar trick to avoid cycles
// as CPython does, since it guarantees that access to these functions is serialized
typedef struct {
	uint32_t depth;
	uint32_t container_ids[POINTLESS_MAX_DEPTH];
} pypointless_print_state_t;

static int32_t print_state_has_container(pypointless_print_state_t* state, uint32_t container_id)
{
	uint32_t i;

	for (i = 0; i < state->depth; i++) {
		if (state->container_ids[i] == container_id)
			return 1;
	}

	return 0;
}

static int print_state_push(pypointless_print_state_t* state, uint32_t container_id)
{
	if (state->depth >= POINTLESS_MAX_DEPTH) {
		PyErr_SetString(PyExc_ValueError, "maximum recursion depth reached during print");
		return 0;
	}

	state->container_ids[state->depth++] = container_id;
	return 1;
}

static void print_state_pop(pypointless_print_state_t* state)
{
	state->depth -= 1;
}

static PyObject* pypointless_unicode_str(pointless_t* p, pointless_value_t* v);
static PyObject* pypointless_vector_str(pointless_t* p, pointless_value_t* v, pypointless_print_state_t* state, uint32_t slice_i, uint32_t slice_n);
static PyObject* pypointless_set_str(pointless_t* p, pointless_value_t* v, pypointless_print_state_t* state);
static PyObject* pypointless_map_str(pointless_t* p, pointless_value_t* v, pypointless_print_state_t* state);
static PyObject* pypointless_bitvector_str(pointless_t* p, pointless_value_t* v);
static PyObject* pypointless_bitvector_str_buffer(void* buffer, uint32_t n_bits);

static PyObject* pypointless_str_rec(pointless_t* p, pointless_value_t* v, pypointless_print_state_t* state, uint32_t vector_slice_i, uint32_t vector_slice_n)
{
	// both compressible types
	if (pointless_is_vector_type(v->type))
		return pypointless_vector_str(p, v, state, vector_slice_i, vector_slice_n);

	if (pointless_is_bitvector_type(v->type))
		return pypointless_bitvector_str(p, v);

	// enough to hold any number
	char buffer[1024];

	switch (v->type) {
		case POINTLESS_UNICODE:
			return pypointless_unicode_str(p, v);
		case POINTLESS_SET_VALUE:
			return pypointless_set_str(p, v, state);
		case POINTLESS_MAP_VALUE_VALUE:
			return pypointless_map_str(p, v, state);
		case POINTLESS_I32:
			snprintf(buffer, sizeof(buffer), "%li", (long int)pointless_value_get_i32(v->type, &v->data));
			return PyString_FromString(buffer);
		case POINTLESS_U32:
			snprintf(buffer, sizeof(buffer), "%lu", (long unsigned)pointless_value_get_u32(v->type, &v->data));
			return PyString_FromString(buffer);
		case POINTLESS_FLOAT:
			snprintf(buffer, sizeof(buffer), "%f", pointless_value_get_float(v->type, &v->data));
			return PyString_FromString(buffer);
		case POINTLESS_BOOLEAN:
			if (pointless_value_get_bool(v->type, &v->data))
				return PyString_FromString("True");
			else
				return PyString_FromString("False");
		case POINTLESS_NULL:
			return PyString_FromString("None");
	}

	PyErr_SetString(PyExc_ValueError, "internal print error in pypointless_str_rec()");
	return 0;
}

static PyObject* pypointless_unicode_str(pointless_t* p, pointless_value_t* v)
{
	PyObject* s = pypointless_value_unicode(p, v);

	if (s == 0)
		return 0;

	PyObject* str = PyObject_Repr(s);
	Py_DECREF(s);
	return str;
}

static PyObject* pypointless_vector_str(pointless_t* p, pointless_value_t* v, pypointless_print_state_t* state, uint32_t slice_i, uint32_t slice_n)
{
	uint32_t container_id = pointless_container_id(p, v);

	if (print_state_has_container(state, container_id))
		return PyString_FromString("[...]");

	PyObject* vector_s = PyString_FromString("[");
	PyObject* vector_comma = PyString_FromString(", ");
	PyObject* vector_postfix = PyString_FromString("]");

	PyObject* v_s = 0;

	if (vector_s == 0 || vector_comma == 0 || vector_postfix == 0 || !print_state_push(state, container_id))
		goto error;

	uint32_t i;

	for (i = 0; i < slice_n; i++) {
		pointless_value_t value = pointless_reader_vector_value_case(p, v, i + slice_i);
		uint32_t v_slice_i = 0;
		uint32_t v_slice_n = 0;

		if (pointless_is_vector_type(value.type)) {
			v_slice_i = 0;
			v_slice_n = pointless_reader_vector_n_items(p, &value);
		}

		v_s = pypointless_str_rec(p, &value, state, v_slice_i, v_slice_n);

		if (v_s == 0) {
			print_state_pop(state);
			goto error;
		}

		PyString_ConcatAndDel(&vector_s, v_s);
		v_s = 0;

		if (vector_s == 0) {
			print_state_pop(state);
			goto error;
		}

		if (i + 1 < slice_n) {
			PyString_Concat(&vector_s, vector_comma);

			if (vector_s == 0) {
				print_state_pop(state);
				goto error;
			}
		}
	}

	PyString_Concat(&vector_s, vector_postfix);

	if (vector_s == 0) {
		print_state_pop(state);
		goto error;
	}

	print_state_pop(state);

	Py_DECREF(vector_comma);
	Py_DECREF(vector_postfix);

	Py_XDECREF(v_s);

	return vector_s;

error:
	Py_XDECREF(vector_s);
	Py_XDECREF(vector_comma);
	Py_XDECREF(vector_postfix);

	Py_XDECREF(v_s);

	return 0;
}

static PyObject* pypointless_set_str(pointless_t* p, pointless_value_t* v, pypointless_print_state_t* state)
{
	uint32_t container_id = pointless_container_id(p, v);

	if (print_state_has_container(state, container_id))
		return PyString_FromString("set([...])");

	PyObject* set_s = PyString_FromString("set([");
	PyObject* set_comma = PyString_FromString(", ");
	PyObject* set_postfix = PyString_FromString("])");

	PyObject* s_v = 0;

	pointless_value_t* value = 0;

	if (set_s == 0 || set_comma == 0 || set_postfix == 0 || !print_state_push(state, container_id))
		goto error;

	uint32_t i = 0, iter_state = 0, n_items = pointless_reader_set_n_items(p, v);

	while (pointless_reader_set_iter(p, v, &value, &iter_state)) {
		uint32_t v_slice_i = 0;
		uint32_t v_slice_n = 0;

		if (pointless_is_vector_type(value->type)) {
			v_slice_i = 0;
			v_slice_n = pointless_reader_vector_n_items(p, value);
		}

		s_v = pypointless_str_rec(p, value, state, v_slice_i, v_slice_n);

		if (s_v == 0) {
			print_state_pop(state);
			goto error;
		}

		PyString_ConcatAndDel(&set_s, s_v);
		s_v = 0;

		if (set_s == 0) {
			print_state_pop(state);
			goto error;
		}

		if (i + 1 < n_items) {
			PyString_Concat(&set_s, set_comma);

			if (set_s == 0) {
				print_state_pop(state);
				goto error;
			}
		}

		i += 1;
	}

	PyString_Concat(&set_s, set_postfix);

	if (set_s == 0) {
		print_state_pop(state);
		goto error;
	}

	print_state_pop(state);

	Py_DECREF(set_comma);
	Py_DECREF(set_postfix);

	Py_XDECREF(s_v);

	return set_s;

error:
	Py_XDECREF(set_s);
	Py_XDECREF(set_comma);
	Py_XDECREF(set_postfix);

	Py_XDECREF(s_v);

	return 0;
}

static PyObject* pypointless_map_str(pointless_t* p, pointless_value_t* v, pypointless_print_state_t* state)
{
	uint32_t container_id = pointless_container_id(p, v);

	if (print_state_has_container(state, container_id))
		return PyString_FromString("{...}");

	PyObject* dict_s = PyString_FromString("{");
	PyObject* dict_colon = PyString_FromString(": ");
	PyObject* dict_comma = PyString_FromString(", ");
	PyObject* dict_postfix = PyString_FromString("}");

	PyObject* s_k = 0;
	PyObject* s_v = 0;

	pointless_value_t* key = 0;
	pointless_value_t* value = 0;

	if (dict_s == 0 || dict_colon == 0 || dict_comma == 0 || dict_postfix == 0 || !print_state_push(state, container_id))
		goto error;

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

		s_k = pypointless_str_rec(p, key, state, v_slice_i_k, v_slice_n_k);
		s_v = pypointless_str_rec(p, value, state, v_slice_i_v, v_slice_n_v);

		if (s_k == 0 || s_v == 0) {
			print_state_pop(state);
			goto error;
		}

		PyString_ConcatAndDel(&dict_s, s_k);
		s_k = 0;

		if (dict_s == 0) {
			print_state_pop(state);
			goto error;
		}

		PyString_Concat(&dict_s, dict_colon);

		if (dict_s == 0) {
			print_state_pop(state);
			goto error;
		}

		PyString_ConcatAndDel(&dict_s, s_v);
		s_v = 0;

		if (dict_s == 0) {
			print_state_pop(state);
			goto error;
		}

		if (i + 1 < n_items) {
			PyString_Concat(&dict_s, dict_comma);

			if (dict_s == 0) {
				print_state_pop(state);
				goto error;
			}
		}

		i += 1;
	}

	PyString_Concat(&dict_s, dict_postfix);

	if (dict_s == 0) {
		print_state_pop(state);
		goto error;
	}

	print_state_pop(state);

	Py_DECREF(dict_colon);
	Py_DECREF(dict_comma);
	Py_DECREF(dict_postfix);

	Py_XDECREF(s_k);
	Py_XDECREF(s_v);

	return dict_s;

error:
	Py_XDECREF(dict_s);
	Py_XDECREF(dict_colon);
	Py_XDECREF(dict_comma);
	Py_XDECREF(dict_postfix);

	Py_XDECREF(s_k);
	Py_XDECREF(s_v);

	return 0;
}

static PyObject* pypointless_bitvector_str(pointless_t* p, pointless_value_t* v)
{
	PyObject* zero = PyString_FromString("0");
	PyObject* one = PyString_FromString("1");
	PyObject* r = PyString_FromString("");
	PyObject* b = PyString_FromString("b");

	if (zero == 0 || one == 0 || r == 0 || b == 0)
		goto error;

	int32_t n_bits = (int32_t)pointless_reader_bitvector_n_bits(p, v);
	int32_t i;

	for (i = 0; i < n_bits; i++) {
		if (pointless_reader_bitvector_is_set(p, v, n_bits - i - 1))
			PyString_Concat(&r, one);
		else
			PyString_Concat(&r, zero);

		if (r == 0)
			goto error;
	}

	PyString_Concat(&r, b);

	if (r == 0)
		goto error;

	Py_DECREF(zero);
	Py_DECREF(one);
	Py_DECREF(b);

	return r;

error:

	Py_XDECREF(zero);
	Py_XDECREF(one);
	Py_XDECREF(r);
	Py_XDECREF(b);

	return 0;
}

static PyObject* pypointless_bitvector_str_buffer(void* buffer, uint32_t n_bits)
{
	PyObject* zero = PyString_FromString("0");
	PyObject* one = PyString_FromString("1");
	PyObject* r = PyString_FromString("");
	PyObject* b = PyString_FromString("b");


	if (zero == 0 || one == 0 || r == 0 || b == 0)
		goto error;

	uint32_t i;

	for (i = 0; i < n_bits; i++) {
		if (bm_is_set_(buffer, n_bits - i - 1))
			 PyString_Concat(&r, one);
		else
			PyString_Concat(&r, zero);

		if (r == 0)
			goto error;
	}

	PyString_Concat(&r, b);

	if (r == 0)
		goto error;

	Py_DECREF(zero);
	Py_DECREF(one);
	Py_DECREF(b);

	return r;

error:

	Py_XDECREF(zero);
	Py_XDECREF(one);
	Py_XDECREF(r);
	Py_XDECREF(b);

	return 0;
}

PyObject* PyPointless_str(PyObject* py_object)
{
	// setup a state
	pypointless_print_state_t state;
	state.depth = 0;

	// buffer based bitvectors handled seperately
	if (PyPointlessBitvector_Check(py_object)) {
		PyPointlessBitvector* b = (PyPointlessBitvector*)py_object;

		if (!b->is_pointless)
			return pypointless_bitvector_str_buffer(b->primitive_bits, b->primitive_n_bits);
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

	if (!pp->allow_print)
		return PyString_FromFormat("<%s object at %p>", Py_TYPE(py_object)->tp_name, (void*)py_object);

	return pypointless_str_rec(&pp->p, v, &state, vector_slice_i, vector_slice_n); 
}

// just hack this for now
PyObject* PyPointless_repr(PyObject* py_object)
{
	return PyPointless_str(py_object);
}
