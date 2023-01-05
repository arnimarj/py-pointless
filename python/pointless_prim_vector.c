#include "../pointless_ext.h"

static PyObject* PyPointlessPrimVector_append_item(PyPointlessPrimVector* self, PyObject* item);
PyPointlessPrimVector* PyPointlessPrimVector_from_T_vector(pointless_dynarray_t* v, uint32_t t);

typedef union {
	int8_t i8;
	uint8_t u8;
	int16_t i16;
	uint16_t u16;
	int32_t i32;
	uint32_t u32;
	int64_t i64;
	uint64_t u64;
	float f;
} pypointless_number_t;

static int parse_pyobject_number(PyObject* v, int* is_signed, int64_t* i, uint64_t* u)
{
	// complicated case
	if (!PyLong_Check(v)) {
		PyErr_SetString(PyExc_TypeError, "expected an integer");
		return 0;
	}

	PY_LONG_LONG ii = 0;
	unsigned PY_LONG_LONG uu = 0;

	ii = PyLong_AsLongLong(v);

	// negative int
	if (!(ii == -1 && PyErr_Occurred()) && ii < 0) {
		*is_signed = 1;
		*i = (int64_t)ii;
		return 1;
	}

	PyErr_Clear();

	uu = PyLong_AsUnsignedLongLong(v);

	if ((uu == (unsigned long long)-1) && PyErr_Occurred()) {
		PyErr_SetString(PyExc_ValueError, "integer too big");
		return 0;
	}

	*is_signed = 0;
	*u = (uint64_t)uu;

	return 1;
}

static int pypointless_parse_number(PyObject* n, pypointless_number_t* v, uint32_t t)
{
	// float is simple
	if (t == POINTLESS_PRIM_VECTOR_TYPE_FLOAT) {
		if (PyFloat_Check(n) && PyArg_Parse(n, "f", &v->f))
			return 1;

		PyErr_SetString(PyExc_TypeError, "expected a number");
		return 0;
	}

	// we try a ULONGLONG first, LONGLONG second
	int is_signed = 0;
	int64_t _ii = 0;
	uint64_t _uu = 0;

	if (!parse_pyobject_number(n, &is_signed, &_ii, &_uu))
		return 0;

	// we assume that _ii is always negative
	if (is_signed && _ii >= 0) {
		PyErr_SetString(PyExc_TypeError, "internal error A");
		return 0;
	}

	int in_range = 0;

	// positive value, need only test upper limit
	if (!is_signed) {
		switch (t) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				in_range = (_uu <= INT8_MAX);
				v->i8 = (int8_t)_uu;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:
				in_range = (_uu <= UINT8_MAX);
				v->u8 = (uint8_t)_uu;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				in_range = (_uu <= INT16_MAX);
				v->i16 = (int16_t)_uu;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:
				in_range = (_uu <= UINT16_MAX);
				v->u16 = (uint16_t)_uu;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				in_range = (_uu <= INT32_MAX);
				v->i32 = (int32_t)_uu;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:
				in_range = (_uu <= UINT32_MAX);
				v->u32 = (uint32_t)_uu;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I64:
				in_range = (_uu <= INT64_MAX);
				v->i64 = (int64_t)_uu;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U64:
				in_range = (_uu <= UINT64_MAX);
				v->u64 = (uint64_t)_uu;
				break;
			default:
				PyErr_SetString(PyExc_TypeError, "internal error A");
				return 0;
		}
	// negative value, only test lower limit
	} else {
		switch (t) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				in_range = (INT8_MIN <= _ii);
				v->i8 = (int8_t)_ii;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				in_range = (INT16_MIN <= _ii);
				v->i16 = (int16_t)_ii;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				in_range = (INT32_MIN <= _ii);
				v->i32 = (int32_t)_ii;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I64:
				in_range = (INT64_MIN <= _ii);
				v->i64 = (int64_t)_ii;
				break;
			// since value is < 0, it is by definition out of range
			case POINTLESS_PRIM_VECTOR_TYPE_U8:
			case POINTLESS_PRIM_VECTOR_TYPE_U16:
			case POINTLESS_PRIM_VECTOR_TYPE_U32:
			case POINTLESS_PRIM_VECTOR_TYPE_U64:
				in_range = 0;
				break;
			default:
				PyErr_SetString(PyExc_TypeError, "internal error A");
				return 0;
		}
	}

	if (!in_range) {
		PyErr_SetString(PyExc_ValueError, "given value is smaller/greater than the allowed values");
		return 0;
	}

	return 1;
}

static void PyPointlessPrimVector_dealloc(PyPointlessPrimVector* self)
{
	if (self->ob_exports > 0) {
		PyErr_SetString(PyExc_SystemError, "deallocated PointlessPrimVector object has exported buffers");
		PyErr_Print();
	}

	pointless_dynarray_destroy(&self->array);
	Py_TYPE(self)->tp_free(self);
}

static void PyPointlessPrimVectorIter_dealloc(PyPointlessPrimVectorIter* self)
{
	Py_XDECREF(self->vector);
	Py_TYPE(self)->tp_free(self);
}

static void PyPointlessPrimVectorRevIter_dealloc(PyPointlessPrimVectorRevIter* self)
{
	Py_XDECREF(self->vector);
	Py_TYPE(self)->tp_free(self);
}

