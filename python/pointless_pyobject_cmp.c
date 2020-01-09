#include "../pointless_ext.h"

// a container for either a pointless value, or PyObject*
typedef struct {
	int32_t is_pointless;

	union {
		struct {
			pointless_t* p;
			pointless_complete_value_t v; // we do not have a ptr, because we want inline values for arbitrary vector items
			uint32_t vector_slice_i;
			uint32_t vector_slice_n;
		} pointless;

		PyObject* py_object;
	} value;
} pypointless_cmp_value_t;

// cmp state
typedef struct {
	pointless_t* pointless;
	const char* error;
	uint32_t depth;
} pypointless_cmp_state_t;

// number values
typedef struct {
	int32_t is_signed;
	int32_t is_unsigned;
	int32_t is_float;
	uint64_t uu;
	int64_t ii;
	float ff;
} pypointless_cmp_int_float_bool_t;

static const char* _type_name(uint32_t type)
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
			return "pointless.PyPointlessVector";
		case POINTLESS_UNICODE_:
			return "unicode";
		case POINTLESS_STRING_:
			return "str";
		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_PACKED:
			return "pointless.PyPointlessBitvector";
		case POINTLESS_SET_VALUE:
			return "pointless.PyPointlessSet";
		case POINTLESS_MAP_VALUE_VALUE:
			return "pointless.PyPointlessMap";
		case POINTLESS_I32:
		case POINTLESS_U32:
		case POINTLESS_FLOAT:
			return "";
		case POINTLESS_BOOLEAN:
			return "bool";
		case POINTLESS_NULL:
			return "NoneType";
		case POINTLESS_I64:
		case POINTLESS_U64:
			return "";
	}

	return "";
}

static const char* my_type_name(pypointless_cmp_value_t *v_)
{
	if (v_->is_pointless)
		return _type_name(v_->value.pointless.v.type);
	else
		return v_->value.py_object->ob_type->tp_name;
}

static void pypointless_cmp_value_init_pointless(pypointless_cmp_value_t* cv, pointless_t* p, pointless_complete_value_t* v)
{
	cv->is_pointless = 1;
	cv->value.pointless.p = p;
	cv->value.pointless.v = *v;
	cv->value.pointless.vector_slice_i = 0;
	cv->value.pointless.vector_slice_n = 0;

	if (pointless_is_vector_type(v->type)) {
		pointless_value_t _v = pointless_value_from_complete(v);
		cv->value.pointless.vector_slice_i = 0;
		cv->value.pointless.vector_slice_n = pointless_reader_vector_n_items(p, &_v);
	}
}

static void pypointless_cmp_value_init_python(pypointless_cmp_value_t* v, PyObject* py_object)
{
	v->value.pointless.vector_slice_i = 0;
	v->value.pointless.vector_slice_n = 0;

	if (PyPointlessVector_Check(py_object)) {
		v->is_pointless = 1;
		v->value.pointless.p = &(((PyPointlessVector*)py_object)->pp->p);
		v->value.pointless.v = pointless_value_to_complete((((PyPointlessVector*)py_object)->v));
		v->value.pointless.vector_slice_i = ((PyPointlessVector*)py_object)->slice_i;
		v->value.pointless.vector_slice_n = ((PyPointlessVector*)py_object)->slice_n;
	} else if (PyPointlessBitvector_Check(py_object) && ((PyPointlessBitvector*)py_object)->is_pointless) {
		v->is_pointless = 1;
		v->value.pointless.p = &(((PyPointlessBitvector*)py_object)->pointless_pp->p);
		v->value.pointless.v = pointless_value_to_complete((((PyPointlessBitvector*)py_object)->pointless_v));
	} else if (PyPointlessSet_Check(py_object)) {
		v->is_pointless = 1;
		v->value.pointless.p = &(((PyPointlessSet*)py_object)->pp->p);
		v->value.pointless.v = pointless_value_to_complete((((PyPointlessSet*)py_object)->v));
	} else if (PyPointlessMap_Check(py_object)) {
		v->is_pointless = 1;
		v->value.pointless.p = &(((PyPointlessMap*)py_object)->pp->p);
		v->value.pointless.v = pointless_value_to_complete((((PyPointlessMap*)py_object)->v));
	} else {
		v->is_pointless = 0;
		v->value.py_object = py_object;
	}
}

