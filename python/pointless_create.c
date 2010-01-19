#include "../pointless.h"

typedef struct {
	pointless_create_t c;   // create-time state
	int is_error;           // true iff error, python exception is also set
	PyObject* objects_used; // set, containing the mapping (PyObject* -> create-time-handle)
} pointless_export_state_t;

static uint32_t pointless_export_get_seen(pointless_export_state_t* state, PyObject* py_object)
{
	PyObject* key_obj = PyLong_FromVoidPtr(py_object);
	PyObject* handle_obj = 0;
	uint32_t handle = POINTLESS_CREATE_VALUE_FAIL;

	if (key_obj == 0) {
		state->is_error = 1;
		goto cleanup;
	}

	handle_obj = PyDict_GetItem(state->objects_used, key_obj);

	if (handle_obj != 0) {
		assert(PyLong_Check(handle_obj));
		handle = (uint32_t)PyLong_AsUnsignedLong(handle_obj);
	}

cleanup:

	Py_XDECREF(key_obj);

	return handle;
}

static int pointless_export_set_seen(pointless_export_state_t* state, PyObject* py_object, uint32_t handle)
{
	PyObject* key_obj = PyLong_FromVoidPtr(py_object);
	PyObject* handle_obj = PyLong_FromUnsignedLong((unsigned long)handle);
	int retval = 0;

	if (key_obj == 0 || handle_obj == 0) {
		state->is_error = 1;
		goto cleanup;
	}

	if (PyDict_SetItem(state->objects_used, key_obj, handle_obj) != 0) {
		state->is_error = 1;
		goto cleanup;
	}

	retval = 1;

cleanup:

	Py_XDECREF(key_obj);
	Py_XDECREF(handle_obj);

	return retval;
}