PyObject* PyPointlessPrimVector_str(PyPointlessPrimVector* self)
{
	if (!self->allow_print) {
		return PyUnicode_FromFormat("<%s object at %p>", Py_TYPE(self)->tp_name, (void*)self);
	}

	pointless_dynarray_t string;
	pointless_dynarray_init(&string, 1);

	uint8_t bracket_left = '[';
	uint8_t comma = ',';
	uint8_t space = ' ';
	uint8_t bracket_right = ']';
	uint8_t terminating_zero = 0;

	uint32_t i, n = pointless_dynarray_n_items(&self->array);
	char buffer[1024];

	if (!pointless_dynarray_push(&string, &bracket_left)) {
		PyErr_NoMemory();
		goto error;
	}

	for (i = 0; i < n; i++) {
		void* data = pointless_dynarray_item_at(&self->array, i);

		switch (self->type) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				snprintf(buffer, sizeof(buffer), "%i", (int)(*((int8_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:
				snprintf(buffer, sizeof(buffer), "%u", (unsigned int)(*((uint8_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				snprintf(buffer, sizeof(buffer), "%i", (int)(*((int16_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:
				snprintf(buffer, sizeof(buffer), "%u", (unsigned int)(*((uint16_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				snprintf(buffer, sizeof(buffer), "%i", (int)(*((int32_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:
				snprintf(buffer, sizeof(buffer), "%u", (unsigned int)(*((uint32_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I64:
				snprintf(buffer, sizeof(buffer), "%lld", (long long)(*((int64_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U64:
				snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)(*((uint64_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
				snprintf(buffer, sizeof(buffer), "%f", (*((float*)data)));
				break;
		}

		if (!pointless_dynarray_push_bulk(&string, buffer, strlen(buffer))) {
			PyErr_NoMemory();
			goto error;
		}

		if (i + 1 < n) {
			if (!pointless_dynarray_push(&string, &comma)) {
				PyErr_NoMemory();
				goto error;
			}

			if (!pointless_dynarray_push(&string, &space)) {
				PyErr_NoMemory();
				goto error;
			}
		}
	}

	if (!pointless_dynarray_push(&string, &bracket_right)) {
		PyErr_NoMemory();
		goto error;
	}

	if (!pointless_dynarray_push(&string, &terminating_zero)) {
		PyErr_NoMemory();
		goto error;
	}

	PyObject* v_s = PyUnicode_FromString(pointless_dynarray_buffer(&string));

	pointless_dynarray_destroy(&string);

	return v_s;

error:
	pointless_dynarray_destroy(&string);

	return 0;
}

PyObject* PyPointlessPrimVector_repr(PyPointlessPrimVector* self)
{
	return PyPointlessPrimVector_str(self);
}

#define POINTLESS_PRIM_VECTOR_N_TYPES 9

struct {
	const char* s;
	uint32_t type;
	uint32_t typesize;
} pointless_prim_vector_type_map[POINTLESS_PRIM_VECTOR_N_TYPES] = {
	{"i8",  POINTLESS_PRIM_VECTOR_TYPE_I8, sizeof(int8_t)},
	{"u8",  POINTLESS_PRIM_VECTOR_TYPE_U8, sizeof(uint8_t)},
	{"i16", POINTLESS_PRIM_VECTOR_TYPE_I16, sizeof(int16_t)},
	{"u16", POINTLESS_PRIM_VECTOR_TYPE_U16, sizeof(uint16_t)},
	{"i32", POINTLESS_PRIM_VECTOR_TYPE_I32, sizeof(int32_t)},
	{"u32", POINTLESS_PRIM_VECTOR_TYPE_U32, sizeof(uint32_t)},
	{"i64", POINTLESS_PRIM_VECTOR_TYPE_I64, sizeof(int64_t)},
	{"u64", POINTLESS_PRIM_VECTOR_TYPE_U64, sizeof(uint64_t)},
	{"f",   POINTLESS_PRIM_VECTOR_TYPE_FLOAT, sizeof(float)}
};

static int PyPointlessPrimVector_can_resize(PyPointlessPrimVector* self)
{
	if (self->ob_exports > 0) {
		PyErr_SetString(PyExc_BufferError, "existing exports of data: object cannot be re-sized");
		return 0;
	}

	return 1;
}

static int PyPointlessPrimVector_init(PyPointlessPrimVector* self, PyObject* args, PyObject* kwds)
{
	// if we have a buffer attached, we shouldn't be here
	if (!PyPointlessPrimVector_can_resize(self))
		return -1;

	// printing enabled by default
	self->allow_print = 1;
	self->ob_exports = 0;

	// clear previous contents
	pointless_dynarray_clear(&self->array);
	self->type = 0;

	// parse input
	const char* type = 0;
	Py_buffer buffer;
	PyObject* sequence_obj = 0;
	PyObject* allow_print = 0;
	uint32_t i;
	int retval = -1;

	buffer.buf = 0;
	buffer.len = 0;
	buffer.obj = 0;

	static char* kwargs[] = {"type", "buffer", "sequence", "allow_print", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ss*OO!", kwargs, &type, &buffer, &sequence_obj, &PyBool_Type, &allow_print))
		return -1;

	if ((type != 0) == (buffer.obj != 0)) {
		PyErr_SetString(PyExc_TypeError, "exactly one of type/buffer must be specified");
		goto cleanup;
	}

	if (type == 0 && sequence_obj != 0) {
		PyErr_SetString(PyExc_ValueError, "sequence only allowed if type is specified");
		goto cleanup;
	}

	if (allow_print == Py_False)
		self->allow_print = 0;

	// typecode and (sequence)
	if (type != 0) {
		for (i = 0; i < POINTLESS_PRIM_VECTOR_N_TYPES; i++) {
			if (strcmp(type, pointless_prim_vector_type_map[i].s) == 0) {
				pointless_dynarray_init(&self->array, pointless_prim_vector_type_map[i].typesize);
				self->type = pointless_prim_vector_type_map[i].type;
				break;
			}
		}

		if (i == POINTLESS_PRIM_VECTOR_N_TYPES) {
			PyErr_SetString(PyExc_TypeError, "unknown primitive vector type");
			goto cleanup;
		}

		// if we have an iterator, construct the vector from it
		if (sequence_obj) {
			PyObject* iterator = PyObject_GetIter(sequence_obj);

			if (iterator == 0)
				goto cleanup;

			PyObject* item = 0;

			while ((item = PyIter_Next(iterator)) != 0) {
				if (!PyPointlessPrimVector_append_item(self, item))
					break;

				Py_DECREF(item);
				item = 0;
			}

			Py_DECREF(iterator);
			iterator = 0;

			if (PyErr_Occurred()) {
				pointless_dynarray_destroy(&self->array);
				goto cleanup;
			}
		}

		retval = 0;
	// else a single buffer
	} else {
		// de-serialize the buffer
		if (buffer.len < (int)(sizeof(uint32_t) + sizeof(uint32_t))) {
			PyErr_SetString(PyExc_ValueError, "buffer too short");
			goto cleanup;
		}

		self->type = ((uint32_t*)buffer.buf)[0];
		uint32_t buffer_n_items = ((uint32_t*)buffer.buf)[1];
		uint64_t expected_buffer_size = 0;

		for (i = 0; i < POINTLESS_PRIM_VECTOR_N_TYPES; i++) {
			if (pointless_prim_vector_type_map[i].type == self->type) {
				expected_buffer_size += (uint64_t)pointless_prim_vector_type_map[i].typesize * (uint64_t)buffer_n_items;
				pointless_dynarray_init(&self->array, pointless_prim_vector_type_map[i].typesize);
				break;
			}
		}

		if (i == POINTLESS_PRIM_VECTOR_N_TYPES) {
			PyErr_SetString(PyExc_ValueError, "illegal buffer vector type");
			goto cleanup;
		}

		expected_buffer_size += sizeof(uint32_t) + sizeof(uint32_t);

		if ((uint64_t)buffer.len != expected_buffer_size) {
			PyErr_SetString(PyExc_ValueError, "illegal buffer length");
			goto cleanup;
		}

		for (i = 0; i < buffer_n_items; i++) {
			void* data_buffer = (uint32_t*)buffer.buf + 2;
			int added = 0;

			switch (self->type) {
				case POINTLESS_PRIM_VECTOR_TYPE_I8:
					added = pointless_dynarray_push(&self->array, (int8_t*)data_buffer + i);
					break;
				case POINTLESS_PRIM_VECTOR_TYPE_U8:
					added = pointless_dynarray_push(&self->array, (uint8_t*)data_buffer + i);
					break;
				case POINTLESS_PRIM_VECTOR_TYPE_I16:
					added = pointless_dynarray_push(&self->array, (int16_t*)data_buffer + i);
					break;
				case POINTLESS_PRIM_VECTOR_TYPE_U16:
					added = pointless_dynarray_push(&self->array, (uint16_t*)data_buffer + i);
					break;
				case POINTLESS_PRIM_VECTOR_TYPE_I32:
					added = pointless_dynarray_push(&self->array, (int32_t*)data_buffer + i);
					break;
				case POINTLESS_PRIM_VECTOR_TYPE_U32:
					added = pointless_dynarray_push(&self->array, (uint32_t*)data_buffer + i);
					break;
				case POINTLESS_PRIM_VECTOR_TYPE_I64:
					added = pointless_dynarray_push(&self->array, (int64_t*)data_buffer + i);
					break;
				case POINTLESS_PRIM_VECTOR_TYPE_U64:
					added = pointless_dynarray_push(&self->array, (uint64_t*)data_buffer + i);
					break;
				case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
					added = pointless_dynarray_push(&self->array, (float*)data_buffer + i);
					break;
				default:
					PyErr_SetString(PyExc_Exception, "internal error");
					goto cleanup;
			}

			if (!added) {
				PyErr_NoMemory();
				goto cleanup;
			}
		}

		retval = 0;
	}

cleanup:

	if (retval == -1)
		pointless_dynarray_clear(&self->array);

	if (buffer.obj)
		PyBuffer_Release(&buffer);

	return retval;
}

static Py_ssize_t PyPointlessPrimVector_length(PyPointlessPrimVector* self)
{
	return (Py_ssize_t)pointless_dynarray_n_items(&self->array);
}

static int PyPointlessPrimVector_check_index(PyPointlessPrimVector* self, PyObject* item, Py_ssize_t* i)
{
	// if this is not an index: throw an exception
	if (!PyIndex_Check(item)) {
		PyErr_Format(PyExc_TypeError, "PrimVector: integer indexes please, got <%s>\n", item->ob_type->tp_name);
		return 0;
	}

	// if index value is not an integer: throw an exception
	*i = PyNumber_AsSsize_t(item, PyExc_IndexError);

	if (*i == -1 && PyErr_Occurred())
		return 0;

	// if index is negative, it is relative to vector end
	if (*i < 0)
		*i += PyPointlessPrimVector_length(self);

	// if it is out of bounds: throw an exception
	if (!(0 <= *i && *i < PyPointlessPrimVector_length(self))) {
		PyErr_SetString(PyExc_IndexError, "index is out of bounds");
		return 0;
	}

	return 1;
}

static PyObject* PyPointlessPrimVector_subscript_priv(PyPointlessPrimVector* self, uint32_t i)
{
	void* base_value = pointless_dynarray_item_at(&self->array, i);

	switch (self->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			return PyLong_FromLong((long)(*((int8_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			return PyLong_FromLong((long)(*((uint8_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			return PyLong_FromLong((long)(*((int16_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			return PyLong_FromLong((long)(*((uint16_t*)base_value)));

		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			return PyLong_FromLong((long)(*((int32_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			return PyLong_FromUnsignedLong((unsigned long)(*((uint32_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_I64:
			return PyLong_FromLongLong((PY_LONG_LONG)(*((int64_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_U64:
			return PyLong_FromUnsignedLong((unsigned PY_LONG_LONG)(*((uint64_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			return PyFloat_FromDouble((double)(*((float*)base_value)));
	}

	PyErr_SetString(PyExc_ValueError, "illegal value type");
	return 0;
}

static PyObject* PyPointlessPrimVector_slice(PyPointlessPrimVector* self, Py_ssize_t ilow, Py_ssize_t ihigh);

static PyObject* PyPointlessPrimVector_subscript(PyPointlessPrimVector* self, PyObject* item)
{
	// get the index
	Py_ssize_t i;

	if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength;

		if (PySlice_GetIndicesEx(item, (Py_ssize_t)pointless_dynarray_n_items(&self->array), &start, &stop, &step, &slicelength) == -1)
			return 0;

		if (step != 1) {
			PyErr_SetString(PyExc_ValueError, "only slice-steps of 1 supported");
			return 0;
		}

		return PyPointlessPrimVector_slice(self, start, stop);
	}

	if (!PyPointlessPrimVector_check_index(self, item, &i))
		return 0;

	return PyPointlessPrimVector_subscript_priv(self, (uint32_t)i);
}

static PyObject* PyPointlessPrimVector_item(PyPointlessPrimVector* self, Py_ssize_t i)
{
	// if index is negative, it is relative to vector end
	if (i < 0)
		i += PyPointlessPrimVector_length(self);

	// if it is out of bounds: throw an exception
	if (!(0 <= i && i < PyPointlessPrimVector_length(self))) {
		PyErr_SetString(PyExc_IndexError, "index is out of bounds");
		return 0;
	}

	return PyPointlessPrimVector_subscript_priv(self, (uint32_t)i);
}


static int PyPointlessPrimVector_ass_item(PyPointlessPrimVector* self, Py_ssize_t i, PyObject* v)
{
	if (!(0 <= i && i < PyPointlessPrimVector_length(self))) {
		PyErr_SetString(PyExc_IndexError, "vector index out of range");
		return -1;
	}

	pypointless_number_t number;

	if (!pypointless_parse_number(v, &number, self->type))
		return -1;

	memcpy(pointless_dynarray_item_at(&self->array, i), &number, self->array.item_size);
	return 0;
}

static int PyPointlessPrimVector_ass_subscript(PyPointlessPrimVector* self, PyObject* item, PyObject* value)
{
	if (PyIndex_Check(item)) {
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

		if (i == -1 && PyErr_Occurred())
			return -1;

		if (i < 0)
			i += PyList_GET_SIZE(self);

		return PyPointlessPrimVector_ass_item(self, i, value);
	} else {
		PyErr_Format(PyExc_TypeError, "indices must be integers %.200s", item->ob_type->tp_name);
		return -1;
	}
}


static PyObject* PyPointlessPrimVector_iter(PyObject* vector)
{
	if (!PyPointlessPrimVector_Check(vector)) {
		PyErr_BadInternalCall();
		return 0;
	}

	PyPointlessPrimVectorIter* iter = PyObject_New(PyPointlessPrimVectorIter, &PyPointlessPrimVectorIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(vector);
	iter->vector = (PyPointlessPrimVector*)vector;
	iter->iter_state = 0;

	return (PyObject*)iter;
}


static PyObject* PyPointlessPrimVector_rev_iter(PyObject* vector)
{
	if (!PyPointlessPrimVector_Check(vector)) {
		PyErr_BadInternalCall();
		return 0;
	}

	PyPointlessPrimVectorRevIter* iter = PyObject_New(PyPointlessPrimVectorRevIter, &PyPointlessPrimVectorRevIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(vector);
	iter->vector = (PyPointlessPrimVector*)vector;
	iter->iter_state = 0;

	return (PyObject*)iter;
}

static PyObject* PyPointlessPrimVectorIter_iternext(PyPointlessPrimVectorIter* iter)
{
	// iterator already reached end
	if (iter->vector == 0)
		return 0;

	// see if we have any items left
	uint32_t n_items = (uint32_t)PyPointlessPrimVector_length(iter->vector);

	if (iter->iter_state < n_items) {
		PyObject* item = PyPointlessPrimVector_subscript_priv(iter->vector, iter->iter_state);
		iter->iter_state += 1;
		return item;
	}

	// we reached end
	Py_DECREF(iter->vector);
	iter->vector = 0;
	return 0;
}


static PyObject* PyPointlessPrimVectorRevIter_iternext(PyPointlessPrimVectorRevIter* iter)
{
	// iterator already reached end
	if (iter->vector == 0)
		return 0;

	// see if we have any items left
	uint32_t n_items = (uint32_t)PyPointlessPrimVector_length(iter->vector);

	if (iter->iter_state < n_items) {
		PyObject* item = PyPointlessPrimVector_subscript_priv(iter->vector, n_items - iter->iter_state - 1);
		iter->iter_state += 1;
		return item;
	}

	// we reached end
	Py_DECREF(iter->vector);
	iter->vector = 0;
	return 0;
}

static PyObject* PyPointlessPrimVector_append_item(PyPointlessPrimVector* self, PyObject* item)
{
	if (!PyPointlessPrimVector_can_resize(self))
		return 0;

	pypointless_number_t number;

	if (!pypointless_parse_number(item, &number, self->type)) {
		return 0;
	}

	if (!pointless_dynarray_push(&self->array, &number)){
		return PyErr_NoMemory();
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyPointlessPrimVector_append(PyPointlessPrimVector* self, PyObject* args)
{
	PyObject* obj = 0;

	if (!PyArg_ParseTuple(args, "O", &obj)) {
		PyErr_SetString(PyExc_TypeError, "expected a float/integer");
		return 0;
	}

	if (!PyPointlessPrimVector_can_resize(self)) {
		return 0;
	}

	return PyPointlessPrimVector_append_item(self, obj);
}

static PyObject* PyPointlessPrimVector_append_bulk(PyPointlessPrimVector* self, PyObject* args)
{
	PyObject* obj = 0;
	PyObject* iterator = 0;
	PyObject* item = 0;
	size_t n_append = 0;
	size_t i = 0;
	size_t j = 0;

	if (!PyArg_ParseTuple(args, "O", &obj))
		return 0;

	if (!PyPointlessPrimVector_can_resize(self))
		return 0;

	// fast version 1
	if (PyPointlessPrimVector_Check(obj)) {
		PyPointlessPrimVector* p_obj = (PyPointlessPrimVector*)obj;

		if (p_obj->type == self->type) {
			for (i = 0; i < pointless_dynarray_n_items(&p_obj->array); i++) {
				void* v = pointless_dynarray_item_at(&p_obj->array, i);
				if (!pointless_dynarray_push(&self->array, v)) {
					for (j = 0; j < n_append; j++)
						pointless_dynarray_pop(&self->array);

					PyErr_NoMemory();
					return 0;
				}
				n_append += 1;
			}

			Py_INCREF(Py_None);
			return Py_None;
		}
	}

	// fast version 2
	if (PyPointlessVector_Check(obj)) {
		PyPointlessVector* p_obj = (PyPointlessVector*)obj;
		size_t s = 0;

		if      (p_obj->v->type == POINTLESS_VECTOR_I8 && self->type == POINTLESS_PRIM_VECTOR_TYPE_I8)
			s = sizeof(int8_t);
		else if (p_obj->v->type == POINTLESS_VECTOR_U8 && self->type == POINTLESS_PRIM_VECTOR_TYPE_U8)
			s = sizeof(uint8_t);
		else if (p_obj->v->type == POINTLESS_VECTOR_I16 && self->type == POINTLESS_PRIM_VECTOR_TYPE_I16)
			s = sizeof(int16_t);
		else if (p_obj->v->type == POINTLESS_VECTOR_U16 && self->type == POINTLESS_PRIM_VECTOR_TYPE_U16)
			s = sizeof(uint16_t);
		else if (p_obj->v->type == POINTLESS_VECTOR_I32 && self->type == POINTLESS_PRIM_VECTOR_TYPE_I32)
			s = sizeof(int32_t);
		else if (p_obj->v->type == POINTLESS_VECTOR_U32 && self->type == POINTLESS_PRIM_VECTOR_TYPE_U32)
			s = sizeof(uint32_t);
		else if (p_obj->v->type == POINTLESS_VECTOR_I64 && self->type == POINTLESS_PRIM_VECTOR_TYPE_I64)
			s = sizeof(int64_t);
		else if (p_obj->v->type == POINTLESS_VECTOR_U64 && self->type == POINTLESS_PRIM_VECTOR_TYPE_U64)
			s = sizeof(uint64_t);
		else if (p_obj->v->type == POINTLESS_VECTOR_FLOAT && self->type == POINTLESS_PRIM_VECTOR_TYPE_FLOAT)
			s = sizeof(float);

		if (s > 0) {
			void* base = 0;

			switch (p_obj->v->type) {
				case POINTLESS_VECTOR_I8:
					base = pointless_reader_vector_i8(&p_obj->pp->p, p_obj->v);
					break;
				case POINTLESS_VECTOR_U8:
					base = pointless_reader_vector_u8(&p_obj->pp->p, p_obj->v);
					break;
				case POINTLESS_VECTOR_I16:
					base = pointless_reader_vector_i16(&p_obj->pp->p, p_obj->v);
					break;
				case POINTLESS_VECTOR_U16:
					base = pointless_reader_vector_u16(&p_obj->pp->p, p_obj->v);
					break;
				case POINTLESS_VECTOR_I32:
					base = pointless_reader_vector_i32(&p_obj->pp->p, p_obj->v);
					break;
				case POINTLESS_VECTOR_U32:
					base = pointless_reader_vector_u32(&p_obj->pp->p, p_obj->v);
					break;
				case POINTLESS_VECTOR_I64:
					base = pointless_reader_vector_i64(&p_obj->pp->p, p_obj->v);
					break;
				case POINTLESS_VECTOR_U64:
					base = pointless_reader_vector_u64(&p_obj->pp->p, p_obj->v);
					break;
				case POINTLESS_VECTOR_FLOAT:
					base = pointless_reader_vector_float(&p_obj->pp->p, p_obj->v);
					break;
			}

			for (i = 0; i < p_obj->slice_n; i++) {
				void* v = (char*)base + (i + p_obj->slice_i) * s;

				if (!pointless_dynarray_push(&self->array, v)) {
					for (j = 0; j < n_append; j++)
						pointless_dynarray_pop(&self->array);

					PyErr_NoMemory();
					return 0;
				}
				n_append += 1;
			}

			Py_INCREF(Py_None);
			return Py_None;
		}
	}

	iterator = PyObject_GetIter(obj);

	if (iterator == 0)
		return 0;

	while ((item = PyIter_Next(iterator)) != 0) {
		if (!PyPointlessPrimVector_append_item(self, item))
			break;

		n_append += 1;
		Py_DECREF(item);
		item = 0;
	}

	Py_DECREF(iterator);
	iterator = 0;

	if (PyErr_Occurred()) {
		for (i = 0; i < n_append; i++)
			pointless_dynarray_pop(&self->array);

		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyPointlessPrimVector_pop(PyPointlessPrimVector* self)
{
	size_t n_items = pointless_dynarray_n_items(&self->array);

	if (n_items == 0) {
		PyErr_SetString(PyExc_IndexError, "pop from empty vector");
		return 0;
	}

	if (!PyPointlessPrimVector_can_resize(self))
		return 0;

	PyObject* v = PyPointlessPrimVector_subscript_priv(self, n_items - 1);

	if (v == 0)
		return 0;

	pointless_dynarray_pop(&self->array);
	return v;
}

static PyObject* PyPointlessPrimVector_pop_bulk(PyPointlessPrimVector* self, PyObject* args)
{
	PY_LONG_LONG n = 0, i = 0;

	if (!PyArg_ParseTuple(args, "L", &n))
		return 0;

	if (n > 0 && (size_t)n > pointless_dynarray_n_items(&self->array)) {
		PyErr_SetString(PyExc_ValueError, "vector is not big enough");
		return 0;
	}

	for (i = 0; i < n; i++)
		pointless_dynarray_pop(&self->array);

	Py_INCREF(Py_None);
	return Py_None;
}

static size_t PyPointlessPrimVector_index_f(PyPointlessPrimVector* self, float v)
{
	uint32_t i, n = pointless_dynarray_n_items(&self->array);
	float* a = (float*)(self->array._data);

	for (i = 0; i < n; i++) {
		if (a[i] == v) {
			return i;
		}
	}

	return SIZE_MAX;
}

static size_t PyPointlessPrimVector_index_i_i(PyPointlessPrimVector* self, int64_t v)
{
	uint32_t i, n = pointless_dynarray_n_items(&self->array);
	void* a = self->array._data;

	for (i = 0; i < n; i++) {
		switch (self->type) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				if (((int8_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				if (((int16_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				if (((int32_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I64:
				if (((int64_t*)a)[i] == v)
					return i;
				break;
		}
	}

	return SIZE_MAX;
}

static size_t PyPointlessPrimVector_index_i_u(PyPointlessPrimVector* self, uint64_t v)
{
	uint32_t i, n = pointless_dynarray_n_items(&self->array);
	void* a = (float*)(self->array._data);

	for (i = 0; i < n; i++) {
		switch (self->type) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				if (((int8_t*)a)[i] >= 0 && (uint64_t)((int8_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:
				if (((uint8_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				if (((int16_t*)a)[i] >= 0 && (uint64_t)((int16_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:
				if (((uint16_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				if (((int32_t*)a)[i] >= 0 && (uint64_t)((int32_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:
				if (((uint32_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I64:
				if (((int64_t*)a)[i] >= 0 && (uint64_t)((int64_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U64:
				if (((uint64_t*)a)[i] == v)
					return i;
				break;
		}
	}

	return SIZE_MAX;
}

static size_t PyPointlessPrimVector_index_(PyPointlessPrimVector* self, PyObject* args, const char* func)
{
	size_t i = SIZE_MAX;

	if (self->type == POINTLESS_PRIM_VECTOR_TYPE_FLOAT) {
		float ff;
	
		if (!PyArg_ParseTuple(args, "f", &ff))
			return (SIZE_MAX-1);

		i = PyPointlessPrimVector_index_f(self, ff);
	} else {
		int is_signed = 0;
		int64_t _ii = 0;
		uint64_t _uu = 0;

		if (PyTuple_Check(args) && PyTuple_GET_SIZE(args) == 1) {
			if (parse_pyobject_number(PyTuple_GET_ITEM(args, 0), &is_signed, &_ii, &_uu)) {
				if (is_signed)
					i = PyPointlessPrimVector_index_i_i(self, _ii);
				else
					i = PyPointlessPrimVector_index_i_u(self, _uu);
			}
		}
	}

	if (i == SIZE_MAX) {
		PyErr_Format(PyExc_ValueError, "vector.%s(x): x not in vector", func);
		return (SIZE_MAX-1);
	}

	return i;
}

static int PyPointlessPrimVector_contains(PyPointlessPrimVector* self, PyObject* b)
{
	size_t i = SIZE_MAX;
	int is_signed = 0;
	int64_t _ii = 0;
	uint64_t _uu = 0;

	if (self->type == POINTLESS_PRIM_VECTOR_TYPE_FLOAT) {

		float ff = 0.0;

		if (PyFloat_Check(b)) {
			ff = (float)PyFloat_AsDouble(b);
		} else {
			if (!parse_pyobject_number(b, &is_signed, &_ii, &_uu)) {
				PyErr_Clear();
				return 0;
			}

			if (is_signed) {
				ff = (float)_ii;
			} else {
				ff = (float)_uu;
			}
		}

		i = PyPointlessPrimVector_index_f(self, ff);
	} else {
		if (!parse_pyobject_number(b, &is_signed, &_ii, &_uu)) {
			PyErr_Clear();
			return 0;
		}

		if (is_signed)
			i = PyPointlessPrimVector_index_i_i(self, _ii);
		else
			i = PyPointlessPrimVector_index_i_u(self, _uu);
	}

	if (i == SIZE_MAX) {
		return 0;
	}

	return 1;
}

static PyObject* PyPointlessPrimVector_index(PyPointlessPrimVector* self, PyObject* args)
{
	size_t i = PyPointlessPrimVector_index_(self, args, "index");

	if (i == (SIZE_MAX-1))
		return 0;

	return PyLong_FromSize_t(i);
}

static PyObject* PyPointlessPrimVector_remove(PyPointlessPrimVector* self, PyObject* args)
{
	if (!PyPointlessPrimVector_can_resize(self))
		return 0;

	size_t i = PyPointlessPrimVector_index_(self, args, "remove");

	if (i == (SIZE_MAX-1))
		return 0;

	// shift array (needlessly expensive because of swap)
	for (; i < pointless_dynarray_n_items(&self->array) - 1; i++)
		pointless_dynarray_swap(&self->array, i, i + 1);

	// remove last
	pointless_dynarray_pop(&self->array);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyPointlessPrimVector_fast_remove(PyPointlessPrimVector* self, PyObject* args)
{
	if (!PyPointlessPrimVector_can_resize(self))
		return 0;

	size_t i = PyPointlessPrimVector_index_(self, args, "fast_remove");

	if (i == (SIZE_MAX-1))
		return 0;

	// swap with last item and remove last item
	pointless_dynarray_swap(&self->array, i, pointless_dynarray_n_items(&self->array) - 1);
	pointless_dynarray_pop(&self->array);

	Py_INCREF(Py_None);
	return Py_None;
}

static size_t PyPointlessPrimVector_type_size(PyPointlessPrimVector* self)
{
	size_t typesize = 0, i;

	for (i = 0; i < POINTLESS_PRIM_VECTOR_N_TYPES; i++) {
		if (pointless_prim_vector_type_map[i].type == self->type) {
			typesize = pointless_prim_vector_type_map[i].typesize;
			break;
		}
	}

	return typesize;
}

static size_t PyPointlessPrimVector_n_items(PyPointlessPrimVector* self)
{
	return pointless_dynarray_n_items(&self->array);
}

static size_t PyPointlessPrimVector_n_bytes(PyPointlessPrimVector* self)
{
	size_t n_items = PyPointlessPrimVector_n_items(self);
	size_t item_size = PyPointlessPrimVector_type_size(self);
	return (n_items * item_size);
}

static PyObject* PyPointlessPrimVector_serialize(PyPointlessPrimVector* self)
{
	// the format is: [uint32_t type] [uint32 length] [raw integers]
	// the length param is redundant, but gives a fair sanity check

	// create a buffer
	size_t n_bytes = PyPointlessPrimVector_n_bytes(self);
	uint32_t n_items = PyPointlessPrimVector_n_items(self);
	uint64_t n_buffer = sizeof(uint32_t) + sizeof(uint32_t) + (uint64_t)n_bytes;

	if (n_buffer > PY_SSIZE_T_MAX || n_buffer > SIZE_MAX) {
		PyErr_SetString(PyExc_Exception, "vector too large for serialization");
		return 0;
	}

	// allocate a buffer
	void* buffer = pointless_malloc((size_t)n_buffer);

	if (buffer == 0)
		return PyErr_NoMemory();

	// serialize it
	((uint32_t*)buffer)[0] = self->type;
	((uint32_t*)buffer)[1] = n_items;
	memcpy((void*)((uint32_t*)buffer + 2), pointless_dynarray_buffer(&self->array), n_bytes);

	n_bytes += sizeof(uint32_t);
	n_bytes += sizeof(uint32_t);

	PyObject* bytearray = PyByteArray_FromStringAndSize((const char*)buffer, (Py_ssize_t)n_bytes);
	pointless_free(buffer);

	return bytearray;
}

static PyObject* PyPointlessPrimVector_clear(PyPointlessPrimVector* self)
{
	if (!PyPointlessPrimVector_can_resize(self))
		return 0;

	pointless_dynarray_clear(&self->array);

	Py_INCREF(Py_None);
	return Py_None;
}


static int PointlessPrimVector_getbuffer(PyPointlessPrimVector* obj, Py_buffer* view, int flags)
{
	int ret;
	void* ptr;

	if (view == NULL) {
		obj->ob_exports++;
		return 0;
	}

	ptr = (void*)pointless_dynarray_buffer(&obj->array);
	ret = PyBuffer_FillInfo(view, (PyObject*)obj, ptr, (Py_ssize_t)PyPointlessPrimVector_n_bytes(obj), 0, flags);

	if (ret >= 0)
		obj->ob_exports++;

	return ret;
}

static void PointlessPrimVector_releasebuffer(PyPointlessPrimVector* obj, Py_buffer* view)
{
	obj->ob_exports--;
}

static PyBufferProcs PointlessPrimVector_as_buffer = {
	(getbufferproc)PointlessPrimVector_getbuffer,
	(releasebufferproc)PointlessPrimVector_releasebuffer,
};


#define SORT_SWAP(T, B, I_A, I_B) T t = ((T*)(B))[I_A]; ((T*)(B))[I_A] = ((T*)(B))[I_B]; ((T*)(B))[I_B] = t
#define PRIM_SORT_SWAP(T, P, I_A, I_B) SORT_SWAP(T, ((PyPointlessPrimVector*)P)->array._data, I_A, I_B)
#define PROJ_SORT_SWAP(T, P, I_A, I_B) SORT_SWAP(T, ((prim_sort_proj_state_t*)P)->p_b, I_A, I_B)

#define SORT_CMP(T, B, I_A, I_B) SIMPLE_CMP(((T*)(B))[I_A], ((T*)(B))[I_B])
#define SORT_CMP_RET(T, B, I_A, I_B) *c = SORT_CMP(T, B, I_A, I_B); return 1
#define PRIM_SORT_CMP(T, P, I_A, I_B) SORT_CMP_RET(T, ((PyPointlessPrimVector*)user)->array._data, I_A, I_B)

static void prim_sort_swap_i8(int a, int b, void* user)
	{ PRIM_SORT_SWAP(int8_t, user, a, b); }
static void prim_sort_swap_u8(int a, int b, void* user)
	{ PRIM_SORT_SWAP(uint8_t, user, a, b); }
static void prim_sort_swap_i16(int a, int b, void* user)
	{ PRIM_SORT_SWAP(int16_t, user, a, b); }
static void prim_sort_swap_u16(int a, int b, void* user)
	{ PRIM_SORT_SWAP(uint16_t, user, a, b); }
static void prim_sort_swap_i32(int a, int b, void* user)
	{ PRIM_SORT_SWAP(int32_t, user, a, b); }
static void prim_sort_swap_u32(int a, int b, void* user)
	{ PRIM_SORT_SWAP(uint32_t, user, a, b); }
static void prim_sort_swap_i64(int a, int b, void* user)
	{ PRIM_SORT_SWAP(int64_t, user, a, b); }
static void prim_sort_swap_u64(int a, int b, void* user)
	{ PRIM_SORT_SWAP(uint64_t, user, a, b); }
static void prim_sort_swap_f(int a, int b, void* user)
	{ PRIM_SORT_SWAP(float, user, a, b); }

static int prim_sort_cmp_i8(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(int8_t, user, a, b); }
static int prim_sort_cmp_u8(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(uint8_t, user, a, b); }
static int prim_sort_cmp_i16(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(int16_t, user, a, b); }
static int prim_sort_cmp_u16(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(uint16_t, user, a, b); }
static int prim_sort_cmp_i32(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(int32_t, user, a, b); }
static int prim_sort_cmp_u32(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(uint32_t, user, a, b); }
static int prim_sort_cmp_i64(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(int64_t, user, a, b); }
static int prim_sort_cmp_u64(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(uint64_t, user, a, b); }
static int prim_sort_cmp_f(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(float, user, a, b); }

static PyObject* PyPointlessPrimVector_sort(PyPointlessPrimVector* self)
{
	int bad = 0;

	{
		int n = (int)pointless_dynarray_n_items(&self->array);

		switch (self->type) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:    bentley_sort_(n, prim_sort_cmp_i8,  prim_sort_swap_i8,  (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:    bentley_sort_(n, prim_sort_cmp_u8,  prim_sort_swap_u8,  (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:   bentley_sort_(n, prim_sort_cmp_i16, prim_sort_swap_i16, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:   bentley_sort_(n, prim_sort_cmp_u16, prim_sort_swap_u16, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:   bentley_sort_(n, prim_sort_cmp_i32, prim_sort_swap_i32, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:   bentley_sort_(n, prim_sort_cmp_u32, prim_sort_swap_u32, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_I64:   bentley_sort_(n, prim_sort_cmp_i64, prim_sort_swap_i64, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_U64:   bentley_sort_(n, prim_sort_cmp_u64, prim_sort_swap_u64, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_FLOAT: bentley_sort_(n, prim_sort_cmp_f,   prim_sort_swap_f,   (void*)self); break;
			default:
				bad = 1;
				break;
		}
	}

	if (bad) {
		PyErr_BadInternalCall();
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

typedef void (*prim_sort_proj_cmp_index_t)(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b);
typedef int (*prim_sort_proj_cmp_cmp_t)(uint64_t a, uint64_t b, void* v_b);

typedef struct {
	// the projection
	void* p_b;     // base pointer
	uint32_t p_n;  // number of items
	uint32_t p_t;  // item type

	// sort value vectors
	uint32_t n;

	PyObject* v_p[16]; // Python objects
	void* v_b[16];     // base pointers
	uint32_t v_n[16];  // number of items
	uint32_t v_t[16];  // item types

	// comparison info
	prim_sort_proj_cmp_index_t cmp_index;
	prim_sort_proj_cmp_cmp_t cmp_cmp[16];
} prim_sort_proj_state_t;

static void prim_sort_proj_swap_i8(int a, int b, void* user)
	{ PROJ_SORT_SWAP(int8_t, user, a, b); }
static void prim_sort_proj_swap_u8(int a, int b, void* user)
	{ PROJ_SORT_SWAP(uint8_t, user, a, b); }
static void prim_sort_proj_swap_i16(int a, int b, void* user)
	{ PROJ_SORT_SWAP(int16_t, user, a, b); }
static void prim_sort_proj_swap_u16(int a, int b, void* user)
	{ PROJ_SORT_SWAP(uint16_t, user, a, b); }
static void prim_sort_proj_swap_i32(int a, int b, void* user)
	{ PROJ_SORT_SWAP(int32_t, user, a, b); }
static void prim_sort_proj_swap_u32(int a, int b, void* user)
	{ PROJ_SORT_SWAP(uint32_t, user, a, b); }
static void prim_sort_proj_swap_i64(int a, int b, void* user)
	{ PROJ_SORT_SWAP(int64_t, user, a, b); }
static void prim_sort_proj_swap_u64(int a, int b, void* user)
	{ PROJ_SORT_SWAP(uint64_t, user, a, b); }

#define prim_sort_proj_cmp_index_(T, a, b, i_a, i_b, p_b) *(i_a) = ((T*)(p_b))[a]; *(i_b) = ((T*)(p_b))[b];

static void prim_sort_proj_cmp_index_i8(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b)
	{ prim_sort_proj_cmp_index_(int8_t, a, b, i_a, i_b, p_b); }
static void prim_sort_proj_cmp_index_u8(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b)
	{ prim_sort_proj_cmp_index_(uint8_t, a, b, i_a, i_b, p_b); }
static void prim_sort_proj_cmp_index_i16(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b)
	{ prim_sort_proj_cmp_index_(int16_t, a, b, i_a, i_b, p_b); }
static void prim_sort_proj_cmp_index_u16(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b)
	{ prim_sort_proj_cmp_index_(uint16_t, a, b, i_a, i_b, p_b); }
static void prim_sort_proj_cmp_index_i32(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b)
	{ prim_sort_proj_cmp_index_(int32_t, a, b, i_a, i_b, p_b); }
static void prim_sort_proj_cmp_index_u32(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b)
	{ prim_sort_proj_cmp_index_(uint32_t, a, b, i_a, i_b, p_b); }
static void prim_sort_proj_cmp_index_i64(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b)
	{ prim_sort_proj_cmp_index_(int64_t, a, b, i_a, i_b, p_b); }
static void prim_sort_proj_cmp_index_u64(int a, int b, uint64_t* i_a, uint64_t* i_b, void* p_b)
	{ prim_sort_proj_cmp_index_(uint64_t, a, b, i_a, i_b, p_b); }

static int prim_sort_proj_cmp_cmp_i8(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(int8_t, v_b, a, b); }
static int prim_sort_proj_cmp_cmp_u8(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(uint8_t, v_b, a, b); }
static int prim_sort_proj_cmp_cmp_i16(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(int16_t, v_b, a, b); }
static int prim_sort_proj_cmp_cmp_u16(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(uint16_t, v_b, a, b); }
static int prim_sort_proj_cmp_cmp_i32(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(int32_t, v_b, a, b); }
static int prim_sort_proj_cmp_cmp_u32(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(uint32_t, v_b, a, b); }
static int prim_sort_proj_cmp_cmp_i64(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(int64_t, v_b, a, b); }
static int prim_sort_proj_cmp_cmp_u64(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(uint64_t, v_b, a, b); }
static int prim_sort_proj_cmp_cmp_f(uint64_t a, uint64_t b, void* v_b)
	{ return SORT_CMP(float, v_b, a, b); }

static int prim_sort_proj_cmp(int a, int b, int* c, void* user)
{
	prim_sort_proj_state_t* state = (prim_sort_proj_state_t*)user;

	uint64_t i_a_u, i_b_u;

	uint32_t i;

	(*state->cmp_index)(a, b, &i_a_u, &i_b_u, state->p_b);

	*c = 0;

	for (i = 0; i < state->n && *c == 0; i++)
		*c = (*(state->cmp_cmp[i]))(i_a_u, i_b_u, state->v_b[i]);

	return 1;
}

static PyObject* PyPointlessPrimVector_sort_proj(PyPointlessPrimVector* self, PyObject* args)
{
	// initialize sort state
	prim_sort_proj_state_t state;

	state.p_b = 0;
	state.p_n = 0;
	state.p_t = 0;

	for (state.n = 0; state.n < 16; state.n++) {
		state.v_p[state.n] = 0;
		state.v_b[state.n] = 0;
		state.v_n[state.n] = 0;
		state.v_t[state.n] = 0;
	}

	state.n = 0;

	// get input vectors
	if (!PyArg_ParseTuple(args, "O|OOOOOOOOOOOOOOO",
		&state.v_p[0], &state.v_p[1], &state.v_p[2], &state.v_p[3],
		&state.v_p[4], &state.v_p[5], &state.v_p[6], &state.v_p[7],
		&state.v_p[8], &state.v_p[9], &state.v_p[10], &state.v_p[11],
		&state.v_p[12], &state.v_p[13], &state.v_p[14], &state.v_p[15])) {
		goto cleanup;
	}

	// initialize projection vector state
	state.p_b = self->array._data;
	state.p_n = pointless_dynarray_n_items(&self->array);
	state.p_t = self->type;

	// it must contain integer values
	switch (self->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:  state.cmp_index = prim_sort_proj_cmp_index_i8;  break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:  state.cmp_index = prim_sort_proj_cmp_index_u8;  break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16: state.cmp_index = prim_sort_proj_cmp_index_i16; break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16: state.cmp_index = prim_sort_proj_cmp_index_u16; break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32: state.cmp_index = prim_sort_proj_cmp_index_i32; break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32: state.cmp_index = prim_sort_proj_cmp_index_u32; break;
		case POINTLESS_PRIM_VECTOR_TYPE_I64: state.cmp_index = prim_sort_proj_cmp_index_i64; break;
		case POINTLESS_PRIM_VECTOR_TYPE_U64: state.cmp_index = prim_sort_proj_cmp_index_u64; break;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			PyErr_SetString(PyExc_ValueError, "projection vector must contain only integer values");
			goto cleanup;
		default:
			PyErr_BadInternalCall();
			goto cleanup;
	}

	// count and initialize value vectors
	for (state.n = 0; state.n < 16 && state.v_p[state.n]; state.n++) {
		if (PyPointlessPrimVector_Check(state.v_p[state.n])) {
			PyPointlessPrimVector* ppv = (PyPointlessPrimVector*)state.v_p[state.n];
			state.v_b[state.n] = ppv->array._data;
			state.v_n[state.n] = pointless_dynarray_n_items(&ppv->array);
			state.v_t[state.n] = ppv->type;

			switch (ppv->type) {
				case POINTLESS_PRIM_VECTOR_TYPE_I8:    state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_i8;  break;
				case POINTLESS_PRIM_VECTOR_TYPE_U8:    state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_u8;  break;
				case POINTLESS_PRIM_VECTOR_TYPE_I16:   state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_i16; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U16:   state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_u16; break;
				case POINTLESS_PRIM_VECTOR_TYPE_I32:   state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_i32; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U32:   state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_u32; break;
				case POINTLESS_PRIM_VECTOR_TYPE_I64:   state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_i64; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U64:   state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_u64; break;
				case POINTLESS_PRIM_VECTOR_TYPE_FLOAT: state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_f;   break;
				default:
					PyErr_BadInternalCall();
					goto cleanup;
			}
		} else if (PyPointlessVector_Check(state.v_p[state.n])) {
			PyPointlessVector* pv = (PyPointlessVector*)state.v_p[state.n];

			state.v_n[state.n] = pv->slice_n;
			state.v_t[state.n] = pv->v->type;

			switch (pv->v->type) {
				// we only want primitive types, or empty vectors
				case POINTLESS_VECTOR_I8:
					state.v_b[state.n] = (void*)(pointless_reader_vector_i8(&pv->pp->p, pv->v)    + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_i8;
					break;
				case POINTLESS_VECTOR_U8:
					state.v_b[state.n] = (void*)(pointless_reader_vector_u8(&pv->pp->p, pv->v)    + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_u8;
					break;
				case POINTLESS_VECTOR_I16:
					state.v_b[state.n] = (void*)(pointless_reader_vector_i16(&pv->pp->p, pv->v)   + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_i16;
					break;
				case POINTLESS_VECTOR_U16:
					state.v_b[state.n] = (void*)(pointless_reader_vector_u16(&pv->pp->p, pv->v)   + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_u16;
					break;
				case POINTLESS_VECTOR_I32:
					state.v_b[state.n] = (void*)(pointless_reader_vector_i32(&pv->pp->p, pv->v)   + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_i32;
					break;
				case POINTLESS_VECTOR_U32:
					state.v_b[state.n] = (void*)(pointless_reader_vector_u32(&pv->pp->p, pv->v)   + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_u32;
					break;
				case POINTLESS_VECTOR_I64:
					state.v_b[state.n] = (void*)(pointless_reader_vector_i64(&pv->pp->p, pv->v)   + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_i64;
					break;
				case POINTLESS_VECTOR_U64:
					state.v_b[state.n] = (void*)(pointless_reader_vector_u64(&pv->pp->p, pv->v)   + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_u64;
					break;
				case POINTLESS_VECTOR_FLOAT:
					state.v_b[state.n] = (void*)(pointless_reader_vector_float(&pv->pp->p, pv->v) + pv->slice_i);
					state.cmp_cmp[state.n] = prim_sort_proj_cmp_cmp_f;
					break;
				case POINTLESS_VECTOR_EMPTY:
					state.v_b[state.n] = 0;
					break;
				case POINTLESS_VECTOR_VALUE:
				case POINTLESS_VECTOR_VALUE_HASHABLE:
					PyErr_SetString(PyExc_ValueError, "illegal pointless vector type");
					goto cleanup;
				default:
					PyErr_BadInternalCall();
					goto cleanup;
			}
		} else {
			PyErr_Format(PyExc_ValueError, "illegal value vector type: %s", state.v_p[state.n]->ob_type->tp_name);
			goto cleanup;
		}
	}

	// all value vectors must have the same number of items
	uint32_t i;

	for (i = 1; i < state.n; i++) {
		if (state.v_n[0] != state.v_n[i]) {
			PyErr_Format(PyExc_ValueError, "all value vectors must have the same number of items (%i, %i)", (int)state.v_n[0], (int)state.v_n[i]);
			goto cleanup;
		}
	}

	// if there are no items in the projection, we're done
	if (state.p_n == 0)
		goto cleanup;

	// true iff: we are inside bounds
	int in_bounds = 1;

	{
		// find min/max values in projection
		int64_t p_min = 0;
		uint64_t p_max = 0;

		for (i = 0; i < state.p_n; i++) {
			int64_t v_a = 0;
			uint64_t v_b = 0;

			switch (state.p_t) {
				case POINTLESS_PRIM_VECTOR_TYPE_I8:  v_a = ((int8_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U8:  v_b = ((uint8_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_I16: v_a = ((int16_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U16: v_b = ((uint16_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_I32: v_a = ((int32_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U32: v_b = ((uint32_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_I64: v_a = ((int64_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U64: v_b = ((uint64_t*)state.p_b)[i]; break;
			}

			if (i == 0 || v_a < p_min)
				p_min = v_a;

			if (i == 0 || v_b > p_max)
				p_max = v_b;
		}

		// bounds check
		in_bounds = (0 <= p_min && p_max < (uint64_t)state.v_n[0]);

		// if we are inside the bounds, we can sort
		if (in_bounds) {
			switch (state.p_t) {
				case POINTLESS_PRIM_VECTOR_TYPE_I8:  bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_i8,  (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_U8:  bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_u8,  (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_I16: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_i16, (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_U16: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_u16, (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_I32: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_i32, (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_U32: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_u32, (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_I64: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_i64, (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_U64: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_u64, (void*)&state); break;
			}
		}
	}

	// if we were not in bounds: raise an exception
	if (!in_bounds) {
		PyErr_SetString(PyExc_ValueError, "projection value out of bounds");
		goto cleanup;
	}

cleanup:

	if (PyErr_Occurred())
		return 0;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyPointlessPrimVector_sizeof(PyPointlessPrimVector* self)
{
	return PyLong_FromSize_t(sizeof(PyPointlessPrimVector) + pointless_dynarray_n_heap_bytes(&self->array));
}

static PyObject* PyPointlessPrimVector_get_typecode(PyPointlessPrimVector* self, void* closure)
{
	const char* s = 0;

	switch (self->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:    s = "i8";  break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:    s = "u8";  break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:   s = "i16"; break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:   s = "u16"; break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:   s = "i32"; break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:   s = "u32"; break;
		case POINTLESS_PRIM_VECTOR_TYPE_I64:   s = "i64"; break;
		case POINTLESS_PRIM_VECTOR_TYPE_U64:   s = "u64"; break;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT: s = "f";   break;
	}

	if (s == 0) {
		PyErr_BadInternalCall();
		return 0;
	}

	return Py_BuildValue("s", s);
}

static int PyPointlessPrimVector_from_remap_index_vector_prim(PyPointlessPrimVector* v_, size_t i, uint64_t* index)
{
	uint64_t n_u;
	int64_t n_i = 0;
	int is_i = 1;

	switch (v_->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			n_i = *((int8_t*)pointless_dynarray_item_at(&v_->array, i));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			n_i = *((int16_t*)pointless_dynarray_item_at(&v_->array, i));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			n_i = *((int32_t*)pointless_dynarray_item_at(&v_->array, i));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I64:
			n_i = *((int64_t*)pointless_dynarray_item_at(&v_->array, i));
			break;
		default:
			is_i = 0;
			break;
	}

	if (is_i) {
		if (n_i < 0)
			return 0;

		*index = (uint64_t)n_i;
		return 1;
	}

	switch (v_->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			n_u = *((uint8_t*)pointless_dynarray_item_at(&v_->array, i));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			n_u = *((uint16_t*)pointless_dynarray_item_at(&v_->array, i));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			n_u = *((uint32_t*)pointless_dynarray_item_at(&v_->array, i));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U64:
			n_u = *((uint64_t*)pointless_dynarray_item_at(&v_->array, i));
			break;
		default:
			return 0;
	}

	*index = n_u;
	return 1;
}

static int PyPointlessPrimVector_from_remap_index_vector_pointless(PyPointlessVector* v_, size_t i, uint64_t* index)
{
	// take slice into account
	i += v_->slice_i;

	uint64_t n_u;
	int64_t n_i = 0;
	int is_i = 1;

	switch (v_->v->type) {
		case POINTLESS_VECTOR_I8:
			n_i = (int64_t)(*(pointless_reader_vector_i8(&v_->pp->p, v_->v) + i));
			break;
		case POINTLESS_VECTOR_I16:
			n_i = (int64_t)(*(pointless_reader_vector_i16(&v_->pp->p, v_->v) + i));
			break;
		case POINTLESS_VECTOR_I32:
			n_i = (int64_t)(*(pointless_reader_vector_i32(&v_->pp->p, v_->v) + i));
			break;
		case POINTLESS_VECTOR_I64:
			n_i = (int64_t)(*(pointless_reader_vector_i64(&v_->pp->p, v_->v) + i));
			break;
		default:
			is_i = 0;
			break;
	}

	if (is_i) {
		if (n_i < 0)
			return 0;
		*index = (uint64_t)n_i;
		return 1;
	}

	switch (v_->v->type) {
		case POINTLESS_VECTOR_U8:
			n_u = (uint64_t)(*(pointless_reader_vector_u8(&v_->pp->p, v_->v) + i));
			break;
		case POINTLESS_VECTOR_U16:
			n_u = (uint64_t)(*(pointless_reader_vector_u16(&v_->pp->p, v_->v) + i));
			break;
		case POINTLESS_VECTOR_U32:
			n_u = (uint64_t)(*(pointless_reader_vector_u32(&v_->pp->p, v_->v) + i));
			break;
		case POINTLESS_VECTOR_U64:
			n_u = (uint64_t)(*(pointless_reader_vector_u64(&v_->pp->p, v_->v) + i));
			break;
		default:
			return 0;
	}

	*index = n_u;
	return 1;
}

static PyObject* PyPointlessPrimVector_from_remap(PyTypeObject* type, PyObject* args)
{
	PyPointlessPrimVector* r_ = 0;
	PyObject* v_ = 0;
	pointless_dynarray_t a_;

	if (!PyArg_ParseTuple(args, "O!O", &PyPointlessPrimVectorType, &r_, &v_))
		return 0;

	if (!PyPointlessPrimVector_Check(v_) && !PyPointlessVector_Check(v_)) {
		PyErr_SetString(PyExc_ValueError, "index vector must be PointlessPrimVector or PointlessVector");
		return 0;
	}

	// initialize destination vector
	switch (r_->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			pointless_dynarray_init(&a_, sizeof(int8_t));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			pointless_dynarray_init(&a_, sizeof(uint8_t));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			pointless_dynarray_init(&a_, sizeof(int16_t));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			pointless_dynarray_init(&a_, sizeof(uint16_t));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			pointless_dynarray_init(&a_, sizeof(int32_t));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			pointless_dynarray_init(&a_, sizeof(uint32_t));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			pointless_dynarray_init(&a_, sizeof(float));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I64:
			pointless_dynarray_init(&a_, sizeof(int64_t));
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U64:
			pointless_dynarray_init(&a_, sizeof(uint64_t));
			break;
		default:
			PyErr_SetString(PyExc_ValueError, "unknown source vector type");
			return 0;
	}

	// get source vector
	size_t i, n_source = pointless_dynarray_n_items(&r_->array);
	size_t n_index = 0;

	if (PyPointlessPrimVector_Check(v_)) {
		n_index = pointless_dynarray_n_items(&((PyPointlessPrimVector*)v_)->array);
	} else {
		n_index = ((PyPointlessVector*)v_)->slice_n;
	}

	// for each index
	for (i = 0; i < n_index; i++) {
		// get it
		uint64_t index = 0;

		if (PyPointlessPrimVector_Check(v_) && !PyPointlessPrimVector_from_remap_index_vector_prim((PyPointlessPrimVector*)v_, i, &index)) {
			PyErr_SetString(PyExc_ValueError, "index vector negative or of the wrong type");
			return 0;
		}

		if (PyPointlessVector_Check(v_) && !PyPointlessPrimVector_from_remap_index_vector_pointless((PyPointlessVector*)v_, i, &index)) {
			PyErr_SetString(PyExc_ValueError, "index vector negative or of the wrong type");
			return 0;
		}

		// check for bounds
		if (index >= n_source) {
			PyErr_SetString(PyExc_ValueError, "index vector out of bounds");
			return 0;
		}

		// TBD: support more vector types

		// add to destination vector
		void* source = pointless_dynarray_item_at(&r_->array, index);

		if (!pointless_dynarray_push(&a_, source)) {
			pointless_dynarray_destroy(&a_);
			return PyErr_NoMemory();
		}
	}

	return (PyObject*)PyPointlessPrimVector_from_T_vector(&a_, r_->type);
}

#define POINTLESS_PRIMVECTOR_MAX_LOOP(T, v, n) \
	{\
		m_i = 0;\
		for (i = 1; i < (n); i++) {\
			if (((T*)(v))[i] > ((T*)(v))[m_i])\
				m_i = i;\
		}\
	}\

#define POINTLESS_PRIMVECTOR_MIN_LOOP(T, v, n) \
	{\
		m_i = 0;\
		for (i = 1; i < (n); i++) {\
			if (((T*)(v))[i] < ((T*)(v))[m_i])\
				m_i = i;\
		}\
	}\

static PyObject* PyPointlessPrimVector_max(PyPointlessPrimVector* self)
{
	size_t i, m_i, n_items;
	n_items = pointless_dynarray_n_items(&self->array);
	void* base_ptr = pointless_dynarray_buffer(&self->array);

	if (n_items == 0) {
		PyErr_SetString(PyExc_ValueError, "vector is empty");
		return 0;
	}

	switch (self->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			POINTLESS_PRIMVECTOR_MAX_LOOP(int8_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			POINTLESS_PRIMVECTOR_MAX_LOOP(uint8_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			POINTLESS_PRIMVECTOR_MAX_LOOP(int16_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			POINTLESS_PRIMVECTOR_MAX_LOOP(uint16_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			POINTLESS_PRIMVECTOR_MAX_LOOP(int32_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			POINTLESS_PRIMVECTOR_MAX_LOOP(uint32_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I64:
			POINTLESS_PRIMVECTOR_MAX_LOOP(int64_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U64:
			POINTLESS_PRIMVECTOR_MAX_LOOP(uint64_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			POINTLESS_PRIMVECTOR_MAX_LOOP(float, base_ptr, n_items);
			break;
		default:
			PyErr_BadInternalCall();
			return 0;
	}

	return PyPointlessPrimVector_subscript_priv(self, m_i);
}

static PyObject* PyPointlessPrimVector_min(PyPointlessPrimVector* self)
{
	size_t i, m_i, n_items;
	n_items = pointless_dynarray_n_items(&self->array);
	void* base_ptr = pointless_dynarray_buffer(&self->array);

	if (n_items == 0) {
		PyErr_SetString(PyExc_ValueError, "vector is empty");
		return 0;
	}

	switch (self->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			POINTLESS_PRIMVECTOR_MIN_LOOP(int8_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			POINTLESS_PRIMVECTOR_MIN_LOOP(uint8_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			POINTLESS_PRIMVECTOR_MIN_LOOP(int16_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			POINTLESS_PRIMVECTOR_MIN_LOOP(uint16_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			POINTLESS_PRIMVECTOR_MIN_LOOP(int32_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			POINTLESS_PRIMVECTOR_MIN_LOOP(uint32_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I64:
			POINTLESS_PRIMVECTOR_MIN_LOOP(int64_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U64:
			POINTLESS_PRIMVECTOR_MIN_LOOP(uint64_t, base_ptr, n_items);
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			POINTLESS_PRIMVECTOR_MIN_LOOP(float, base_ptr, n_items);
			break;
		default:
			PyErr_BadInternalCall();
			return 0;
	}

	return PyPointlessPrimVector_subscript_priv(self, m_i);
}


static PyGetSetDef PyPointlessPrimVector_getsets [] = {
	{"typecode", (getter)PyPointlessPrimVector_get_typecode, 0, "the typecode string used to create the vector"},
	{NULL}
};

static PyMethodDef PyPointlessPrimVector_methods[] = {
	{"append",      (PyCFunction)PyPointlessPrimVector_append,        METH_VARARGS, ""},
	{"append_bulk", (PyCFunction)PyPointlessPrimVector_append_bulk,   METH_VARARGS, ""},
	{"pop",         (PyCFunction)PyPointlessPrimVector_pop,           METH_NOARGS,  ""},
	{"pop_bulk",    (PyCFunction)PyPointlessPrimVector_pop_bulk,      METH_VARARGS, ""},
	{"index",       (PyCFunction)PyPointlessPrimVector_index,         METH_VARARGS,  ""},
	{"remove",      (PyCFunction)PyPointlessPrimVector_remove,        METH_VARARGS,  ""},
	{"fast_remove", (PyCFunction)PyPointlessPrimVector_fast_remove,   METH_VARARGS,  ""},
	{"serialize",   (PyCFunction)PyPointlessPrimVector_serialize,     METH_NOARGS,  ""},
	{"sort",        (PyCFunction)PyPointlessPrimVector_sort,          METH_NOARGS,  ""},
	{"sort_proj",   (PyCFunction)PyPointlessPrimVector_sort_proj,     METH_VARARGS, ""},
	{"__sizeof__",  (PyCFunction)PyPointlessPrimVector_sizeof,        METH_NOARGS,  ""},
	{"__reversed__",(PyCFunction)PyPointlessPrimVector_rev_iter,     METH_NOARGS,  ""},
	{"clear",       (PyCFunction)PyPointlessPrimVector_clear,         METH_NOARGS,  ""},
	{"FromRemap",   (PyCFunction)PyPointlessPrimVector_from_remap,    METH_VARARGS | METH_CLASS, ""},
	{"max",         (PyCFunction)PyPointlessPrimVector_max,           METH_NOARGS, ""},
	{"min",         (PyCFunction)PyPointlessPrimVector_min,           METH_NOARGS, ""},
	{NULL, NULL}
};

static PyObject* PyPointlessPrimVector_slice(PyPointlessPrimVector* self, Py_ssize_t ilow, Py_ssize_t ihigh)
{
	// clamp the limits
	uint32_t n_items = pointless_dynarray_n_items(&self->array), i;

	if (ilow < 0)
		ilow = 0;
	else if (ilow > (Py_ssize_t)n_items)
		ilow = (Py_ssize_t)n_items;

	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > n_items)
		ihigh = (Py_ssize_t)n_items;

	uint32_t slice_i = (uint32_t)ilow;
	uint32_t slice_n = (uint32_t)(ihigh - ilow);

	PyPointlessPrimVector* pv = PyObject_New(PyPointlessPrimVector, &PyPointlessPrimVectorType);

	if (pv == 0)
		return 0;

	pv->ob_exports = 0;
	pv->type = self->type;
	pointless_dynarray_init(&pv->array, self->array.item_size);

	for (i = 0; i < slice_n; i++) {
		void* ii = pointless_dynarray_item_at(&self->array, i + slice_i);
		if (!pointless_dynarray_push(&pv->array, ii)) {
			Py_DECREF(pv);
			PyErr_NoMemory();
			return 0;
		}
	}

	return (PyObject*)pv;
}

static PyMappingMethods PyPointlessPrimVector_as_mapping = {
	(lenfunc)PyPointlessPrimVector_length,
	(binaryfunc)PyPointlessPrimVector_subscript,
	(objobjargproc)PyPointlessPrimVector_ass_subscript
};

static PySequenceMethods PyPointlessPrimVector_as_sequence = {
	(lenfunc)PyPointlessPrimVector_length,          /* sq_length */
	0,                                          /* sq_concat */
	0,                                          /* sq_repeat */
	(ssizeargfunc)PyPointlessPrimVector_item,       /* sq_item */
    (ssizessizeargfunc)PyPointlessPrimVector_slice, /* sq_slice */
	0,                                          /* sq_ass_item */
    0,                                          /* sq_ass_slice */
	(objobjproc)PyPointlessPrimVector_contains,     /* sq_contains */
	0,                                          /* sq_inplace_concat */
	0,                                          /* sq_inplace_repeat */
};

PyTypeObject PyPointlessPrimVectorType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessPrimVector",              /*tp_name*/
	sizeof(PyPointlessPrimVector),                  /*tp_basicsize*/
	0,                                              /*tp_itemsize*/
	(destructor)PyPointlessPrimVector_dealloc,      /*tp_dealloc*/
	0,                                              /*tp_print*/
	0,                                              /*tp_getattr*/
	0,                                              /*tp_setattr*/
	0,                                              /*tp_compare*/
	(reprfunc)PyPointlessPrimVector_repr,           /*tp_repr*/
	0,                                              /*tp_as_number*/
	&PyPointlessPrimVector_as_sequence,             /*tp_as_sequence*/
	&PyPointlessPrimVector_as_mapping,              /*tp_as_mapping*/
	0,                                              /*tp_hash */
	0,                                              /*tp_call*/
	(reprfunc)PyPointlessPrimVector_str,            /*tp_str*/
	PyObject_GenericGetAttr,                                              /*tp_getattro*/
	0,                                              /*tp_setattro*/
	&PointlessPrimVector_as_buffer,                 /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                             /*tp_flags*/
	"PyPointlessPrimVector wrapper",                /*tp_doc */
	0,                                              /*tp_traverse */
	0,                                              /*tp_clear */
	0,                                              /*tp_richcompare */
	0,                                              /*tp_weaklistoffset */
	PyPointlessPrimVector_iter,                     /*tp_iter */
	0,                                              /*tp_iternext */
	PyPointlessPrimVector_methods,                  /*tp_methods */
	0,                                              /*tp_members */
	PyPointlessPrimVector_getsets,                  /*tp_getset */
	0,                                              /*tp_base */
	0,                                              /*tp_dict */
	0,                                              /*tp_descr_get */
	0,                                              /*tp_descr_set */
	0,                                              /*tp_dictoffset */
	(initproc)PyPointlessPrimVector_init,           /*tp_init */
	PyType_GenericAlloc,                            /*tp_alloc */
	PyType_GenericNew,                              /*tp_new */
	PyObject_Del,                                   /* tp_free */
};


PyTypeObject PyPointlessPrimVectorIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessPrimVectorIter",            /*tp_name*/
	sizeof(PyPointlessPrimVectorIter),                /*tp_basicsize*/
	0,                                                /*tp_itemsize*/
	(destructor)PyPointlessPrimVectorIter_dealloc,    /*tp_dealloc*/
	0,                                                /*tp_print*/
	0,                                                /*tp_getattr*/
	0,                                                /*tp_setattr*/
	0,                                                /*tp_compare*/
	0,                                                /*tp_repr*/
	0,                                                /*tp_as_number*/
	0,                                                /*tp_as_sequence*/
	0,                                                /*tp_as_mapping*/
	0,                                                /*tp_hash */
	0,                                                /*tp_call*/
	0,                                                /*tp_str*/
	PyObject_GenericGetAttr,                          /*tp_getattro*/
	0,                                                /*tp_setattro*/
	0,                                                /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,          /*tp_flags*/
	"PyPointlessPrimVectorIter",                      /*tp_doc */
	0, /*tp_traverse */
	0,                                                /*tp_clear */
	0,                                                /*tp_richcompare */
	0,                                                /*tp_weaklistoffset */
	PyObject_SelfIter,                                /*tp_iter */
	(iternextfunc)PyPointlessPrimVectorIter_iternext, /*tp_iternext */
	0,                                                /*tp_methods */
};

PyTypeObject PyPointlessPrimVectorRevIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessPrimVectorRevIter",            /*tp_name*/
	sizeof(PyPointlessPrimVectorRevIter),                /*tp_basicsize*/
	0,                                                /*tp_itemsize*/
	(destructor)PyPointlessPrimVectorRevIter_dealloc,    /*tp_dealloc*/
	0,                                                /*tp_print*/
	0,                                                /*tp_getattr*/
	0,                                                /*tp_setattr*/
	0,                                                /*tp_compare*/
	0,                                                /*tp_repr*/
	0,                                                /*tp_as_number*/
	0,                                                /*tp_as_sequence*/
	0,                                                /*tp_as_mapping*/
	0,                                                /*tp_hash */
	0,                                                /*tp_call*/
	0,                                                /*tp_str*/
	PyObject_GenericGetAttr,                          /*tp_getattro*/
	0,                                                /*tp_setattro*/
	0,                                                /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,          /*tp_flags*/
	"PyPointlessPrimVectorRevIter",                      /*tp_doc */
	0, /*tp_traverse */
	0,                                                /*tp_clear */
	0,                                                /*tp_richcompare */
	0,                                                /*tp_weaklistoffset */
	PyObject_SelfIter,                                /*tp_iter */
	(iternextfunc)PyPointlessPrimVectorRevIter_iternext, /*tp_iternext */
	0,                                                /*tp_methods */
};

PyPointlessPrimVector* PyPointlessPrimVector_from_T_vector(pointless_dynarray_t* v, uint32_t t)
{
	// NOTE: we're responsible for 'v'
	size_t s = 0;

	switch (t) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:    s = sizeof(int8_t);   break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:    s = sizeof(uint8_t);  break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:   s = sizeof(int16_t);  break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:   s = sizeof(uint16_t); break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:   s = sizeof(int32_t);  break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:   s = sizeof(uint32_t); break;
		case POINTLESS_PRIM_VECTOR_TYPE_I64:   s = sizeof(int64_t);  break;
		case POINTLESS_PRIM_VECTOR_TYPE_U64:   s = sizeof(uint64_t); break;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT: s = sizeof(float);    break;
	}

	if (s == 0) {
		pointless_dynarray_destroy(v);
		PyErr_SetString(PyExc_ValueError, "illegal type passed to PyPointlessPrimVector_from_T_vector()");
		return 0;
	}

	if (s != v->item_size) {
		pointless_dynarray_destroy(v);
		PyErr_SetString(PyExc_ValueError, "illegal vector passed to PyPointlessPrimVector_from_T_vector()");
		return 0;
	}

	PyPointlessPrimVector* pv = PyObject_New(PyPointlessPrimVector, &PyPointlessPrimVectorType);

	if (pv == 0) {
		pointless_dynarray_destroy(v);
		return 0;
	}

	pv->ob_exports = 0;
	pv->type = t;
	pv->array = *v;

	return pv;
}

PyPointlessPrimVector* PyPointlessPrimVector_from_buffer(void* buffer, size_t n_buffer)
{
	pointless_dynarray_t a;
	pointless_dynarray_init(&a, 1);
	pointless_dynarray_give_data(&a, buffer, n_buffer);
	return PyPointlessPrimVector_from_T_vector(&a, POINTLESS_PRIM_VECTOR_TYPE_U8);
}