static int32_t pypointless_is_pylong_negative(PyObject* py_object, pypointless_cmp_state_t* state)
{
	PyObject* i = PyLong_FromLong(0);

	int32_t retval = 0;

	if (i == 0) {
		PyErr_Clear();
		state->error = "out of memory";
		return retval;
	}

	int c = PyObject_RichCompareBool(py_object, i, Py_LT);

	if (c == -1) {
		PyErr_Clear();
		state->error = "integer rich-compare error";
		return retval;
	}

	if (c == 1)
		retval = 1;

	Py_DECREF(i);

	return retval;
}

// most basic cmp function
static int32_t pypointless_cmp_rec(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state);

// forward declerations of all type-specific comparison functions
typedef int32_t (*pypointless_cmp_cb)(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state);

static int32_t pypointless_cmp_string_unicode(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state);
static int32_t pypointless_cmp_int_float_bool(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state);
static int32_t pypointless_cmp_none(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state);
static int32_t pypointless_cmp_vector(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state);
static int32_t pypointless_cmp_bitvector(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state);

// mapping of pointless value/PyObject* to a type specific comparison function
static pypointless_cmp_cb pypointless_cmp_func(pypointless_cmp_value_t* v, uint32_t* type, pypointless_cmp_state_t* state)
{
	if (v->is_pointless) {
		*type = v->value.pointless.v.type;

		switch (*type) {
			case POINTLESS_I32:
			case POINTLESS_U32:
			case POINTLESS_FLOAT:
			case POINTLESS_BOOLEAN:
				return pypointless_cmp_int_float_bool;
			case POINTLESS_NULL:
				return pypointless_cmp_none;
			case POINTLESS_STRING_:
			case POINTLESS_UNICODE_:
				return pypointless_cmp_string_unicode;
			case POINTLESS_SET_VALUE:
			case POINTLESS_MAP_VALUE_VALUE:
			case POINTLESS_EMPTY_SLOT:
				return 0;
		}

		if (pointless_is_vector_type(*type)) {
			return pypointless_cmp_vector;
		}

		if (pointless_is_bitvector_type(*type)) {
			return pypointless_cmp_bitvector;
		}

		// some illegal type, anyways, we cannot compare it
		state->error = "comparison not supported for pointless type";
		return 0;
	} else {
		// we need to check for every useful Python type
		PyObject* py_object = v->value.py_object;

		if (PyLong_Check(py_object)) {
			if (pypointless_is_pylong_negative(py_object, state))
				*type = POINTLESS_I32;
			else
				*type = POINTLESS_U32;

			return pypointless_cmp_int_float_bool;
		}

		if (PyFloat_Check(py_object)) {
			*type = POINTLESS_FLOAT;
			return pypointless_cmp_int_float_bool;
		}

		if (PyBool_Check(py_object)) {
			*type = POINTLESS_BOOLEAN;
			return pypointless_cmp_int_float_bool;
		}

		if (py_object == Py_None) {
			*type = POINTLESS_NULL;
			return pypointless_cmp_none;
		}

		if (PyUnicode_Check(py_object)) {
			*type = POINTLESS_UNICODE_;
			return pypointless_cmp_string_unicode;
		}

		if (PySet_Check(py_object) || PyFrozenSet_Check(py_object)) {
			*type = POINTLESS_SET_VALUE;
			return 0;
		}

		if (PyDict_Check(py_object)) {
			*type = POINTLESS_MAP_VALUE_VALUE;
			return 0;
		}

		if (PyList_Check(py_object)) {
			*type = POINTLESS_VECTOR_VALUE;
			return pypointless_cmp_vector;
		}

		if (PyTuple_Check(py_object)) {
			*type = POINTLESS_VECTOR_VALUE;
			return pypointless_cmp_vector;
		}

		if (PyPointlessBitvector_Check(py_object)) {
			*type = POINTLESS_BITVECTOR;
			return pypointless_cmp_bitvector;
		}

		// no known type
		state->error = "comparison not supported for Python type";
		*type = UINT32_MAX;
		return 0;
	}
}

