#include "../pointless_ext.h"

PyTypeObject PyPointlessBitvectorType;
PyTypeObject PyPointlessBitvectorIterType;

static void PyPointlessBitvector_dealloc(PyPointlessBitvector* self)
{
	Py_XDECREF(self->pointless_pp);
	self->is_pointless = 0;
	self->pointless_pp = 0;
	self->pointless_v = 0;
	self->primitive_n_bits = 0;
	free(self->primitive_bits);
	self->primitive_bits = 0;
	self->primitive_n_bytes_alloc = 0;
	self->ob_type->tp_free((PyObject*)self);
}

static void PyPointlessBitvectorIter_dealloc(PyPointlessBitvectorIter* self)
{
	Py_XDECREF(self->bitvector);
	self->bitvector = 0;
	self->iter_state = 0;
	self->ob_type->tp_free((PyObject*)self);
}

PyObject* PyPointlessBitvector_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessBitvector* self = (PyPointlessBitvector*)type->tp_alloc(type, 0);

	if (self) {
		self->is_pointless = 0;
		self->pointless_pp = 0;
		self->pointless_v = 0;
		self->primitive_n_bits = 0;
		self->primitive_bits = 0;
		self->primitive_n_bytes_alloc = 0;
	}

	return (PyObject*)self;
}

PyObject* PyPointlessBitvectorIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessBitvectorIter* self = (PyPointlessBitvectorIter*)type->tp_alloc(type, 0);

	if (self) {
		self->bitvector = 0;
		self->iter_state = 0;
	}

	return (PyObject*)self;
}

static int PyPointlessBitvector_init(PyPointlessBitvector* self, PyObject* args, PyObject* kwds)
{
	// cleanup
	self->is_pointless = 0;
	self->allow_print = 1;

	Py_XDECREF(self->pointless_pp);
	self->pointless_pp = 0;
	self->pointless_v = 0;

	free(self->primitive_bits);
	self->primitive_n_bits = 0;
	self->primitive_bits = 0;
	self->primitive_n_bytes_alloc = 0;

	// parse input
	static char* kwargs[] = {"size", "sequence", "allow_print", 0};
	PyObject* size = 0;
	PyObject* sequence = 0;
	PyObject* allow_print = Py_True;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO!", kwargs, &size, &sequence, &PyBool_Type, &allow_print))
		return -1;

	// size xor sequence
	if ((size != 0) && (sequence!= 0)) {
		PyErr_SetString(PyExc_TypeError, "only one of size/sequence can be specified");
		return -1;
	}

	if (allow_print == Py_False)
		self->allow_print = 0;

	// determine number of items
	Py_ssize_t n_items = 0, i;
	int is_error = 0;

	if (size != 0) {
		if (PyInt_Check(size) || PyInt_CheckExact(size)) {
			n_items = (Py_ssize_t)PyInt_AS_LONG(size);
		} else if (PyLong_Check(size) || PyLong_CheckExact(size)) {
			n_items = (Py_ssize_t)PyLong_AsLongLong(size);

			if (PyErr_Occurred())
				return -1;
		} else {
			is_error = 1;
		}

		if (is_error || !(0 <= n_items && n_items <= UINT32_MAX)) {
			PyErr_SetString(PyExc_ValueError, "size must be an integer 0 <= i < 2**32");
			return -1;
		}
	} else if (sequence) {
		if (!PySequence_Check(sequence)) {
			PyErr_SetString(PyExc_ValueError, "sequence must be a sequence");
			return -1;
		}
		n_items = PySequence_Size(sequence);

		if (n_items == -1)
			return -1;
	}

	self->primitive_n_bits = (uint32_t)n_items;
	self->primitive_bits = 0;
	self->primitive_n_bytes_alloc = (uint32_t)ICEIL(n_items, 8);

	if (n_items > 0) {
		self->primitive_bits = calloc(self->primitive_n_bytes_alloc, 1);

		if (self->primitive_bits == 0) {
			self->primitive_n_bytes_alloc = 0;
			PyErr_NoMemory();
			return -1;
		}
	}

	// if we have a sequence
	is_error = 0;

	if (sequence != 0) {
		// for each object
		for (i = 0; i < n_items; i++) {
			PyObject* obj = PySequence_GetItem(sequence, i);

			// 0) GetItem() failure
			if (obj == 0) {
				is_error = 1;
			// 1) boolean
			} else if (PyBool_Check(obj)) {
				if (obj == Py_True)
					bm_set_(self->primitive_bits, i);
			// 2) integer (must be 0 or 1)
			} else if ((PyInt_Check(obj) || PyInt_CheckExact(obj)) && (PyInt_AS_LONG(obj) == 0 || PyInt_AS_LONG(obj) == 1)) {
				if (PyInt_AS_LONG(obj) == 1)
					bm_set_(self->primitive_bits, i);
			// 3) long (must be 0 or 1)
			} else if (PyLong_Check(obj) || PyLong_CheckExact(obj)) {
				PY_LONG_LONG v = PyLong_AsLongLong(obj);

				if (PyErr_Occurred() || (v != 0 && v != 1)) {
					PyErr_Clear();
					is_error = 1;
				} else if (v == 1) {
					bm_set_(self->primitive_bits, i);
				}
			// 4) nothong else allowed
			} else {
				is_error = 1;
			}

			if (is_error) {
				free(self->primitive_bits);
				self->primitive_n_bits = 0;
				self->primitive_bits = 0;
				self->primitive_n_bytes_alloc = 0;

				if (!PyErr_Occurred())
					PyErr_SetString(PyExc_ValueError, "init sequence must only contain True, False, 0 or 1");

				return -1;
			}
		}
	}

	return 0;
}

