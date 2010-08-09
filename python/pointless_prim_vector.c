#include "../pointless_ext.h"

static PyObject* PyPointlessPrimVector_append_item(PyPointlessPrimVector* self, PyObject* item);

static void PyPointlessPrimVector_dealloc(PyPointlessPrimVector* self)
{
	pointless_dynarray_destroy(&self->array);
	self->ob_type->tp_free((PyObject*)self);
}

static void PyPointlessPrimVectorIter_dealloc(PyPointlessPrimVectorIter* self)
{
	Py_XDECREF(self->vector);
	self->vector = 0;
	self->iter_state = 0;
	self->ob_type->tp_free((PyObject*)self);
}

PyObject* PyPointlessPrimVector_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessPrimVector* self = (PyPointlessPrimVector*)type->tp_alloc(type, 0);

	if (self) {
		self->read_lock = 0;
		self->write_lock = 0;
		pointless_dynarray_init(&self->array, 1);
	}

	return (PyObject*)self;
}

PyObject* PyPointlessPrimVectorIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessPrimVectorIter* self = (PyPointlessPrimVectorIter*)type->tp_alloc(type, 0);

	if (self) {
		self->vector = 0;
		self->iter_state = 0;
	}

	return (PyObject*)self;
}

PyObject* PyPointlessPrimVector_str(PyPointlessPrimVector* self)
{
	// if object has a lock
	if (((PyPointlessPrimVector*)self)->write_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a write lock: cannot be read");
		return 0;
	}

	if (!self->allow_print)
		return PyString_FromFormat("<%s object at %p>", Py_TYPE(self)->tp_name, (void*)self);

	PyObject* vector_s = PyString_FromString("[");
	PyObject* vector_comma = PyString_FromString(", ");
	PyObject* vector_postfix = PyString_FromString("]");

	PyObject* v_s = 0;

	if (vector_s == 0 || vector_comma == 0 || vector_postfix == 0)
		goto error;

	uint32_t i, n = pointless_dynarray_n_items(&self->array);
	void* data = 0;
	char buffer[1024];

	for (i = 0; i < n; i++) {
		data = pointless_dynarray_item_at(&self->array, i);

		switch (self->type) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				v_s = PyString_FromFormat("%i", (int)(*((int8_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:
				v_s = PyString_FromFormat("%u", (unsigned int)(*((uint8_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				v_s = PyString_FromFormat("%i", (int)(*((int16_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:
				v_s = PyString_FromFormat("%u", (unsigned int)(*((uint16_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				v_s = PyString_FromFormat("%i", (int)(*((int32_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:
				v_s = PyString_FromFormat("%u", (unsigned int)(*((uint32_t*)data)));
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
				snprintf(buffer, sizeof(buffer), "%f", (*((float*)data)));
				v_s = PyString_FromString(buffer);
				break;
		}

		if (v_s == 0)
			goto error;

		PyString_ConcatAndDel(&vector_s, v_s);
		v_s = 0;

		if (vector_s == 0)
			goto error;

		if (i + 1 < n) {
			PyString_Concat(&vector_s, vector_comma);

			if (vector_s == 0)
				goto error;
		}
	}

	PyString_Concat(&vector_s, vector_postfix);

	if (vector_s == 0)
		goto error;

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

PyObject* PyPointlessPrimVector_repr(PyPointlessPrimVector* self)
{
	return PyPointlessPrimVector_str(self);
}

#define POINTLESS_PRIM_VECTOR_N_TYPES 7

struct {
	const char* s;
	uint32_t type;
	uint32_t typesize;
} pointless_prim_vector_type_map[POINTLESS_PRIM_VECTOR_N_TYPES] = {
	{"i8", POINTLESS_PRIM_VECTOR_TYPE_I8, sizeof(int8_t)},
	{"u8", POINTLESS_PRIM_VECTOR_TYPE_U8, sizeof(uint8_t)},
	{"i16", POINTLESS_PRIM_VECTOR_TYPE_I16, sizeof(int16_t)},
	{"u16", POINTLESS_PRIM_VECTOR_TYPE_U16, sizeof(uint16_t)},
	{"i32", POINTLESS_PRIM_VECTOR_TYPE_I32, sizeof(int32_t)},
	{"u32", POINTLESS_PRIM_VECTOR_TYPE_U32, sizeof(uint32_t)},
	{"f", POINTLESS_PRIM_VECTOR_TYPE_FLOAT, sizeof(float)}
};

static int pointless_get_continuous_buffer(PyObject* buffer_obj, void** buffer, uint32_t* n_buffer, uint32_t* must_free)
{
	// our return values
	*buffer = 0;
	*n_buffer = 0;
	*must_free = 0;

	// object must be a buffer
	if (!PyBuffer_Check(buffer_obj)) {
		PyErr_SetString(PyExc_ValueError, "object is not a buffer");
		return 0;
	}

	// get the buffer interface
	PyBufferProcs* procs = buffer_obj->ob_type->tp_as_buffer;

	// we can't rely in bf_getreadbuffer existing, see http://docs.python.org/c-api/typeobj.html
	if (procs->bf_getreadbuffer == 0) {
		PyErr_SetString(PyExc_ValueError, "buffer object does not support bf_getreadbuffer");
		return 0;
	}

	// get number of segments and bytes
	Py_ssize_t n_bytes = 0;
	Py_ssize_t n_segments = (*procs->bf_getsegcount)(buffer_obj, &n_bytes);

	if (!(0 <= n_bytes && n_bytes <= UINT32_MAX)) {
		PyErr_SetString(PyExc_ValueError, "buffer size not supported");
		return 0;
	}

	// if there are no segments, we're done
	if (n_segments == 0)
		return 1;

	// if there is only a single segment, just get a pointer to that
	if (n_segments == 1) {
		Py_ssize_t segment_len = (*procs->bf_getreadbuffer)(buffer_obj, 0, buffer);

		if (segment_len == -1)
			return 0;

		assert(segment_len == n_bytes);

		*n_buffer = n_bytes;
		return 1;
	}

	// otherwise, we concat all the segments into a single buffer, and free it later
	void* m_buffer = malloc(n_bytes);
	*buffer = 0;
	*n_buffer = n_bytes;
	*must_free = 1;

	if (m_buffer == 0) {
		PyErr_NoMemory();
		return 0;
	}

	Py_ssize_t i, c = 0;

	for (i = 0; i < n_segments; i++) {
		void* ptr = 0;
		Py_ssize_t segment_len = (*procs->bf_getreadbuffer)(buffer_obj, i, &ptr);

		if (segment_len == -1) {
			free(m_buffer);
			return 0;
		}

		memcpy((char*)m_buffer + c, ptr, segment_len);
		c += segment_len;
	}

	*buffer = m_buffer;

	assert(c == n_bytes);

	return 1;
}

static int PyPointlessPrimVector_init(PyPointlessPrimVector* self, PyObject* args, PyObject* kwds)
{
	// printing enabled by default
	self->allow_print = 1;

	// clear previous contents
	pointless_dynarray_clear(&self->array);
	self->read_lock = 0;
	self->write_lock = 0;
	self->type = 0;

	// parse input
	const char* type = 0;
	PyObject* buffer_obj = 0;
	PyObject* sequence_obj = 0;
	PyObject* allow_print = 0;
	uint32_t i;

	static char* kwargs[] = {"type", "buffer", "sequence", "allow_print", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sO!OO!", kwargs, &type, &PyBuffer_Type, &buffer_obj, &sequence_obj, &PyBool_Type, &allow_print))
		return -1;

	if ((type != 0) == (buffer_obj != 0)) {
		PyErr_SetString(PyExc_TypeError, "exactly one of type/buffer must be specified");
		return -1;
	}

	if (type == 0 && sequence_obj != 0) {
		PyErr_SetString(PyExc_ValueError, "sequence only allowed if type is specified");
		return -1;
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
			return -1;
		}

		// if we have an iterator, construct the vector from it
		if (sequence_obj) {
			PyObject* iterator = PyObject_GetIter(sequence_obj);

			if (iterator == 0)
				return -1;

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
				return -1;
			}
		}
	// else a single buffer
	} else {
		int retval = -1;
		void* buffer = 0;
		uint32_t n_buffer = 0;
		uint32_t must_free = 0;

		if (!pointless_get_continuous_buffer(buffer_obj, &buffer, &n_buffer, &must_free))
			goto buffer_cleanup;

		// de-serialize the buffer
		if (n_buffer < sizeof(uint32_t) + sizeof(uint32_t)) {
			PyErr_SetString(PyExc_ValueError, "buffer too short");
			goto buffer_cleanup;
		}

		self->type = ((uint32_t*)buffer)[0];
		uint32_t buffer_n_items = ((uint32_t*)buffer)[1];
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
			goto buffer_cleanup;
		}

		expected_buffer_size += sizeof(uint32_t) + sizeof(uint32_t);

		if ((uint64_t)n_buffer != expected_buffer_size) {
			PyErr_SetString(PyExc_ValueError, "illegal buffer length");
			goto buffer_cleanup;
			return -1;
		}



		for (i = 0; i < buffer_n_items; i++) {
			void* data_buffer = (uint32_t*)buffer + 2;
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
				case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
					added = pointless_dynarray_push(&self->array, (float*)data_buffer + i);
					break;
				default:
					PyErr_SetString(PyExc_Exception, "internal error");
					goto buffer_cleanup;
			}

			if (!added) {
				PyErr_NoMemory();
				goto buffer_cleanup;
			}
		}

		retval = 0;

buffer_cleanup:

		if (retval == -1)
			pointless_dynarray_clear(&self->array);

		if (must_free)
			free(buffer);

		return retval;
	}

	return 0;
}

static int PyPointlessPrimVectorIter_init(PyPointlessPrimVector* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessPrimVectorIter directly");
	return -1;
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
			return PyInt_FromLong((long)(*((int8_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			return PyInt_FromLong((long)(*((uint8_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			return PyInt_FromLong((long)(*((int16_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			return PyInt_FromLong((long)(*((uint16_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			return PyInt_FromLong((long)(*((int32_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			return PyLong_FromUnsignedLong((unsigned long)(*((uint32_t*)base_value)));
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			return PyFloat_FromDouble((double)(*((float*)base_value)));
	}

	PyErr_SetString(PyExc_ValueError, "illegal value type");
	return 0;
}

static PyObject* PyPointlessPrimVector_subscript(PyPointlessPrimVector* self, PyObject* item)
{
	// if object has a lock
	if (self->write_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a write lock: cannot be read");
		return 0;
	}

	// get the index
	Py_ssize_t i;

	if (!PyPointlessPrimVector_check_index(self, item, &i))
		return 0;

	return PyPointlessPrimVector_subscript_priv(self, (uint32_t)i);
}

static PyObject* PyPointlessPrimVector_item(PyPointlessPrimVector* self, Py_ssize_t i)
{
	// if object has a lock
	if (self->write_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a write lock: cannot be read");
		return 0;
	}

	if (!(0 <= i && i < pointless_dynarray_n_items(&self->array))) {
		PyErr_SetString(PyExc_IndexError, "vector index out of range");
		return 0;
	}

	return PyPointlessPrimVector_subscript_priv(self, (uint32_t)i);
}

static int PyPointlessPrimVector_ass_item(PyPointlessPrimVector* self, Py_ssize_t i, PyObject* v)
{
	if (self->write_lock > 0 || self->read_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a read/write lock: cannot be modified");
		return -1;
	}

	if (!(0 <= i && i < PyPointlessPrimVector_length(self))) {
		PyErr_SetString(PyExc_IndexError, "vector index out of range");
		return -1;
	}

	PY_LONG_LONG ii;
	float ff;

	if (self->type == POINTLESS_PRIM_VECTOR_TYPE_FLOAT) {
		if (!(PyFloat_Check(v) && PyArg_Parse(v, "f", &ff))) {
			PyErr_SetString(PyExc_TypeError, "expected a float");
			return -1;
		}
	} else {
		if (!((PyInt_Check(v) || PyLong_Check(v)) && PyArg_Parse(v, "L", &ii))) {
			PyErr_SetString(PyExc_TypeError, "expected an integer");
			return -1;
		}
	}

	int in_range = 0;

	union {
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		float f;
	} vv;

	switch (self->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			in_range = (INT8_MIN <= ii && ii <= INT8_MAX);
			vv.i8 = (int8_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			in_range = (0 <= ii && ii <= UINT8_MAX);
			vv.u8 = (int8_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			in_range = (INT16_MIN <= ii && ii <= INT16_MAX);
			vv.i16 = (int16_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			in_range = (0 <= ii && ii <= UINT16_MAX);
			vv.u16 = (uint16_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			in_range = (INT32_MIN <= ii && ii <= INT32_MAX);
			vv.i32 = (int32_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			in_range = (0 <= ii && ii <= UINT32_MAX);
			vv.u32 = (uint32_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			in_range = 1;
			vv.f = ff;
			break;
	}

	if (!in_range) {
		PyErr_SetString(PyExc_ValueError, "given value is smaller/greater than the allowed values");
		return -1;
	}

	memcpy(pointless_dynarray_item_at(&self->array, i), &vv, self->array.item_size);

	return 0;
}

static PyObject* PyPointlessPrimVector_iter(PyObject* vector)
{
	if (!PyPointlessPrimVector_Check(vector)) {
		PyErr_BadInternalCall();
		return 0;
	}

	// if object has a lock
	if (((PyPointlessPrimVector*)vector)->write_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a write lock: cannot be read");
		return 0;
	}

	PyPointlessPrimVectorIter* iter = PyObject_New(PyPointlessPrimVectorIter, &PyPointlessPrimVectorIterType);

	if (iter == 0)
		return 0;

	iter = (PyPointlessPrimVectorIter*)PyObject_Init((PyObject*)iter, &PyPointlessPrimVectorIterType);

	Py_INCREF(vector);

	iter->vector = (PyPointlessPrimVector*)vector;
	iter->iter_state = 0;

	return (PyObject*)iter;
}

static PyObject* PyPointlessPrimVectorIter_iternext(PyPointlessPrimVectorIter* iter)
{
	// if object has a lock
	if (iter->vector->write_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a write lock: cannot be read");
		return 0;
	}

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

static PyObject* PyPointlessPrimVector_append_item(PyPointlessPrimVector* self, PyObject* item)
{
	PY_LONG_LONG ii;
	float ff;

	if (self->type == POINTLESS_PRIM_VECTOR_TYPE_FLOAT) {
		if (!(PyFloat_Check(item) && PyArg_Parse(item, "f", &ff))) {
			PyErr_SetString(PyExc_TypeError, "expected a float");
			return 0;
		}
	} else {
		if (!((PyInt_Check(item) || PyLong_Check(item)) && PyArg_Parse(item, "L", &ii))) {
			PyErr_SetString(PyExc_TypeError, "expected an integer");
			return 0;
		}
	}

	int in_range = 0;

	union {
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		float f;
	} vv;

	switch (self->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			in_range = (INT8_MIN <= ii && ii <= INT8_MAX);
			vv.i8 = (int8_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			in_range = (0 <= ii && ii <= UINT8_MAX);
			vv.u8 = (int8_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			in_range = (INT16_MIN <= ii && ii <= INT16_MAX);
			vv.i16 = (int16_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			in_range = (0 <= ii && ii <= UINT16_MAX);
			vv.u16 = (uint16_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			in_range = (INT32_MIN <= ii && ii <= INT32_MAX);
			vv.i32 = (int32_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			in_range = (0 <= ii && ii <= UINT32_MAX);
			vv.u32 = (uint32_t)ii;
			break;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			in_range = 1;
			vv.f = ff;
			break;
	}

	if (!in_range) {
		PyErr_SetString(PyExc_ValueError, "given value is smaller/greater than the allowed values");
		return 0;
	}

	if (!pointless_dynarray_push(&self->array, &vv))
		return PyErr_NoMemory();

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

	if (self->write_lock > 0 || self->read_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a read/write lock: cannot be modified");
		return 0;
	}

	return PyPointlessPrimVector_append_item(self, obj);
}

static PyObject* PyPointlessPrimVector_pop(PyPointlessPrimVector* self)
{
	if (self->write_lock > 0 || self->read_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a read/write lock: cannot be modified");
		return 0;
	}

	size_t n_items = pointless_dynarray_n_items(&self->array);

	if (n_items == 0) {
		PyErr_SetString(PyExc_IndexError, "pop from empty vector");
		return 0;
	}

	PyObject* v = PyPointlessPrimVector_subscript_priv(self, n_items - 1);

	if (v == 0)
		return 0;

	pointless_dynarray_pop(&self->array);
	return v;
}

static size_t PyPointlessPrimVector_index_f(PyPointlessPrimVector* self, float v)
{
	uint32_t i, n = pointless_dynarray_n_items(&self->array);
	float* a = (float*)(self->array._data);

	for (i = 0; i < n; i++) {
		if (a[i] == v)
			return i;
	}

	return SIZE_MAX;
}

static size_t PyPointlessPrimVector_index_i(PyPointlessPrimVector* self, int64_t v)
{
	uint32_t i, n = pointless_dynarray_n_items(&self->array);
	void* a = (float*)(self->array._data);

	for (i = 0; i < n; i++) {
		switch (self->type) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				if (((int8_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:
				if (((uint8_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				if (((int16_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:
				if (((uint16_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				if (((int32_t*)a)[i] == v)
					return i;
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:
				if (((uint32_t*)a)[i] == v)
					return i;
				break;
		}
	}

	return SIZE_MAX;
}

static size_t PyPointlessPrimVector_index_(PyPointlessPrimVector* self, PyObject* args, const char* func)
{
	PY_LONG_LONG ii;
	float ff;
	size_t i;

	if (self->type == POINTLESS_PRIM_VECTOR_TYPE_FLOAT) {
		if (!PyArg_ParseTuple(args, "f", &ff))
			return (SIZE_MAX-1);

		i = PyPointlessPrimVector_index_f(self, ff);
	} else {
		if (!PyArg_ParseTuple(args, "L", &ii))
			return (SIZE_MAX-1);

		i = PyPointlessPrimVector_index_i(self, ii);
	}

	if (i == SIZE_MAX) {
		PyErr_Format(PyExc_ValueError, "vector.%s(x): x not in vector", func);
		return (SIZE_MAX-1);
	}

	return i;
}

static PyObject* PyPointlessPrimVector_index(PyPointlessPrimVector* self, PyObject* args)
{
	if (self->write_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a write lock: cannot be read");
		return 0;
	}

	size_t i = PyPointlessPrimVector_index_(self, args, "index");

	if (i == (SIZE_MAX-1))
		return 0;

	return PyInt_FromSize_t(i);
}

static PyObject* PyPointlessPrimVector_remove(PyPointlessPrimVector* self, PyObject* args)
{
	if (self->write_lock > 0 || self->read_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a read/write lock: cannot be modified");
		return 0;
	}

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
	if (self->write_lock > 0 || self->read_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a read/write lock: cannot be modified");
		return 0;
	}

	size_t i = PyPointlessPrimVector_index_(self, args, "fast_remove");

	if (i == (SIZE_MAX-1))
		return 0;

	// swap with last item and remove last item
	pointless_dynarray_swap(&self->array, i, pointless_dynarray_n_items(&self->array) - 1);
	pointless_dynarray_pop(&self->array);

	Py_INCREF(Py_None);
	return Py_None;	
}

static PyObject* PyPointlessPrimVector_serialize(PyPointlessPrimVector* self)
{
	if (self->write_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a write lock: cannot be read");
		return 0;
	}

	// the format is: [uint32_t type] [uint32 length] [raw integers]
	// the length param is redundant, but gives a fair sanity check

	// create a buffer
	uint32_t typesize = 0, i;

	for (i = 0; i < POINTLESS_PRIM_VECTOR_N_TYPES; i++) {
		if (pointless_prim_vector_type_map[i].type == self->type) {
			typesize = pointless_prim_vector_type_map[i].typesize;
			break;
		}
	}

	if (typesize == 0) {
		PyErr_SetString(PyExc_Exception, "internal error, unknown type");
		return 0;
	}

	uint32_t n_items = pointless_dynarray_n_items(&self->array);
	uint64_t n_buffer = sizeof(uint32_t) + sizeof(uint32_t) + (uint64_t)n_items * (uint64_t)typesize;

	if (n_buffer > UINT32_MAX) {
		PyErr_SetString(PyExc_Exception, "vector too large for serialization");
		return 0;
	}

	// allocate a buffer object
	PyObject* buffer_object = PyBuffer_New((uint32_t)n_buffer);

	if (buffer_object == 0)
		return 0;

	// copy the data across
	PyBufferProcs* procs = buffer_object->ob_type->tp_as_buffer;
	void* buffer_ptr = 0;

	if ((*procs->bf_getwritebuffer)(buffer_object, 0, &buffer_ptr) == -1) {
		Py_DECREF(buffer_object);
		return 0;
	}

	// serialize it
	((uint32_t*)buffer_ptr)[0] = self->type;
	((uint32_t*)buffer_ptr)[1] = n_items;
	memcpy((void*)((uint32_t*)buffer_ptr + 2), pointless_dynarray_buffer(&self->array), n_items * typesize);

	// we're done
	return buffer_object;
}

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
static int prim_sort_cmp_f(int a, int b, int* c, void* user)
	{ PRIM_SORT_CMP(float, user, a, b); }

static PyObject* PyPointlessPrimVector_sort(PyPointlessPrimVector* self)
{
	if (self->write_lock > 0 || self->read_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a read/write lock: cannot be modified");
		return 0;
	}

	// lock
	self->write_lock += 1;
	int bad = 0;
	Py_INCREF(self);
	Py_BEGIN_ALLOW_THREADS

	{
		int n = (int)pointless_dynarray_n_items(&self->array);

		switch (self->type) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:    bentley_sort_(n, prim_sort_cmp_i8,  prim_sort_swap_i8,  (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:    bentley_sort_(n, prim_sort_cmp_u8,  prim_sort_swap_u8,  (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:   bentley_sort_(n, prim_sort_cmp_i16, prim_sort_swap_i16, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:   bentley_sort_(n, prim_sort_cmp_u16, prim_sort_swap_u16, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:   bentley_sort_(n, prim_sort_cmp_i32, prim_sort_swap_i32, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:   bentley_sort_(n, prim_sort_cmp_u32, prim_sort_swap_u32, (void*)self); break;
			case POINTLESS_PRIM_VECTOR_TYPE_FLOAT: bentley_sort_(n, prim_sort_cmp_f,   prim_sort_swap_f,   (void*)self); break;
			default:
				bad = 1;
				break;
		}
	}

	// unlock
	Py_END_ALLOW_THREADS
	Py_DECREF(self);
	self->write_lock -= 1;

	if (bad) {
		PyErr_BadInternalCall();
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

typedef struct {
	// the projection
	void* p_b;     // base pointer
	uint32_t p_n;  // number of items
	uint32_t p_t;  // item type
	uint32_t p_w;  // lock status

	// sort value vectors
	uint32_t n;

	PyObject* v_p[16]; // Python objects
	void* v_b[16];     // base pointers
	uint32_t v_n[16];  // number of items
	uint32_t v_t[16];  // item types
	uint32_t v_w[16];  // lock status
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

static int prim_sort_proj_cmp(int a, int b, int* c, void* user)
{
	prim_sort_proj_state_t* state = (prim_sort_proj_state_t*)user;

	uint32_t i_a = 0, i_b = 0, i;

	switch (state->p_t) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:  i_a = ((int8_t*)state->p_b)[a];   i_b = ((int8_t*)state->p_b)[b];   break;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:  i_a = ((uint8_t*)state->p_b)[a];  i_b = ((uint8_t*)state->p_b)[b];  break;
		case POINTLESS_PRIM_VECTOR_TYPE_I16: i_a = ((int16_t*)state->p_b)[a];  i_b = ((int16_t*)state->p_b)[b];  break;
		case POINTLESS_PRIM_VECTOR_TYPE_U16: i_a = ((uint16_t*)state->p_b)[a]; i_b = ((uint16_t*)state->p_b)[b]; break;
		case POINTLESS_PRIM_VECTOR_TYPE_I32: i_a = ((int32_t*)state->p_b)[a];  i_b = ((int32_t*)state->p_b)[b];  break;
		case POINTLESS_PRIM_VECTOR_TYPE_U32: i_a = ((uint32_t*)state->p_b)[a]; i_b = ((uint32_t*)state->p_b)[b]; break;
		default: assert(0); break;
	}

	*c = 0;

	for (i = 0; i < state->n && *c == 0; i++) {
		switch (state->v_t[i]) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				*c = SORT_CMP(int8_t, state->v_b[i], i_a, i_b);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:
				*c = SORT_CMP(uint8_t, state->v_b[i], i_a, i_b);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				*c = SORT_CMP(int16_t, state->v_b[i], i_a, i_b);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:
				*c = SORT_CMP(uint16_t, state->v_b[i], i_a, i_b);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				*c = SORT_CMP(int32_t, state->v_b[i], i_a, i_b);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:
				*c = SORT_CMP(uint32_t, state->v_b[i], i_a, i_b);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
				*c = SORT_CMP(float, state->v_b[i], i_a, i_b);
				break;
		}
	}

	// HACK
	if (*c == 0)
		*c = SIMPLE_CMP(a, b);

	return 1;
}

static PyObject* PyPointlessPrimVector_sort_proj(PyPointlessPrimVector* self, PyObject* args)
{
	if (self->write_lock > 0 || self->read_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a read/write lock: cannot be modified");
		return 0;
	}

	// initialize sort state
	prim_sort_proj_state_t state;

	state.p_b = 0;
	state.p_n = 0;
	state.p_t = 0;
	state.p_w = 0;

	for (state.n = 0; state.n < 16; state.n++) {
		state.v_p[state.n] = 0;
		state.v_b[state.n] = 0;
		state.v_n[state.n] = 0;
		state.v_t[state.n] = 0;
		state.v_w[state.n] = 0;
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
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			break;
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

			if (ppv->write_lock > 0) {
				PyErr_SetString(PyExc_ValueError, "vector has a write lock: cannot be read");
				return 0;
			}

		} else if (PyPointlessVector_Check(state.v_p[state.n])) {
			PyPointlessVector* pv = (PyPointlessVector*)state.v_p[state.n];

			state.v_n[state.n] = pv->slice_n;
			state.v_t[state.n] = pv->v->type;

			switch (pv->v->type) {
				// we only want primitive types, or empty vectors
				case POINTLESS_VECTOR_I8:    state.v_b[state.n] = (void*)(pointless_reader_vector_i8(&pv->pp->p, pv->v)    + pv->slice_i); break;
				case POINTLESS_VECTOR_U8:    state.v_b[state.n] = (void*)(pointless_reader_vector_u8(&pv->pp->p, pv->v)    + pv->slice_i); break;
				case POINTLESS_VECTOR_I16:   state.v_b[state.n] = (void*)(pointless_reader_vector_i16(&pv->pp->p, pv->v)   + pv->slice_i); break;
				case POINTLESS_VECTOR_U16:   state.v_b[state.n] = (void*)(pointless_reader_vector_u16(&pv->pp->p, pv->v)   + pv->slice_i); break;
				case POINTLESS_VECTOR_I32:   state.v_b[state.n] = (void*)(pointless_reader_vector_i32(&pv->pp->p, pv->v)   + pv->slice_i); break;
				case POINTLESS_VECTOR_U32:   state.v_b[state.n] = (void*)(pointless_reader_vector_u32(&pv->pp->p, pv->v)   + pv->slice_i); break;
				case POINTLESS_VECTOR_FLOAT: state.v_b[state.n] = (void*)(pointless_reader_vector_float(&pv->pp->p, pv->v) + pv->slice_i); break;
				case POINTLESS_VECTOR_EMPTY: state.v_b[state.n] = 0; break;
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

		Py_INCREF(state.v_p[state.n]);
		state.v_w[state.n] = 1;
	}

	if (self->write_lock > 0 || self->read_lock > 0) {
		PyErr_SetString(PyExc_ValueError, "vector has a read/write lock: cannot be modified");
		return 0;
	}

	state.p_w = 1;
	Py_INCREF(self);

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

	// release GIL
	Py_BEGIN_ALLOW_THREADS

	{
		// find min/max values in projection
		int64_t p_min = 0, p_max = 0;

		for (i = 0; i < state.p_n; i++) {
			int64_t v = 0;

			switch (state.p_t) {
				case POINTLESS_PRIM_VECTOR_TYPE_I8:  v = ((int8_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U8:  v = ((uint8_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_I16: v = ((int16_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U16: v = ((uint16_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_I32: v = ((int32_t*)state.p_b)[i]; break;
				case POINTLESS_PRIM_VECTOR_TYPE_U32: v = ((uint32_t*)state.p_b)[i]; break;
			}

			if (i == 0 || v < p_min)
				p_min = v;

			if (i == 0 || v > p_max)
				p_max = v;
		}

		// bounds check
		in_bounds = (0 <= p_min && p_max < state.v_n[0]);

		// if we are inside the bounds, we can sort
		if (in_bounds) {
			switch (state.p_t) {
				case POINTLESS_PRIM_VECTOR_TYPE_I8:  bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_i8,  (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_U8:  bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_u8,  (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_I16: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_i16, (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_U16: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_u16, (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_I32: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_i32, (void*)&state); break;
				case POINTLESS_PRIM_VECTOR_TYPE_U32: bentley_sort_((int)state.p_n, prim_sort_proj_cmp,  prim_sort_proj_swap_u32, (void*)&state); break;
			}
		}
	}

	// acquire GIL
	Py_END_ALLOW_THREADS

	// if we were not in bounds: raise an exception
	if (!in_bounds) {
		PyErr_SetString(PyExc_ValueError, "projection value out of bounds");
		goto cleanup;
	}

cleanup:

	for (state.n = 0; state.n < 16 && state.v_p[state.n]; state.n++) {
		if (state.v_w[state.n]) {
			if (PyPointlessPrimVector_Check(state.v_p[state.n])) {
				PyPointlessPrimVector* ppv = (PyPointlessPrimVector*)state.v_p[state.n];
				ppv->write_lock -= 1;
			}

			Py_DECREF(state.v_p[state.n]);
		}
	}

	if (state.p_w) {
		self->write_lock -= 1;
		Py_DECREF(self);
	}

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
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT: s = "f";   break;
	}

	if (s == 0) {
		PyErr_BadInternalCall();
		return 0;
	}

	return Py_BuildValue("s", s);
}

static PyGetSetDef PyPointlessPrimVector_getsets [] = {
	{"typecode", (getter)PyPointlessPrimVector_get_typecode, 0, "the typecode string used to create the vector"},
	{NULL}
};

static PyMethodDef PyPointlessPrimVector_methods[] = {
	{"append",      (PyCFunction)PyPointlessPrimVector_append,      METH_VARARGS, ""},
	{"pop",         (PyCFunction)PyPointlessPrimVector_pop,         METH_NOARGS,  ""},
	{"index",       (PyCFunction)PyPointlessPrimVector_index,       METH_VARARGS,  ""},
	{"remove",      (PyCFunction)PyPointlessPrimVector_remove,      METH_VARARGS,  ""},
	{"fast_remove", (PyCFunction)PyPointlessPrimVector_fast_remove, METH_VARARGS,  ""},
	{"serialize",   (PyCFunction)PyPointlessPrimVector_serialize,   METH_NOARGS,  ""},
	{"sort",        (PyCFunction)PyPointlessPrimVector_sort,        METH_NOARGS,  ""},
	{"sort_proj",   (PyCFunction)PyPointlessPrimVector_sort_proj,   METH_VARARGS, ""},
	{"__sizeof__",  (PyCFunction)PyPointlessPrimVector_sizeof,      METH_NOARGS,  ""}, 
	{NULL, NULL}
};

static PyObject* PyPointlessPrimVector_slice(PyPointlessPrimVector* self, Py_ssize_t ilow, Py_ssize_t ihigh)
{
	if (self->write_lock > 0) {
		PyErr_SetString(PyExc_BufferError, "object has a write lock: cannot be modified");
		return 0;
	}

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

	pv = (PyPointlessPrimVector*)PyObject_Init((PyObject*)pv, &PyPointlessPrimVectorType);

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
	(objobjargproc)0
};

static PySequenceMethods PyPointlessPrimVector_as_sequence = {
	(lenfunc)PyPointlessPrimVector_length,            /* sq_length */
	0,                                                /* sq_concat */
	0,                                                /* sq_repeat */
	(ssizeargfunc)PyPointlessPrimVector_item,         /* sq_item */
	(ssizessizeargfunc)PyPointlessPrimVector_slice,   /* sq_slice */
	(ssizeobjargproc)PyPointlessPrimVector_ass_item,  /* sq_ass_item */
	0,                                                /* sq_ass_slice */
	(objobjproc)0,                                    /* sq_contains */
	0,                                                /* sq_inplace_concat */
	0,                                                /* sq_inplace_repeat */
};

PyTypeObject PyPointlessPrimVectorType = {
	PyObject_HEAD_INIT(NULL)
	0,                                              /*ob_size*/
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
	0,                                              /*tp_getattro*/
	0,                                              /*tp_setattro*/
	0,                                              /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_NEWBUFFER, /*tp_flags*/
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
	0,                                              /*tp_alloc */
	PyPointlessPrimVector_new,                      /*tp_new */
};

PyTypeObject PyPointlessPrimVectorIterType = {
	PyObject_HEAD_INIT(NULL)
	0,                                                /*ob_size*/
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
	0,                                                /*tp_getattro*/
	0,                                                /*tp_setattro*/
	0,                                                /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                               /*tp_flags*/
	"PyPointlessPrimVectorIter",                      /*tp_doc */
	0,                                                /*tp_traverse */
	0,                                                /*tp_clear */
	0,                                                /*tp_richcompare */
	0,                                                /*tp_weaklistoffset */
	PyObject_SelfIter,                                /*tp_iter */
	(iternextfunc)PyPointlessPrimVectorIter_iternext, /*tp_iternext */
	0,                                                /*tp_methods */
	0,                                                /*tp_members */
	0,                                                /*tp_getset */
	0,                                                /*tp_base */
	0,                                                /*tp_dict */
	0,                                                /*tp_descr_get */
	0,                                                /*tp_descr_set */
	0,                                                /*tp_dictoffset */
	(initproc)PyPointlessPrimVectorIter_init,         /*tp_init */
	0,                                                /*tp_alloc */
	PyPointlessPrimVectorIter_new,                    /*tp_new */
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

	pv = (PyPointlessPrimVector*)PyObject_Init((PyObject*)pv, &PyPointlessPrimVectorType);

	pv->type = t;
	pv->array = *v;

	return pv;
}
