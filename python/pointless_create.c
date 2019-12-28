#include "../pointless_ext.h"

typedef struct {
	pointless_create_t c;   // create-time state
	int is_error;           // true iff error, python exception is also set
	int error_line;
	Pvoid_t objects_used;   // PyObject* -> create-time-handle
	int unwiden_strings;    // true iff: we find the smallest representations for strings
	int normalize_bitvector;
} pointless_export_state_t;

static uint32_t pointless_export_get_seen(pointless_export_state_t* state, PyObject* py_object)
{
	PWord_t handle = 0;
	JLG(handle, state->objects_used, (Word_t)py_object);
	return handle ? (uint32_t)(*handle) : POINTLESS_CREATE_VALUE_FAIL;
}

static int pointless_export_set_seen(pointless_export_state_t* state, PyObject* py_object, uint32_t handle)
{
	PWord_t value = 0;
	JLI(value, state->objects_used, (Word_t)py_object);

	if (value == 0)
		return 0;

	*value = (Word_t)handle;
	return 1;
}

static uint32_t pointless_export_py_rec(pointless_export_state_t* state, PyObject* py_object, uint32_t depth)
{
	// don't go too deep
	if (depth >= POINTLESS_MAX_DEPTH) {
		PyErr_SetString(PyExc_ValueError, "structure is too deep");
		state->is_error = 1;
		state->error_line = __LINE__;
		return POINTLESS_CREATE_VALUE_FAIL;
	}

	// check simple types first
	uint32_t handle = POINTLESS_CREATE_VALUE_FAIL;

	// return an error on failure
	#define RETURN_OOM(state) {PyErr_NoMemory(); (state)->is_error = 1; state->error_line = __LINE__; return POINTLESS_CREATE_VALUE_FAIL;}
	#define RETURN_OOM_IF_FAIL(handle, state) if ((handle) == POINTLESS_CREATE_VALUE_FAIL) RETURN_OOM(state);

	// booleans, need this above integer check, cause PyInt_Check return 1 for booleans
	if (PyBool_Check(py_object)) {
		if (py_object == Py_True)
			handle = pointless_create_boolean_true(&state->c);
		else
			handle = pointless_create_boolean_false(&state->c);

		RETURN_OOM_IF_FAIL(handle, state);
	// long
	} else if (PyLong_Check(py_object)) {
		// this will raise an overflow error if number is outside the legal range of PY_LONG_LONG
		PY_LONG_LONG v = PyLong_AsLongLong(py_object);

		// if there was an exception, clear it, and set our own
		if (PyErr_Occurred()) {
			PyErr_Clear();
			PyErr_SetString(PyExc_ValueError, "value of long is way beyond what we can store right now");
			state->is_error = 1;
			state->error_line = __LINE__;
			return POINTLESS_CREATE_VALUE_FAIL;
		}

		// unsigned
		if (v >= 0) {
			if (v > UINT32_MAX) {
				PyErr_Format(PyExc_ValueError, "long too large for mere 32 bits");
				state->is_error = 1;
				state->error_line = __LINE__;
				return POINTLESS_CREATE_VALUE_FAIL;
			}

			handle = pointless_create_u32(&state->c, (uint32_t)v);
		// signed
		} else {
			if (!(INT32_MIN <= v && v <= INT32_MAX)) {
				PyErr_Format(PyExc_ValueError, "long too large for mere 32 bits with a sign");
				state->is_error = 1;
				state->error_line = __LINE__;
				return POINTLESS_CREATE_VALUE_FAIL;
			}

			handle = pointless_create_i32(&state->c, (int32_t)v);
		}

		RETURN_OOM_IF_FAIL(handle, state);
	// None object
	} else if (py_object == Py_None) {
		handle = pointless_create_null(&state->c);
		RETURN_OOM_IF_FAIL(handle, state);
	} else if (PyFloat_Check(py_object)) {
		handle = pointless_create_float(&state->c, (float)PyFloat_AS_DOUBLE(py_object));
		RETURN_OOM_IF_FAIL(handle, state);
	}

	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	// remaining types are containers/big-values, which we track
	// either for space-savings or maintaining circular references

	// if object has been seen before, return its handle
	handle = pointless_export_get_seen(state, py_object);

	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	// list/tuple object
	if (PyList_Check(py_object) || PyTuple_Check(py_object)) {
		// create and cache handle
		assert(is_container(py_object));

		handle = pointless_create_vector_value(&state->c);
		RETURN_OOM_IF_FAIL(handle, state);

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

		// populate vector
		Py_ssize_t i, n_items = PyList_Check(py_object) ? PyList_GET_SIZE(py_object) : PyTuple_GET_SIZE(py_object);

		for (i = 0; i < n_items; i++) {
			PyObject* child = PyList_Check(py_object) ? PyList_GET_ITEM(py_object, i) : PyTuple_GET_ITEM(py_object, i);
			uint32_t child_handle = pointless_export_py_rec(state, child, depth + 1);

			if (child_handle == POINTLESS_CREATE_VALUE_FAIL)
				return child_handle;

			if (pointless_create_vector_value_append(&state->c, handle, child_handle) == POINTLESS_CREATE_VALUE_FAIL) {
				RETURN_OOM(state);
			}
		}

	// pointless value vectors
	} else if (PyPointlessVector_Check(py_object)) {
		// currently, we only support value vectors, they are simple
		PyPointlessVector* v = (PyPointlessVector*)py_object;
		const char* error = 0;

		switch(v->v->type) {
			case POINTLESS_VECTOR_VALUE:
			case POINTLESS_VECTOR_VALUE_HASHABLE:
				handle = pointless_recreate_value(&v->pp->p, v->v, &state->c, &error);

				if (handle == POINTLESS_CREATE_VALUE_FAIL) {
					state->is_error = 1;
					state->error_line = __LINE__;
					PyErr_Format(PyExc_ValueError, "pointless_recreate_value(): %s", error);
					return POINTLESS_CREATE_VALUE_FAIL;
				}

				break;
			case POINTLESS_VECTOR_I8:
				handle = pointless_create_vector_i8_owner(&state->c, pointless_reader_vector_i8(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_U8:
				handle = pointless_create_vector_u8_owner(&state->c, pointless_reader_vector_u8(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_I16:
				handle = pointless_create_vector_i16_owner(&state->c, pointless_reader_vector_i16(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_U16:
				handle = pointless_create_vector_u16_owner(&state->c, pointless_reader_vector_u16(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_I32:
				handle = pointless_create_vector_i32_owner(&state->c, pointless_reader_vector_i32(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_U32:
				handle = pointless_create_vector_u32_owner(&state->c, pointless_reader_vector_u32(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_I64:
				handle = pointless_create_vector_i64_owner(&state->c, pointless_reader_vector_i64(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_U64:
				handle = pointless_create_vector_u64_owner(&state->c, pointless_reader_vector_u64(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_FLOAT:
				handle = pointless_create_vector_float_owner(&state->c, pointless_reader_vector_float(&v->pp->p, v->v) + v->slice_i, v->slice_n);
				break;
			case POINTLESS_VECTOR_EMPTY:
				handle = pointless_create_vector_value(&state->c);
				break;
			default:
				state->error_line = __LINE__;
				state->is_error = 1;
				PyErr_SetString(PyExc_ValueError, "internal error: illegal type for primitive vector");
				return POINTLESS_CREATE_VALUE_FAIL;
		}

		RETURN_OOM_IF_FAIL(handle, state);

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

	// python bytearray
	} else if (PyByteArray_Check(py_object)) {
		// create handle and hand over the memory
		Py_ssize_t n_items = PyByteArray_GET_SIZE(py_object);

		if (n_items > UINT32_MAX) {
			PyErr_SetString(PyExc_ValueError, "bytearray has too many items");
			state->error_line = __LINE__;
			state->is_error = 1;
			return POINTLESS_CREATE_VALUE_FAIL;
		}

		handle = pointless_create_vector_u8_owner(&state->c, (uint8_t*)PyByteArray_AsString(py_object), (uint32_t)n_items);
		RETURN_OOM_IF_FAIL(handle, state);

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

	// primitive vectors
	} else if (PyPointlessPrimVector_Check(py_object)) {
		// we just hand over the memory
		PyPointlessPrimVector* prim_vector = (PyPointlessPrimVector*)py_object;
		uint32_t n_items = pointless_dynarray_n_items(&prim_vector->array);
		void* data = prim_vector->array._data;

		switch (prim_vector->type) {
			case POINTLESS_PRIM_VECTOR_TYPE_I8:
				handle = pointless_create_vector_i8_owner(&state->c, (int8_t*)data, n_items);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U8:
				handle = pointless_create_vector_u8_owner(&state->c, (uint8_t*)data, n_items);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I16:
				handle = pointless_create_vector_i16_owner(&state->c, (int16_t*)data, n_items);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U16:
				handle = pointless_create_vector_u16_owner(&state->c, (uint16_t*)data, n_items);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I32:
				handle = pointless_create_vector_i32_owner(&state->c, (int32_t*)data, n_items);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U32:
				handle = pointless_create_vector_u32_owner(&state->c, (uint32_t*)data, n_items);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_I64:
				handle = pointless_create_vector_i64_owner(&state->c, (int64_t*)data, n_items);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_U64:
				handle = pointless_create_vector_u64_owner(&state->c, (uint64_t*)data, n_items);
				break;
			case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
				handle = pointless_create_vector_float_owner(&state->c, (float*)data, n_items);
				break;
			default:
				PyErr_SetString(PyExc_ValueError, "internal error: illegal type for primitive vector");
				state->error_line = __LINE__;
				state->is_error = 1;
				return POINTLESS_CREATE_VALUE_FAIL;
		}

		RETURN_OOM_IF_FAIL(handle, state);

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

	// unicode object
	} else if (PyUnicode_Check(py_object)) {
		Py_ssize_t s_len_python = PyUnicode_GET_LENGTH(py_object);
		Py_ssize_t s_len_pointless = 0;

		switch (PyUnicode_KIND(py_object)) {
			case PyUnicode_WCHAR_KIND:
				PyErr_SetString(PyExc_ValueError, "wchar kind unsupported");
				state->error_line = __LINE__;
				state->is_error = 1;
				return POINTLESS_CREATE_VALUE_FAIL;
				break;
			case PyUnicode_1BYTE_KIND:
				s_len_pointless = pointless_ucs1_len((uint8_t*)PyUnicode_DATA(py_object));
				break;
			case PyUnicode_2BYTE_KIND:
				s_len_pointless = pointless_ucs2_len((uint16_t*)PyUnicode_DATA(py_object));
				break;
			case PyUnicode_4BYTE_KIND:
				s_len_pointless = pointless_ucs4_len((uint32_t*)PyUnicode_DATA(py_object));
				break;
			default:
				PyErr_SetString(PyExc_ValueError, "unsupported unicode width");
				state->error_line = __LINE__;
				state->is_error = 1;
				return POINTLESS_CREATE_VALUE_FAIL;
		}

		if (s_len_python < 0 || (int64_t)s_len_python != (int64_t)s_len_pointless) {
			PyErr_SetString(PyExc_ValueError, "unicode string contains a zero, where it shouldn't");
			state->error_line = __LINE__;
			state->is_error = 1;
			return POINTLESS_CREATE_VALUE_FAIL;
		}

		void* python_buffer = (void*)PyUnicode_DATA(py_object);

		switch (PyUnicode_KIND(py_object)) {
			case PyUnicode_WCHAR_KIND:
				PyErr_SetString(PyExc_ValueError, "wchar based unicode strings not supported");
				state->error_line = __LINE__;
				state->is_error = 1;
				return POINTLESS_CREATE_VALUE_FAIL;
			case PyUnicode_1BYTE_KIND:
				handle = pointless_create_string_ascii(&state->c, (uint8_t*)python_buffer);
				break;
			case PyUnicode_2BYTE_KIND:
				if (state->unwiden_strings && pointless_is_ucs2_ascii(python_buffer))
					handle = pointless_create_string_ucs2(&state->c, python_buffer);
				else
					handle = pointless_create_unicode_ucs2(&state->c, python_buffer);
				break;
			case PyUnicode_4BYTE_KIND:
				if (state->unwiden_strings && pointless_is_ucs4_ascii((uint32_t*)python_buffer))
					handle = pointless_create_unicode_ucs4(&state->c, (uint32_t*)python_buffer);
				else
					handle = pointless_create_string_ucs4(&state->c, (uint32_t*)python_buffer);
				break;
			// should not happen
			default:
				break;
		}

		RETURN_OOM_IF_FAIL(handle, state);

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

	// dict object
	} else if (PyDict_Check(py_object)) {
		handle = pointless_create_map(&state->c);
		RETURN_OOM_IF_FAIL(handle, state);

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

		PyObject* key = 0;
		PyObject* value = 0;
		Py_ssize_t pos = 0;

		while (PyDict_Next(py_object, &pos, &key, &value)) {
			uint32_t key_handle = pointless_export_py_rec(state, key, depth + 1);
			uint32_t value_handle = pointless_export_py_rec(state, value, depth + 1);

			if (key_handle == POINTLESS_CREATE_VALUE_FAIL || value_handle == POINTLESS_CREATE_VALUE_FAIL)
				break;

			if (pointless_create_map_add(&state->c, handle, key_handle, value_handle) == POINTLESS_CREATE_VALUE_FAIL) {
				PyErr_SetString(PyExc_ValueError, "error adding key/value pair to map");
				state->error_line = __LINE__;
				state->is_error = 1;
				break;
			}
		}

		if (state->is_error) {
			return POINTLESS_CREATE_VALUE_FAIL;
		}

	// set object
	} else if (PySet_Check(py_object) || PyFrozenSet_Check(py_object)) {
		PyObject* iterator = PyObject_GetIter(py_object);
		PyObject* item = 0;

		if (iterator == 0) {
			state->is_error = 1;
			state->error_line = __LINE__;
			return POINTLESS_CREATE_VALUE_FAIL;
		}

		// get a handle
		handle = pointless_create_set(&state->c);
		RETURN_OOM_IF_FAIL(handle, state);

		// cache object
		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

		// iterate over it
		while ((item = PyIter_Next(iterator)) != 0) {
			uint32_t item_handle = pointless_export_py_rec(state, item, depth + 1);

			if (item_handle == POINTLESS_CREATE_VALUE_FAIL)
				break;

			if (pointless_create_set_add(&state->c, handle, item_handle) == POINTLESS_CREATE_VALUE_FAIL) {
				PyErr_SetString(PyExc_ValueError, "error adding item to set");
				state->is_error = 1;
				state->error_line = __LINE__;
				break;
			}
		}

		Py_DECREF(iterator);

		if (PyErr_Occurred()) {
			state->is_error = 1;
			state->error_line = __LINE__;
			return POINTLESS_CREATE_VALUE_FAIL;
		}

	// bitvector
	} else if (PyPointlessBitvector_Check(py_object)) {
		PyPointlessBitvector* bitvector = (PyPointlessBitvector*)py_object;

		if (bitvector->is_pointless) {
			uint32_t i, n_bits = pointless_reader_bitvector_n_bits(&bitvector->pointless_pp->p, bitvector->pointless_v);
			void* bits = pointless_calloc(ICEIL(n_bits, 8), 1);

			if (bits == 0) {
				RETURN_OOM(state);
			}

			for (i = 0; i < n_bits; i++) {
				if (pointless_reader_bitvector_is_set(&bitvector->pointless_pp->p, bitvector->pointless_v, i))
					bm_set_(bits, i);
			}

			if (state->normalize_bitvector)
				handle = pointless_create_bitvector(&state->c, bits, n_bits);
			else
				handle = pointless_create_bitvector_no_normalize(&state->c, bits, n_bits);
			pointless_free(bits);
			bits = 0;

		} else {
			if (state->normalize_bitvector)
				handle = pointless_create_bitvector(&state->c, bitvector->primitive_bits, bitvector->primitive_n_bits);
			else
				handle = pointless_create_bitvector_no_normalize(&state->c, bitvector->primitive_bits, bitvector->primitive_n_bits);
		}

		RETURN_OOM_IF_FAIL(handle, state);

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

	} else if (PyPointlessSet_Check(py_object)) {
		PyPointlessSet* set = (PyPointlessSet*)py_object;
		const char* error = 0;
		handle = pointless_recreate_value(&set->pp->p, set->v, &state->c, &error);

		if (handle == POINTLESS_CREATE_VALUE_FAIL) {
			state->is_error = 1;
			state->error_line = __LINE__;
			PyErr_Format(PyExc_ValueError, "pointless_recreate_value(): %s", error);
			return POINTLESS_CREATE_VALUE_FAIL;
		}

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

	} else if (PyPointlessMap_Check(py_object)) {
		PyPointlessMap* map = (PyPointlessMap*)py_object;
		const char* error = 0;
		handle = pointless_recreate_value(&map->pp->p, map->v, &state->c, &error);

		if (handle == POINTLESS_CREATE_VALUE_FAIL) {
			state->is_error = 1;
			state->error_line = __LINE__;
			PyErr_Format(PyExc_ValueError, "pointless_recreate_value(): %s", error);
			return POINTLESS_CREATE_VALUE_FAIL;
		}

		if (!pointless_export_set_seen(state, py_object, handle)) {
			RETURN_OOM(state);
		}

	// type not supported
	} else {
		PyErr_Format(PyExc_ValueError, "type <%s> not supported", py_object->ob_type->tp_name);
		state->error_line = __LINE__;
		state->is_error = 1;
	}

	#undef RETURN_OOM
	#undef RETURN_IF_OOM

	return handle;
}

static void pointless_export_py(pointless_export_state_t* state, PyObject* py_object)
{
	uint32_t root = pointless_export_py_rec(state, py_object, 0);

	if (root != POINTLESS_CREATE_VALUE_FAIL)
		pointless_create_set_root(&state->c, root);
}

const char pointless_write_object_doc[] =
"0\n"
"pointless.serialize_to_file(object, fname)\n"
"\n"
"Serializes the object to a file.\n"
"\n"
"  object: the object\n"
"  fname:  the file name\n"
;
PyObject* pointless_write_object(PyObject* self, PyObject* args, PyObject* kwds)
{
	const char* fname = 0;
	PyObject* object = 0;
	PyObject* retval = 0;
	PyObject* normalize_bitvector = Py_True;
	PyObject* unwiden_strings = Py_False;
	int create_end = 0;

	const char* error = 0;

	pointless_export_state_t state;
	state.objects_used = 0;
	state.is_error = 0;
	state.error_line = -1;
	state.unwiden_strings = 0;
	state.normalize_bitvector = 1;

	static char* kwargs[] = {"object", "filename", "unwiden_strings", "normalize_bitvector", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "Os|O!O!:serialize", kwargs, &object, &fname, &PyBool_Type, &unwiden_strings, &PyBool_Type, &normalize_bitvector))
		return 0;

	state.unwiden_strings = (unwiden_strings == Py_True);
	state.normalize_bitvector = (normalize_bitvector == Py_True);

	pointless_create_begin_64(&state.c);

	pointless_export_py(&state, object);

	if (state.is_error)
		goto cleanup;

	create_end = 0;

	if (!pointless_create_output_and_end_f(&state.c, fname, &error)) {
		PyErr_Format(PyExc_IOError, "pointless_create_output: %s", error);
		goto cleanup;
	}

	retval = Py_None;

cleanup:

	if (create_end)
		pointless_create_end(&state.c);

	JudyLFreeArray(&state.objects_used, 0);

	Py_XINCREF(retval);
	return retval;
}


const char pointless_write_object_to_buffer_doc[] =
"0\n"
"pointless.serialize_to_buffer(object)\n"
"\n"
"Serializes the object to a buffer.\n"
"\n"
"  object: the object\n"
;

static PyObject* pointless_write_object_to(int buffer_type, PyObject* self, PyObject* args, PyObject* kwds)
{
	PyObject* object = 0;
	PyObject* retval = 0;
	PyObject* normalize_bitvector = Py_True;
	PyObject* unwiden_strings = Py_False;
	int create_end = 0;

	void* buf = 0;
	size_t buflen = 0;

	const char* error = 0;

	pointless_export_state_t state;
	state.objects_used = 0;
	state.is_error = 0;
	state.error_line = -1;
	state.unwiden_strings = 0;
	state.normalize_bitvector = 1;

	static char* kwargs[] = {"object", "unwiden_strings", "normalize_bitvector", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O!O!:serialize", kwargs, &object, &PyBool_Type, &unwiden_strings, &PyBool_Type, &normalize_bitvector))
		return 0;

	state.unwiden_strings = (unwiden_strings == Py_True);
	state.normalize_bitvector = (normalize_bitvector == Py_True);

	pointless_create_begin_64(&state.c);

	pointless_export_py(&state, object);

	if (state.is_error)
		goto cleanup;

	create_end = 0;

	if (!pointless_create_output_and_end_b(&state.c, &buf, &buflen, &error)) {
		PyErr_Format(PyExc_IOError, "pointless_create_output: %s", error);
		goto cleanup;
	}

	if (buffer_type == 0)
		retval = (PyObject*)PyPointlessPrimVector_from_buffer(buf, buflen);
	else
		retval = (PyObject*)PyByteArray_FromStringAndSize(buf, buflen);

cleanup:

	if (create_end)
		pointless_create_end(&state.c);

	JudyLFreeArray(&state.objects_used, 0);

	return retval;
}

PyObject* pointless_write_object_to_primvector(PyObject* self, PyObject* args, PyObject* kwds)
{
	return pointless_write_object_to(0, self, args, kwds);
}

PyObject* pointless_write_object_to_bytearray(PyObject* self, PyObject* args, PyObject* kwds)
{
	return pointless_write_object_to(1, self, args, kwds);
}