typedef struct {
	union {
		uint32_t* string_32;
		uint16_t* string_16;
		uint8_t* string_8;
	} string;
	uint8_t n_bits;
} _var_string_t;

static _var_string_t pypointless_cmp_extract_string(pypointless_cmp_value_t* v, pypointless_cmp_state_t* state)
{
	_var_string_t s;

	if (v->is_pointless) {
		pointless_value_t v_ = pointless_value_from_complete(&v->value.pointless.v);

		if (v_.type == POINTLESS_UNICODE_) {
			s.n_bits = 32;
			s.string.string_32 = pointless_reader_unicode_value_ucs4(v->value.pointless.p, &v_);
		} else {
			s.n_bits = 8;
			s.string.string_8 = pointless_reader_string_value_ascii(v->value.pointless.p, &v_);
		}
	} else {
		assert(PyUnicode_Check(v->value.py_object));

		{

#ifdef Py_UNICODE_WIDE
			s.n_bits = 32;
			s.string.string_32 = (uint32_t*)PyUnicode_AS_UNICODE(v->value.py_object);
#else
			s.n_bits = 16;
			s.string.string_16 = (uint16_t*)PyUnicode_AS_UNICODE(v->value.py_object);
#endif
		}
	}

	return s;
}

static int32_t pypointless_cmp_string_unicode(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state)
{
	_var_string_t s_a = pypointless_cmp_extract_string(a, state);

	if (state->error)
		return 0;

	_var_string_t s_b = pypointless_cmp_extract_string(b, state);

	if (state->error)
		return 0;

	assert(s_a.n_bits == 8 || s_a.n_bits == 16 || s_a.n_bits == 32);
	assert(s_b.n_bits == 8 || s_b.n_bits == 16 || s_b.n_bits == 32);

	if (s_a.n_bits == 8 && s_b.n_bits == 8)
		return pointless_cmp_string_8_8(s_a.string.string_8, s_b.string.string_8);
	if (s_a.n_bits == 8 && s_b.n_bits == 16)
		return pointless_cmp_string_8_16(s_a.string.string_8, s_b.string.string_16);
	if (s_a.n_bits == 8 && s_b.n_bits == 32)
		return pointless_cmp_string_8_32(s_a.string.string_8, s_b.string.string_32);
	if (s_a.n_bits == 16 && s_b.n_bits == 8)
		return pointless_cmp_string_16_8(s_a.string.string_16, s_b.string.string_8);
	if (s_a.n_bits == 16 && s_b.n_bits == 16)
		return pointless_cmp_string_16_16(s_a.string.string_16, s_b.string.string_16);
	if (s_a.n_bits == 16 && s_b.n_bits == 32)
		return pointless_cmp_string_16_32(s_a.string.string_16, s_b.string.string_32);
	if (s_a.n_bits == 32 && s_b.n_bits == 8)
		return pointless_cmp_string_32_8(s_a.string.string_32, s_b.string.string_8);
	if (s_a.n_bits == 32 && s_b.n_bits == 16)
		return pointless_cmp_string_32_16(s_a.string.string_32, s_b.string.string_16);
	if (s_a.n_bits == 32 && s_b.n_bits == 32)
		return pointless_cmp_string_32_32(s_a.string.string_32, s_b.string.string_32);

	assert(0);
	return 0;
}