static int PyPointlessBitvectorIter_init(PyPointlessBitvector* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessBitvectorIter directly");
	return -1;
}

static Py_ssize_t PyPointlessBitvector_length(PyPointlessBitvector* self)
{
	if (self->is_pointless)
		return (Py_ssize_t)pointless_reader_bitvector_n_bits(&self->pointless_pp->p, self->pointless_v);
	else
		return (Py_ssize_t)self->primitive_n_bits;
}

static int PyPointlessBitvector_check_index(PyPointlessBitvector* self, PyObject* item, Py_ssize_t* i)
{
	// if this is not an index: throw an exception
	if (!PyIndex_Check(item)) {
		PyErr_Format(PyExc_TypeError, "BitVector: integer indexes please, got <%s>\n", item->ob_type->tp_name);
		return 0;
	}

	// if index value is not an integer: throw an exception
	*i = PyNumber_AsSsize_t(item, PyExc_IndexError);

	if (*i == -1 && PyErr_Occurred())
		return 0;

	// if index is negative, it is relative to vector end
	if (*i < 0)
		*i += PyPointlessBitvector_length(self);

	// if it is out of bounds: throw an exception
	if (!(0 <= *i && *i < PyPointlessBitvector_length(self))) {
		PyErr_SetString(PyExc_IndexError, "index is out of bounds");
		return 0;
	}

	return 1;
}

static int PyPointlessBitvector_ass_subscript(PyPointlessBitvector* self, PyObject* item, PyObject* value)
{
	if (self->is_pointless) {
		PyErr_SetString(PyExc_ValueError, "this PyPointlessBitvector is read-only");
		return -1;
	}

	// get the index
	Py_ssize_t i;

	if (!PyPointlessBitvector_check_index(self, item, &i))
		return -1;

	// we only want 0|1|True|False
	if (PyBool_Check(value)) {
		if (value == Py_True)
			bm_set_(self->primitive_bits, i);
		else
			bm_reset_(self->primitive_bits, i);

		return 0;	
	}

	if (PyInt_Check(value)) {
		long v = PyInt_AS_LONG(value);

		if (v == 0 || v == 1) {
			if (v == 1)
				bm_set_(self->primitive_bits, i);
			else
				bm_reset_(self->primitive_bits, i);

			return 0;
		}
	}

	PyErr_SetString(PyExc_ValueError, "we only want 0, 1, True or False");
	return -1;
}

