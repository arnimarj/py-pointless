#include "../pointless_ext.h"

static PyObject* pointless_int_to_python(int64_t v)
{
	if (LONG_MIN <= v && v <= LONG_MAX) {
		return PyLong_FromLong((long)v);
	} else if (LLONG_MIN <= v && v <= LLONG_MAX)
		return PyLong_FromLongLong((PY_LONG_LONG)v);
	else
		return PyErr_Format(PyExc_ValueError, "this particular integer value not supported by python");
}

static PyObject* pointless_uint_to_python(uint64_t v)
{
	if (v <= LONG_MAX) {
		return PyLong_FromLong((long)v);
	} else if (v <= LLONG_MAX)
		return PyLong_FromLongLong((PY_LONG_LONG)v);
	else if (v <= ULLONG_MAX)
		return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)v);
	else
		return PyErr_Format(PyExc_ValueError, "this particular integer value not supported by python");
}

PyObject* pypointless_i32(PyPointless* p, int32_t i32)
{
	return pointless_int_to_python((int64_t)i32);
}

PyObject* pypointless_u32(PyPointless* p, uint32_t u32)
{
	return pointless_uint_to_python((uint64_t)u32);
}

PyObject* pypointless_i64(PyPointless* p, int64_t i64)
{
	return pointless_int_to_python(i64);
}

PyObject* pypointless_u64(PyPointless* p, uint64_t u64)
{
	return pointless_uint_to_python(u64);
}

PyObject* pypointless_float(PyPointless* p, float f)
{
	return PyFloat_FromDouble((double)f);
}

PyObject* pypointless_value_unicode(pointless_t* p, pointless_value_t* v)
{
	Py_ssize_t unicode_len = (Py_ssize_t)pointless_reader_unicode_len(p, v);
	uint32_t* unicode_ucs4 = pointless_reader_unicode_value_ucs4(p, v);
	return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, (uint32_t*)unicode_ucs4, unicode_len);
}

PyObject* pypointless_value_string(pointless_t* p, pointless_value_t* v)
{
	uint8_t* string_ascii = pointless_reader_string_value_ascii(p, v);
	return PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND , (const char*)string_ascii, strlen((const char*)string_ascii));
}

PyObject* pypointless_value(PyPointless* p, pointless_value_t* v)
{
	// create the actual value
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
		case POINTLESS_VECTOR_EMPTY:
			return (PyObject*)PyPointlessVector_New(p, v, 0, pointless_reader_vector_n_items(&p->p, v));

		case POINTLESS_STRING_:
			return pypointless_value_string(&p->p, v);

		case POINTLESS_UNICODE_:
			return pypointless_value_unicode(&p->p, v);

		case POINTLESS_BITVECTOR:
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
		case POINTLESS_BITVECTOR_10:
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_PACKED:
			return (PyObject*)PyPointlessBitvector_New(p, v);

		case POINTLESS_I32:
			return pypointless_i32(p, pointless_value_get_i32(v->type, &v->data));
		case POINTLESS_U32:
			return pypointless_u32(p, pointless_value_get_u32(v->type, &v->data));
		case POINTLESS_FLOAT:
			return pypointless_float(p, pointless_value_get_float(v->type, &v->data));

		case POINTLESS_BOOLEAN:
			if (pointless_value_get_bool(v->type, &v->data))
				Py_RETURN_TRUE;
			else
				Py_RETURN_FALSE;

		case POINTLESS_NULL:
			Py_INCREF(Py_None);
			return Py_None;

		case POINTLESS_SET_VALUE:
			return (PyObject*)PyPointlessSet_New(p, v);

		case POINTLESS_MAP_VALUE_VALUE:
			return (PyObject*)PyPointlessMap_New(p, v);
	}

	PyErr_Format(PyExc_ValueError, "internal error, got strange type ID %u, this file should not have passed validation", (unsigned int)v->type);
	return 0;
}