static pypointless_cmp_int_float_bool_t pypointless_cmp_int_float_bool_from_value(pypointless_cmp_value_t* v, pypointless_cmp_state_t* state)
{
	pypointless_cmp_int_float_bool_t r;
	r.is_signed = 0;
	r.is_unsigned = 0;
	r.is_float = 0;
	r.ii = 0;
	r.ff = 0.0f;

	if (v->is_pointless) {
		pointless_complete_value_t* pv = &v->value.pointless.v;

		switch (pv->type) {
			case POINTLESS_I32:
			case POINTLESS_I64:
				r.is_signed = 1;
				r.ii = (int64_t)pointless_complete_value_get_as_i64(pv->type, &pv->complete_data);
				return r;
			case POINTLESS_U32:
			case POINTLESS_U64:
			case POINTLESS_BOOLEAN:
				r.is_unsigned = 1;
				r.uu = (uint64_t)pointless_complete_value_get_as_u64(pv->type, &pv->complete_data);
				return r;
			case POINTLESS_FLOAT:
				r.is_float = 1;
				r.ff = pointless_complete_value_get_float(pv->type, &pv->complete_data);
				return r;
		}
	} else {
		PyObject* py_object = v->value.py_object;

		if (PyLong_Check(py_object)) {
			PY_LONG_LONG v = PyLong_AsLongLong(py_object);

			if (!PyErr_Occurred() && (INT64_MIN <= v && v <= INT64_MAX)) {
				r.is_signed = 1;
				r.ii = (int64_t)v;
				return r;
			}

			PyErr_Clear();

			unsigned PY_LONG_LONG vv = PyLong_AsUnsignedLongLong(py_object);

			if (PyErr_Occurred() || vv > UINT64_MAX) {
				PyErr_Clear();
				state->error = "python long too big for comparison";
				return r;
			}

			r.is_unsigned = 1;
			r.uu = (uint64_t)vv;
			return r;
		} else if (PyFloat_Check(py_object)) {
			r.is_float = 1;
			r.ff = (float)PyFloat_AS_DOUBLE(py_object);
			return r;
		} else if (PyBool_Check(py_object)) {
			r.is_unsigned = 0;

			if (py_object == Py_True)
				r.uu = 1;
			else
				r.uu = 0;

			return r;
		}
	}

	state->error = "int/float/bool comparison internal error";
	return r;
}

static int32_t pypointless_cmp_int_float_bool_priv(pypointless_cmp_int_float_bool_t* v_a, pypointless_cmp_int_float_bool_t* v_b)
{
	pointless_complete_value_t _v_a, _v_b;

	if (v_a->is_signed)
		_v_a = pointless_complete_value_create_as_read_i64(v_a->ii);
	else if (v_a->is_unsigned)
		_v_a = pointless_complete_value_create_as_read_u64(v_a->uu);
	else
		_v_a = pointless_complete_value_create_as_read_float(v_a->ff);

	if (v_b->is_signed)
		_v_b = pointless_complete_value_create_as_read_i64(v_b->ii);
	else if (v_b->is_unsigned)
		_v_b = pointless_complete_value_create_as_read_u64(v_b->uu);
	else
		_v_b = pointless_complete_value_create_as_read_float(v_b->ff);

	return pointless_cmp_reader_acyclic(0, &_v_a, 0, &_v_b);
}

static int32_t pypointless_cmp_int_float_bool(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state)
{
	pypointless_cmp_int_float_bool_t v_a = pypointless_cmp_int_float_bool_from_value(a, state);

	if (state->error)
		return 0;

	pypointless_cmp_int_float_bool_t v_b = pypointless_cmp_int_float_bool_from_value(b, state);

	if (state->error)
		return 0;

	return pypointless_cmp_int_float_bool_priv(&v_a, &v_b);
}

static int32_t pypointless_cmp_none(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state)
{
	// always equal
	return 0;
}

static uint32_t pypointless_cmp_vector_n_items(pypointless_cmp_value_t* a)
{
	if (a->is_pointless) {
		return a->value.pointless.vector_slice_n;
	}

	assert(PyList_Check(a->value.py_object) || PyTuple_Check(a->value.py_object));

	if (PyList_Check(a->value.py_object)) {
		return (uint32_t)PyList_GET_SIZE(a->value.py_object);
	}

	return (uint32_t)PyTuple_GET_SIZE(a->value.py_object);
}