static uint32_t PyPointlessBitvector_subscript_priv_int(PyPointlessBitvector* self, uint32_t i)
{
	if (self->is_pointless)
		return pointless_reader_bitvector_is_set(&self->pointless_pp->p, self->pointless_v, i);

	return (bm_is_set_(self->primitive_bits, i) != 0);
}

static PyObject* PyPointlessBitvector_subscript_priv(PyPointlessBitvector* self, uint32_t i)
{
	if (PyPointlessBitvector_subscript_priv_int(self, i))
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}

static PyObject* PyPointlessBitvector_subscript(PyPointlessBitvector* self, PyObject* item)
{
	// get the index
	Py_ssize_t i;

	if (!PyPointlessBitvector_check_index(self, item, &i))
		return 0;

	return PyPointlessBitvector_subscript_priv(self, (uint32_t)i);
}

static PyObject* PyPointlessBitvector_richcompare(PyObject* a, PyObject* b, int op)
{
	if (!PyPointlessBitvector_Check(a) || !PyPointlessBitvector_Check(b)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	uint32_t n_bits_a = (uint32_t)PyPointlessBitvector_length((PyPointlessBitvector*)a);
	uint32_t n_bits_b = (uint32_t)PyPointlessBitvector_length((PyPointlessBitvector*)b);
	uint32_t i = 0, is_set_a = 0, is_set_b = 0, n_bits = (n_bits_a < n_bits_b) ? n_bits_a : n_bits_b;
	long c;

	if (n_bits_a != n_bits_b && (op == Py_EQ || op == Py_NE)) {
		if (op == Py_EQ)
			Py_RETURN_FALSE;
		else
			Py_RETURN_TRUE;
	}

	// first item where they differ
	for (i = 0; i < n_bits; i++) {
		is_set_a = PyPointlessBitvector_subscript_priv_int((PyPointlessBitvector*)a, i);
		is_set_b = PyPointlessBitvector_subscript_priv_int((PyPointlessBitvector*)b, i);

		if (is_set_a != is_set_b)
			break;
	}

	// if all items up to n_bits are the same, just compare on lengths
	if (i >= n_bits_a || i >= n_bits_b) {
		switch (op) {
			case Py_LT: c = (n_bits_a <  n_bits_b); break;
			case Py_LE: c = (n_bits_a <= n_bits_b); break;
			case Py_EQ: c = (n_bits_a == n_bits_b); break;
			case Py_NE: c = (n_bits_a != n_bits_b); break;
			case Py_GT: c = (n_bits_a >  n_bits_b); break;
			case Py_GE: c = (n_bits_a >= n_bits_b); break;
			default: assert(0); return 0;
		}

		return PyBool_FromLong(c);
	}

	// items are different
	if (op == Py_EQ)
		Py_RETURN_FALSE;
	else if (op == Py_NE)
		Py_RETURN_TRUE;

	// similar to above
	switch (op) {
		case Py_LT: c = (is_set_a <  is_set_b); break;
		case Py_LE: c = (is_set_a <= is_set_b); break;
		case Py_GT: c = (is_set_a >  is_set_b); break;
		case Py_GE: c = (is_set_a >= is_set_b); break;
		default: assert(0); return 0;
	}

	return PyBool_FromLong(c);
}

static PyObject* PyPointlessBitvector_iter(PyObject* bitvector)
{
	if (!PyPointlessBitvector_Check(bitvector)) {
		PyErr_BadInternalCall();
		return 0;
	}

	PyPointlessBitvectorIter* iter = PyObject_New(PyPointlessBitvectorIter, &PyPointlessBitvectorIterType);

	if (iter == 0)
		return 0;

	iter = (PyPointlessBitvectorIter*)PyObject_Init((PyObject*)iter, &PyPointlessBitvectorIterType);

	Py_INCREF(bitvector);

	iter->bitvector = (PyPointlessBitvector*)bitvector;
	iter->iter_state = 0;

	return (PyObject*)iter;
}

static uint32_t next_size(uint32_t n_alloc)
{
	size_t small_add[] = {1, 1, 2, 2, 4, 4, 4, 8, 8, 10, 11, 12, 13, 14, 15, 16};
	size_t a = n_alloc / 16;
	size_t b = n_alloc;
	size_t c = (n_alloc < 16) ? small_add[n_alloc] : 0;

	return (a + b + c);
}

static PyObject* PyPointlessBitvector_append(PyPointlessBitvector* self, PyObject* args)
{
	PyObject* v = 0;

	if (!PyArg_ParseTuple(args, "O!", &PyBool_Type, &v))
		return 0;

	if (self->is_pointless) {
		PyErr_SetString(PyExc_ValueError, "BitVector is pointless based, and thus read-only");
		return 0;
	}

	if (self->primitive_n_bits == self->primitive_n_bytes_alloc * 8) {
		uint32_t next_bytes = next_size(self->primitive_n_bytes_alloc);
		void* next_data = realloc(self->primitive_bits, next_bytes);

		if (next_data == 0) {
			PyErr_NoMemory();
			return 0;
		}

		uint32_t i;

		for (i = self->primitive_n_bytes_alloc; i < next_bytes; i++)
			((char*)next_data)[i] = 0;

		self->primitive_n_bytes_alloc = next_bytes;
		self->primitive_bits = next_data;
	}

	if (v == Py_True)
		bm_set_(self->primitive_bits, self->primitive_n_bits);

	self->primitive_n_bits += 1;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef PyPointlessBitvector_methods[] = {
	{"append",      (PyCFunction)PyPointlessBitvector_append,  METH_VARARGS, ""},
	{NULL, NULL}
};

static PyMappingMethods PyPointlessBitvector_as_mapping = {
	(lenfunc)PyPointlessBitvector_length,
	(binaryfunc)PyPointlessBitvector_subscript,
	(objobjargproc)PyPointlessBitvector_ass_subscript
};

PyObject* PyBitvector_repr(PyPointlessBitvector* self)
{
	if (!self->is_pointless && !self->allow_print)
		return PyString_FromFormat("<%s object at %p>", Py_TYPE(self)->tp_name, (void*)self);

	return PyPointless_repr((PyObject*)self);
}

PyObject* PyBitvector_str(PyPointlessBitvector* self)
{
	if (!self->is_pointless && !self->allow_print)
		return PyString_FromFormat("<%s object at %p>", Py_TYPE(self)->tp_name, (void*)self);

	return PyPointless_str((PyObject*)self);
}


PyTypeObject PyPointlessBitvectorType = {
	PyObject_HEAD_INIT(NULL)
	0,                                        /*ob_size*/
	"pointless.PyPointlessBitvector",         /*tp_name*/
	sizeof(PyPointlessBitvector),             /*tp_basicsize*/
	0,                                        /*tp_itemsize*/
	(destructor)PyPointlessBitvector_dealloc, /*tp_dealloc*/
	0,                                        /*tp_print*/
	0,                                        /*tp_getattr*/
	0,                                        /*tp_setattr*/
	0,                                        /*tp_compare*/
	(reprfunc)PyBitvector_repr,               /*tp_repr*/
	0,                                        /*tp_as_number*/
	0,                                        /*tp_as_sequence*/
	&PyPointlessBitvector_as_mapping,         /*tp_as_mapping*/
	0,                                        /*tp_hash */
	0,                                        /*tp_call*/
	(reprfunc)PyBitvector_str,                /*tp_str*/
	0,                                        /*tp_getattro*/
	0,                                        /*tp_setattro*/
	0,                                        /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
	"PyPointlessBitvector wrapper",           /*tp_doc */
	0,                                        /*tp_traverse */
	0,                                        /*tp_clear */
	PyPointlessBitvector_richcompare,         /*tp_richcompare */
	0,                                        /*tp_weaklistoffset */
	PyPointlessBitvector_iter,                /*tp_iter */
	0,                                        /*tp_iternext */
	PyPointlessBitvector_methods,             /*tp_methods */
	0,                                        /*tp_members */
	0,                                        /*tp_getset */
	0,                                        /*tp_base */
	0,                                        /*tp_dict */
	0,                                        /*tp_descr_get */
	0,                                        /*tp_descr_set */
	0,                                        /*tp_dictoffset */
	(initproc)PyPointlessBitvector_init,      /*tp_init */
	0,                                        /*tp_alloc */
	PyPointlessBitvector_new,                 /*tp_new */
};

static PyObject* PyPointlessBitvectorIter_iternext(PyPointlessBitvectorIter* iter)
{
	// iterartor already reached end
	if (iter->bitvector == 0)
		return 0;

	// see if we have any bits left
	uint32_t n_bits = (uint32_t)PyPointlessBitvector_length(iter->bitvector);

	if (iter->iter_state < n_bits) {
		PyObject* bit = PyPointlessBitvector_subscript_priv(iter->bitvector, iter->iter_state);
		iter->iter_state += 1;
		return bit;
	}

	// we reached end
	Py_DECREF(iter->bitvector);
	iter->bitvector = 0;
	return 0;
}


PyTypeObject PyPointlessBitvectorIterType = {
	PyObject_HEAD_INIT(NULL)
	0,                                               /*ob_size*/
	"pointless.PyPointlessBitvectorIter",            /*tp_name*/
	sizeof(PyPointlessBitvectorIter),                /*tp_basicsize*/
	0,                                               /*tp_itemsize*/
	(destructor)PyPointlessBitvectorIter_dealloc,    /*tp_dealloc*/
	0,                                               /*tp_print*/
	0,                                               /*tp_getattr*/
	0,                                               /*tp_setattr*/
	0,                                               /*tp_compare*/
	0,                                               /*tp_repr*/
	0,                                               /*tp_as_number*/
	0,                                               /*tp_as_sequence*/
	0,                                               /*tp_as_mapping*/
	0,                                               /*tp_hash */
	0,                                               /*tp_call*/
	0,                                               /*tp_str*/
	0,                                               /*tp_getattro*/
	0,                                               /*tp_setattro*/
	0,                                               /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                              /*tp_flags*/
	"PyPointlessBitvectorIter",                      /*tp_doc */
	0,                                               /*tp_traverse */
	0,                                               /*tp_clear */
	0,                                               /*tp_richcompare */
	0,                                               /*tp_weaklistoffset */
	PyObject_SelfIter,                               /*tp_iter */
	(iternextfunc)PyPointlessBitvectorIter_iternext, /*tp_iternext */
	0,                                               /*tp_methods */
	0,                                               /*tp_members */
	0,                                               /*tp_getset */
	0,                                               /*tp_base */
	0,                                               /*tp_dict */
	0,                                               /*tp_descr_get */
	0,                                               /*tp_descr_set */
	0,                                               /*tp_dictoffset */
	(initproc)PyPointlessBitvectorIter_init,         /*tp_init */
	0,                                               /*tp_alloc */
	PyPointlessBitvectorIter_new,                    /*tp_new */
};

PyPointlessBitvector* PyPointlessBitvector_New(PyPointless* pp, pointless_value_t* v)
{
	PyPointlessBitvector* pv = PyObject_New(PyPointlessBitvector, &PyPointlessBitvectorType);

	if (pv == 0)
		return 0;

	pv = (PyPointlessBitvector*)PyObject_Init((PyObject*)pv, &PyPointlessBitvectorType);

	Py_INCREF(pp);

	pv->is_pointless = 1;
	pv->pointless_pp = pp;
	pv->pointless_v = v;
	pv->primitive_n_bits = 0;
	pv->primitive_bits = 0;

	return pv;
}

uint32_t pointless_pybitvector_hash(PyPointlessBitvector* bitvector)
{
	if (bitvector->is_pointless) {
		void* buffer = 0;

		if (bitvector->pointless_v->type == POINTLESS_BITVECTOR)
			buffer = pointless_reader_bitvector_buffer(&bitvector->pointless_pp->p, bitvector->pointless_v);

		return pointless_bitvector_hash(bitvector->pointless_v->type, &bitvector->pointless_v->data, buffer);
	}

	uint32_t n_bits = bitvector->primitive_n_bits;
	void* bits = bitvector->primitive_bits;

	return pointless_bitvector_hash_n_bits_bits(n_bits, bits);
}