static uint32_t pointless_export_py_rec(pointless_export_state_t* state, PyObject* py_object, uint32_t depth)
{
	// don't go too deep
	if (depth >= POINTLESS_MAX_DEPTH) {
		PyErr_SetString(PyExc_ValueError, "structure is too deep");
		state->is_error = 1;
		return POINTLESS_CREATE_VALUE_FAIL;
	}

	// check if container object has been seen before
	uint32_t handle = pointless_export_get_seen(state, py_object);

	// lookup exception
	if (state->is_error)
		return POINTLESS_CREATE_VALUE_FAIL;

	// object has been seen before, return its handle
	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	// ...otherwise, create a new handle
	const char* error = 0;

	// list/tuple object
	if (PyList_Check(py_object) || PyTuple_Check(py_object)) {
		handle = pointless_create_vector_value(&state->c);

		if (handle == POINTLESS_CREATE_VALUE_FAIL) {
			PyErr_NoMemory();
			state->is_error = 1;
			return handle;
		}

		// we only do this for container objects
		if (!pointless_export_set_seen(state, py_object, handle)) {
			state->is_error = 1;
			handle = POINTLESS_CREATE_VALUE_FAIL;
			return handle;
		}

		Py_ssize_t i, n_items = PyList_Check(py_object) ? PyList_GET_SIZE(py_object) : PyTuple_GET_SIZE(py_object);

		for (i = 0; i < n_items; i++) {
			PyObject* child = PyList_Check(py_object) ? PyList_GET_ITEM(py_object, i) : PyTuple_GET_ITEM(py_object, i);
			uint32_t child_handle = pointless_export_py_rec(state, child, depth + 1);

			if (child_handle == POINTLESS_CREATE_VALUE_FAIL)
				return child_handle;

			if (pointless_create_vector_value_append(&state->c, handle, child_handle) == POINTLESS_CREATE_VALUE_FAIL) {
				PyErr_Format(PyExc_ValueError, "error appending to vector: out of memory");
				state->is_error = 1;
				handle = POINTLESS_CREATE_VALUE_FAIL;
				return handle;
			}
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
			case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
				handle = pointless_create_vector_float_owner(&state->c, (float*)data, n_items);
				break;
			default:
				state->is_error = 1;
				PyErr_SetString(PyExc_ValueError, "internal error: illegal type for primitive vector");
				return handle;
		}

		if (handle == POINTLESS_CREATE_VALUE_FAIL) {
			PyErr_NoMemory();
			state->is_error = 1;
			return handle;
		}

		// we only do this for container objects
		if (!pointless_export_set_seen(state, py_object, handle)) {
			state->is_error = 1;
			handle = POINTLESS_CREATE_VALUE_FAIL;
			return handle;
		}
	// unicode object
	} else if (PyUnicode_Check(py_object)) {
		// get it from python
		Py_UNICODE* python_buffer = PyUnicode_AS_UNICODE(py_object);

		// create the handle
		if (Py_UNICODE_SIZE != 2 && Py_UNICODE_SIZE != 4) {
			PyErr_Format(PyExc_ValueError, "strange value for Py_UNICODE_SIZE: %u", (unsigned int)Py_UNICODE_SIZE);
			state->is_error = 1;
		} else {
			if (Py_UNICODE_SIZE == 4)
				handle = pointless_create_unicode_ucs4(&state->c, (uint32_t*)python_buffer, &error);
			else
				handle = pointless_create_unicode_ucs2(&state->c, (uint16_t*)python_buffer, &error);

			if (handle == POINTLESS_CREATE_VALUE_FAIL) {
				PyErr_Format(PyExc_ValueError, "error converting unicode to UCS-4: %s", error);
				state->is_error = 1;
			}
		}
	// string object, converts to unicode
	} else if (PyString_Check(py_object)) {
		const char* s = PyString_AS_STRING(py_object);
		handle = pointless_create_unicode_ascii(&state->c, s, &error);

		if (handle == POINTLESS_CREATE_VALUE_FAIL) {
			PyErr_Format(PyExc_ValueError, "error converting (assumed) ascii string to UCS-4: %s", error);
			state->is_error = 1;
		}
	// float object
	} else if (PyFloat_Check(py_object)) {
		handle = pointless_create_float(&state->c, (float)PyFloat_AS_DOUBLE(py_object));

		if (handle == POINTLESS_CREATE_VALUE_FAIL) {
			PyErr_NoMemory();
			state->is_error = 1;
		}
	// booleans, need this above integer check, cause PyInt_Check return 1 for booleans
	} else if (PyBool_Check(py_object)) {
		if (py_object == Py_True)
			handle = pointless_create_boolean_true(&state->c);
		else
			handle = pointless_create_boolean_false(&state->c);

		if (handle == POINTLESS_CREATE_VALUE_FAIL) {
			PyErr_Format(PyExc_ValueError, "error creating boolean value: %s", error);
			state->is_error = 1;
		}

	// integer
	} else if (PyInt_Check(py_object)) {
		long v = PyInt_AS_LONG(py_object);

		if (v >= 0) {
			if (v > UINT32_MAX) {
				PyErr_Format(PyExc_ValueError, "integer too large for mere 32 bits");
				state->is_error = 1;
			} else
				handle = pointless_create_u32(&state->c, (uint32_t)v);
		} else {
			if (!(INT32_MIN <= v && v <= INT32_MAX)) {
				PyErr_Format(PyExc_ValueError, "integer too large for mere 32 bits with a sign");
				state->is_error = 1;
			} else
				handle = pointless_create_i32(&state->c, (int32_t)v);
		}

		if (!state->is_error && handle == POINTLESS_CREATE_VALUE_FAIL) {
			PyErr_NoMemory();
			state->is_error = 1;
		}
	// long
	} else if (PyLong_Check(py_object)) {
		// this will raise an overflow error if number is outside the legal range of PY_LONG_LONG
		PY_LONG_LONG v = PyLong_AsLongLong(py_object);

		// if there was an exception, clear it, and set our own
		if (PyErr_Occurred()) {
			PyErr_Clear();
			PyErr_SetString(PyExc_ValueError, "value of long is way beyond what we can store right now");
			state->is_error = 1;
		// we have the value, see if we can represent it using 32-bits
		// NOTE: same code as above
		// TODO: see if we can re-use it
		} else {
			if (v >= 0) {
				if (v > UINT32_MAX) {
					PyErr_Format(PyExc_ValueError, "long too large for mere 32 bits");
					state->is_error = 1;
				} else
					handle = pointless_create_u32(&state->c, (uint32_t)v);
			} else {
				if (!(INT32_MIN <= v && v <= INT32_MAX)) {
					PyErr_Format(PyExc_ValueError, "long too large for mere 32 bits with a sign");
					state->is_error = 1;
				} else
					handle = pointless_create_i32(&state->c, (int32_t)v);
			}

			if (!state->is_error && handle == POINTLESS_CREATE_VALUE_FAIL) {
				PyErr_NoMemory();
				state->is_error = 1;
			}
		}
	// None object
	} else if (py_object == Py_None) {
		if ((handle = pointless_create_null(&state->c)) == POINTLESS_CREATE_VALUE_FAIL) {
			PyErr_Format(PyExc_ValueError, "error creating NULL value");
			state->is_error = 1;
		}
	// dict object
	} else if (PyDict_Check(py_object)) {
		if ((handle = pointless_create_map(&state->c)) == POINTLESS_CREATE_VALUE_FAIL) {
			PyErr_SetString(PyExc_ValueError, "error creating map");
			state->is_error = 1;
		} else {
			// we only do this for container objects
			if (!pointless_export_set_seen(state, py_object, handle)) {
				state->is_error = 1;
				handle = POINTLESS_CREATE_VALUE_FAIL;
				return handle;
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
					state->is_error = 1;
					break;
				}
			}

			if (state->is_error) {
				handle = POINTLESS_CREATE_VALUE_FAIL;
				return handle;
			}

			if (!pointless_export_set_seen(state, py_object, handle)) {
				state->is_error = 1;
				handle = POINTLESS_CREATE_VALUE_FAIL;
				return handle;
			}
		}
	// set object
	} else if (PyAnySet_Check(py_object)) {
		PyObject* iterator = PyObject_GetIter(py_object);
		PyObject* item = 0;

		if (iterator == 0) {
			state->is_error = 1;
		} else {
			// get a handle
			if ((handle = pointless_create_set(&state->c)) == POINTLESS_CREATE_VALUE_FAIL) {
				PyErr_SetString(PyExc_ValueError, "error creating set");
				state->is_error = 1;
			} else {
				// we only do this for container objects
				if (!pointless_export_set_seen(state, py_object, handle)) {
					state->is_error = 1;
					handle = POINTLESS_CREATE_VALUE_FAIL;
					return handle;
				}

				// iterate over it
				while ((item = PyIter_Next(iterator)) != 0) {
					uint32_t item_handle = pointless_export_py_rec(state, item, depth + 1);

					if (item_handle == POINTLESS_CREATE_VALUE_FAIL)
						break;

					if (pointless_create_set_add(&state->c, handle, item_handle) == POINTLESS_CREATE_VALUE_FAIL) {
						PyErr_SetString(PyExc_ValueError, "error adding item to set");
						state->is_error = 1;
						break;
					}
				}

				Py_DECREF(iterator);

				if (PyErr_Occurred()) {
					state->is_error = 1;
				} else {
					// we only do this for container objects
					if (!pointless_export_set_seen(state, py_object, handle)) {
						state->is_error = 1;
						handle = POINTLESS_CREATE_VALUE_FAIL;
						return handle;
					}
				}
			}
		}

	// bitvector
	} else if (PyPointlessBitvector_Check(py_object)) {
		PyPointlessBitvector* bitvector = (PyPointlessBitvector*)py_object;

		if (bitvector->is_pointless) {
			uint32_t i, n_bits = pointless_reader_bitvector_n_bits(&bitvector->pointless_pp->p, bitvector->pointless_v);
			void* bits = calloc(ICEIL(n_bits, 8), 1);

			if (bits == 0) {
				PyErr_NoMemory();
				state->is_error = 1;
			} else {
				for (i = 0; i < n_bits; i++) {
					if (pointless_reader_bitvector_is_set(&bitvector->pointless_pp->p, bitvector->pointless_v, i))
						bm_set_(bits, i);
				}

				handle = pointless_create_bitvector(&state->c, bits, n_bits);
				free(bits);
				bits = 0;

				if (handle == POINTLESS_CREATE_VALUE_FAIL) {
					PyErr_NoMemory();
					state->is_error = 1;
				}
			}
		} else {
			if ((handle = pointless_create_bitvector(&state->c, bitvector->primitive_bits, bitvector->primitive_n_bits)) == POINTLESS_CREATE_VALUE_FAIL) {
				PyErr_Format(PyExc_ValueError, "error creating bitvector value");
				state->is_error = 1;
			}
		}
	// type not supported
	} else {
		PyErr_Format(PyExc_ValueError, "type <%s> not supported", py_object->ob_type->tp_name);
		state->is_error = 1;
	}

	assert((handle == POINTLESS_CREATE_VALUE_FAIL) == (state->is_error != 0));

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
"pointless.serialize(object, fname)\n"
"\n"
"Serializes the object to a file.\n"
"\n"
"  object: the object\n"
"  fname:  the file name\n"
;
PyObject* pointless_write_object(PyObject* self, PyObject* args)
{
	const char* fname = 0;
	PyObject* object = 0;
	PyObject* retval = 0;
	int create_end = 0;

	const char* error = 0;

	pointless_export_state_t state;
	state.objects_used = 0;
	state.is_error = 0;

	if (!PyArg_ParseTuple(args, "Os:write_object", &object, &fname))
		return 0;

	pointless_create_begin(&state.c);

	state.objects_used = PyDict_New();

	if (state.objects_used == 0)
		goto cleanup;

	pointless_export_py(&state, object);

	if (state.is_error)
		goto cleanup;

	int c = pointless_create_output_and_end_f(&state.c, fname, &error);
	create_end = 0;

	if (!c) {
		PyErr_Format(PyExc_IOError, "pointless_create_output: %s", error);
		goto cleanup;
	}

	retval = Py_None;

cleanup:

	if (create_end)
		pointless_create_end(&state.c);

	Py_XDECREF(state.objects_used);

	Py_XINCREF(retval);
	return retval;
}