static pypointless_cmp_value_t pypointless_cmp_vector_item_at(pypointless_cmp_value_t* v, uint32_t i)
{
	// our return value
	pypointless_cmp_value_t r;

	// pointless
	if (v->is_pointless) {
		pointless_value_t _v = pointless_value_from_complete(&v->value.pointless.v);
		r.is_pointless = 1;
		r.value.pointless.p = v->value.pointless.p;
		r.value.pointless.v = pointless_reader_vector_value_case(v->value.pointless.p, &_v, i + v->value.pointless.vector_slice_i);
		r.value.pointless.vector_slice_i = 0;
		r.value.pointless.vector_slice_n = 0;

		if (pointless_is_vector_type(r.value.pointless.v.type)) {
			pointless_value_t _v = pointless_value_from_complete(&r.value.pointless.v);
			r.value.pointless.vector_slice_i = 0;
			r.value.pointless.vector_slice_n = pointless_reader_vector_n_items(v->value.pointless.p, &_v);
		}
	// PyObject
	} else {
		r.is_pointless = 0;

		assert(PyList_Check(v->value.py_object) || PyTuple_Check(v->value.py_object));

		if (PyList_Check(v->value.py_object))
			r.value.py_object = PyList_GET_ITEM(v->value.py_object, i);
		else
			r.value.py_object = PyTuple_GET_ITEM(v->value.py_object, i);
	}

	return r;
}

static int32_t pypointless_cmp_vector(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state)
{
	uint32_t n_items_a = pypointless_cmp_vector_n_items(a);
	uint32_t n_items_b = pypointless_cmp_vector_n_items(b);
	uint32_t i, n_items = (n_items_a < n_items_b) ? n_items_a : n_items_b;
	int32_t c;

	pypointless_cmp_value_t v_a, v_b;

	for (i = 0; i < n_items; i++) {
		v_a = pypointless_cmp_vector_item_at(a, i);
		v_b = pypointless_cmp_vector_item_at(b, i);
		c = pypointless_cmp_rec(&v_a, &v_b, state);

		if (c != 0)
			return c;
	}

	return SIMPLE_CMP(n_items_a, n_items_b);
}

static uint32_t pypointless_cmp_bitvector_n_items(pypointless_cmp_value_t* v)
{
	if (v->is_pointless) {
		pointless_value_t _v = pointless_value_from_complete(&v->value.pointless.v);
		return pointless_reader_bitvector_n_bits(v->value.pointless.p, &_v);
	}

	assert(PyPointlessBitvector_Check(v->value.py_object));

	PyPointlessBitvector* bv = (PyPointlessBitvector*)v->value.py_object;

	if (bv->is_pointless)
		return pointless_reader_bitvector_n_bits(&bv->pointless_pp->p, bv->pointless_v);

	return bv->primitive_n_bits;
}

static uint32_t pypointless_cmp_bitvector_item_at(pypointless_cmp_value_t* v, uint32_t i)
{
	if (v->is_pointless) {
		pointless_value_t _v = pointless_value_from_complete(&v->value.pointless.v);
		return pointless_reader_bitvector_is_set(v->value.pointless.p, &_v, i);
	}

	assert(PyPointlessBitvector_Check(v->value.py_object));

	PyPointlessBitvector* bv = (PyPointlessBitvector*)v->value.py_object;

	if (bv->is_pointless)
		return pointless_reader_bitvector_is_set(&bv->pointless_pp->p, bv->pointless_v, i);

	return (bm_is_set_(bv->primitive_bits, i) != 0);
}

static int32_t pypointless_cmp_bitvector(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state)
{
	uint32_t n_items_a = pypointless_cmp_bitvector_n_items(a);
	uint32_t n_items_b = pypointless_cmp_bitvector_n_items(b);
	uint32_t i, n_items = (n_items_a < n_items_b) ? n_items_a : n_items_b;
	uint32_t v_a, v_b;
	int32_t c;

	for (i = 0; i < n_items; i++) {
		v_a = pypointless_cmp_bitvector_item_at(a, i);
		v_b = pypointless_cmp_bitvector_item_at(b, i);

		c = SIMPLE_CMP(v_a, v_b);

		if (c != 0)
			return c;
	}

	return SIMPLE_CMP(n_items_a, n_items_b);
}

static int32_t pypointless_cmp_rec(pypointless_cmp_value_t* a, pypointless_cmp_value_t* b, pypointless_cmp_state_t* state)
{
	// check for too much depth
	if (state->depth >= POINTLESS_MAX_DEPTH) {
		state->error = "maximum recursion depth reached during comparison";
		return 0;
	}

	// get the two comparison functions needed
	uint32_t t_a, t_b;
	pypointless_cmp_cb cmp_a = pypointless_cmp_func(a, &t_a, state);
	pypointless_cmp_cb cmp_b = pypointless_cmp_func(b, &t_b, state);
	int32_t c;

	// we're going one deep
	state->depth += 1;

	if (cmp_a == 0 || cmp_b == 0 || cmp_a != cmp_b) {
		state->error = "comparison not supported between these types";
		state->depth -= 1;
		return 0;
	} else {
		c = (*cmp_a)(a, b, state);
	}

	// ...and up again
	state->depth -= 1;

	// return cmp result
	return c;
}

uint32_t pypointless_cmp_eq(pointless_t* p, pointless_complete_value_t* v, PyObject* py_object, const char** error)
{
	pypointless_cmp_value_t v_a, v_b;
	int32_t c;
	pypointless_cmp_state_t state;

	pypointless_cmp_value_init_pointless(&v_a, p, v);
	pypointless_cmp_value_init_python(&v_b, py_object);

	state.error = 0;
	state.depth = 0;

	c = pypointless_cmp_rec(&v_a, &v_b, &state);

	if (state.error != 0) {
		*error = state.error;
		return 0;
	}

	return (c == 0);
}

const char pointless_cmp_doc[] =
"2\n"
"pointless.pointless_cmp(a, b)\n"
"\n"
"Return a pointless-consistent comparison of two objects.\n"
"\n"
"  a: first object\n"
"  b: second object\n"
;
PyObject* pointless_cmp(PyObject* self, PyObject* args)
{
	PyObject* a = 0;
	PyObject* b = 0;
	pypointless_cmp_value_t v_a, v_b;
	int32_t c;
	pypointless_cmp_state_t state;

	if (!PyArg_ParseTuple(args, "OO:pointless_cmp", &a, &b))
		return 0;

	pypointless_cmp_value_init_python(&v_a, a);
	pypointless_cmp_value_init_python(&v_b, b);

	state.error = 0;
	state.depth = 0;

	c = pypointless_cmp_rec(&v_a, &v_b, &state);

	if (state.error) {
		PyErr_Format(PyExc_TypeError, "pointless_cmp: %s", state.error);
		return 0;
	}

	return PyLong_FromLong((long)c);
}

const char pointless_is_eq_doc[] =
"3\n"
"pointless.pointless_is_eq(a, b)\n"
"\n"
"Return true iff: a == b, as defined by pointless.\n"
"\n"
"  a: first object\n"
"  b: second object\n"
;
PyObject* pointless_is_eq(PyObject* self, PyObject* args)
{
	PyObject* a = 0;
	PyObject* b = 0;
	pypointless_cmp_value_t v_a, v_b;
	int32_t c;
	pypointless_cmp_state_t state;

	if (!PyArg_ParseTuple(args, "OO:pointless_is_eq", &a, &b))
		return 0;

	pypointless_cmp_value_init_python(&v_a, a);
	pypointless_cmp_value_init_python(&v_b, b);

	state.error = 0;
	state.depth = 0;

	c = pypointless_cmp_rec(&v_a, &v_b, &state);

	if (state.error) {
		PyErr_Format(PyExc_ValueError, "pointless_cmp: %s", state.error);
		return 0;
	}

	if (c == 0)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
