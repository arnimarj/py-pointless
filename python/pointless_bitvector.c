#include "../pointless_ext.h"

PyTypeObject PyPointlessBitvectorType;
PyTypeObject PyPointlessBitvectorIterType;

static int PyPointlessBitvector_extend_by(PyPointlessBitvector* self, uint32_t n, int is_true);

static void PyPointlessBitvector_dealloc(PyPointlessBitvector* self)
{
	if (self->is_pointless && self->pointless_pp)
		self->pointless_pp->n_bitvector_refs -= 1;

	Py_XDECREF(self->pointless_pp);
	self->is_pointless = 0;
	self->pointless_pp = 0;
	self->pointless_v = 0;
	self->primitive_n_bits = 0;
	pointless_free(self->primitive_bits);
	self->primitive_bits = 0;
	self->primitive_n_bytes_alloc = 0;
	self->primitive_n_one = 0;
	Py_TYPE(self)->tp_free(self);
}

static void PyPointlessBitvectorIter_dealloc(PyPointlessBitvectorIter* self)
{
	Py_XDECREF(self->bitvector);
	self->bitvector = 0;
	self->iter_state = 0;
	Py_TYPE(self)->tp_free(self);
}

PyObject* PyPointlessBitvector_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessBitvector* self = (PyPointlessBitvector*)type->tp_alloc(type, 0);

	if (self) {
		self->is_pointless = 0;
		self->pointless_pp = 0;
		self->pointless_v = 0;
		self->primitive_n_bytes_alloc = 0;
		self->primitive_n_bits = 0;
		self->primitive_bits = 0;
		self->primitive_n_one = 0;
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

	if (self->pointless_pp) {
		self->pointless_pp->n_bitvector_refs -= 1;
		Py_DECREF(self->pointless_pp);
	}

	self->pointless_pp = 0;
	self->pointless_v = 0;

	pointless_free(self->primitive_bits);
	self->primitive_n_bits = 0;
	self->primitive_bits = 0;
	self->primitive_n_bytes_alloc = 0;
	self->primitive_n_one = 0;

	// parse input
	static char* kwargs[] = {"size", "sequence", "allow_print", 0};
	PyObject* size = 0;
	PyObject* sequence = 0;
	PyObject* sequence_iterator = 0;
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
	Py_ssize_t n_items = 0;
	int is_error = 0;

	if (size != 0) {
		if (PyLong_Check(size) || PyLong_CheckExact(size)) {
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
	} else if (sequence != 0) {
		sequence_iterator = PyObject_GetIter(sequence);

		if (sequence_iterator == 0) {
			PyErr_SetString(PyExc_ValueError, "sequence must be iterable");
			return -1;
		}

		n_items = 0;
	}

	self->primitive_n_bits = (uint32_t)n_items;
	self->primitive_bits = 0;
	self->primitive_n_bytes_alloc = (uint32_t)ICEIL(n_items, 8);

	if (n_items > 0) {
		self->primitive_bits = pointless_calloc(self->primitive_n_bytes_alloc, 1);

		if (self->primitive_bits == 0) {
			self->primitive_n_bytes_alloc = 0;
			Py_XDECREF(sequence_iterator);
			PyErr_NoMemory();
			return -1;
		}
	}

	// if we have a sequence
	is_error = 0;

	if (sequence_iterator != 0) {
		// for each object
		PyObject* obj = 0;
		size_t i = 0;

		while ((obj = PyIter_Next(sequence_iterator)) != 0) {
			if (!PyPointlessBitvector_extend_by(self, 1, 0)) {
				is_error = 1;
			// 0) PyIter_Next() failure
			} else if (obj == 0) {
				is_error = 1;
			// 1) boolean
			} else if (PyBool_Check(obj)) {
				if (obj == Py_True) {
					bm_set_(self->primitive_bits, i);
					self->primitive_n_one += 1;
				}
			// 3) long (must be 0 or 1)
			} else if (PyLong_Check(obj) || PyLong_CheckExact(obj)) {
				PY_LONG_LONG v = PyLong_AsLongLong(obj);

				if (PyErr_Occurred() || (v != 0 && v != 1)) {
					PyErr_Clear();
					is_error = 1;
				} else if (v == 1) {
					bm_set_(self->primitive_bits, i);
					self->primitive_n_one += 1;
				}
			// 4) nothong else allowed
			} else {
				is_error = 1;
			}

			if (is_error) {
				pointless_free(self->primitive_bits);
				self->primitive_n_bits = 0;
				self->primitive_bits = 0;
				self->primitive_n_bytes_alloc = 0;
				self->primitive_n_one = 0;

				if (!PyErr_Occurred())
					PyErr_SetString(PyExc_ValueError, "sequence must only contain True, False, 0 or 1");

				return -1;
			}

			i += 1;
		}
	}

	return 0;
}

static int PyPointlessBitvectorIter_init(PyPointlessBitvector* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessBitvectorIter directly");
	return -1;
}

uint32_t PyPointlessBitvector_n_items(PyPointlessBitvector* self)
{
	if (self->is_pointless)
		return pointless_reader_bitvector_n_bits(&self->pointless_pp->p, self->pointless_v);
	else
		return self->primitive_n_bits;
}

static Py_ssize_t PyPointlessBitvector_length(PyPointlessBitvector* self)
{
	return (Py_ssize_t)PyPointlessBitvector_n_items(self);
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

uint32_t PyPointlessBitvector_is_set(PyPointlessBitvector* self, uint32_t i)
{
	if (self->is_pointless)																																																																																																																																																																																																																																																																																																																																																																																																																																							
		i = pointless_reader_bitvector_is_set(&self->pointless_pp->p, self->pointless_v, i);																																								
	else
		i = (bm_is_set_(self->primitive_bits, i) != 0);

	return i;																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																				
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

	// true iff: the value was set
	uint32_t was_set = PyPointlessBitvector_is_set(self, i);
	int i_value = -1;

	// we only want 0|1|True|PyPointlessBitvector_extend_false
	if (PyBool_Check(value)) {
		if (value == Py_True)
			i_value = 1;
		else
			i_value = 0;
	}

	if (PyLong_Check(value)) {
		long long v = PyLong_AsLongLong(value);

		if (PyErr_Occurred())
			return -1;

		if (v == 0)
			i_value = 0;
		else if (v == 1)
			i_value = 1;
	}

	if (i_value == 0) {
		bm_reset_(self->primitive_bits, i);

		if (was_set)
			self->primitive_n_one -= 1;

		return 0;
	} else if (i_value == 1) {
		bm_set_(self->primitive_bits, i);

		if (!was_set)
			self->primitive_n_one += 1;

		return 0;
	}

	PyErr_SetString(PyExc_ValueError, "we only want 0, 1, True or False");
	return -1;
}

static PyObject* PyPointlessBitvector_subscript_priv(PyPointlessBitvector* self, uint32_t i)
{
	if (PyPointlessBitvector_is_set(self, i))
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
		is_set_a = PyPointlessBitvector_is_set((PyPointlessBitvector*)a, i);
		is_set_b = PyPointlessBitvector_is_set((PyPointlessBitvector*)b, i);

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

static PyObject* PyPointlessBitvector_n_zero_prefix(PyPointlessBitvector* self)
{
	size_t n_zero = 0, n;

	if (self->is_pointless) {
		n = pointless_reader_bitvector_n_bits(&self->pointless_pp->p, self->pointless_v);

		for (; n_zero < n; n_zero++) {
			if (pointless_reader_bitvector_is_set(&self->pointless_pp->p, self->pointless_v, n_zero))
				break;
		}
	} else {
		n = self->primitive_n_bits;

		for (; n_zero < n; n_zero++) {
			if (bm_is_set_(self->primitive_bits, n_zero))
				break;
		}
	}

	return PyLong_FromSize_t(n_zero);
}


static PyObject* PyPointlessBitvector_n_zero_postfix(PyPointlessBitvector* self)
{
	size_t n_zero = 0, n;

	if (self->is_pointless) {
		n = pointless_reader_bitvector_n_bits(&self->pointless_pp->p, self->pointless_v);

		for (; n_zero < n; n_zero++) {
			if (pointless_reader_bitvector_is_set(&self->pointless_pp->p, self->pointless_v, n - 1 - n_zero))
				break;
		}
	} else {
		n = self->primitive_n_bits;

		for (; n_zero < n; n_zero++) {
			if (bm_is_set_(self->primitive_bits, n - 1 - n_zero))
				break;
		}
	}

	return PyLong_FromSize_t(n_zero);
}


static PyObject* PyPointlessBitvector_n_one_prefix(PyPointlessBitvector* self)
{
	size_t n_one = 0, n;

	if (self->is_pointless) {
		n = pointless_reader_bitvector_n_bits(&self->pointless_pp->p, self->pointless_v);

		for (; n_one < n; n_one++) {
			if (!pointless_reader_bitvector_is_set(&self->pointless_pp->p, self->pointless_v, n_one))
				break;
		}
	} else {
		n = self->primitive_n_bits;

		for (; n_one < n; n_one++) {
			if (!bm_is_set_(self->primitive_bits, n_one))
				break;
		}
	}

	return PyLong_FromSize_t(n_one);
}


static PyObject* PyPointlessBitvector_n_one_postfix(PyPointlessBitvector* self)
{
	size_t n_one = 0, n;

	if (self->is_pointless) {
		n = pointless_reader_bitvector_n_bits(&self->pointless_pp->p, self->pointless_v);

		for (; n_one < n; n_one++) {
			if (!pointless_reader_bitvector_is_set(&self->pointless_pp->p, self->pointless_v, n - n_one - 1))
				break;
		}
	} else {
		n = self->primitive_n_bits;

		for (; n_one < n; n_one++) {
			if (!bm_is_set_(self->primitive_bits, n - n_one - 1))
				break;
		}
	}

	return PyLong_FromSize_t(n_one);
}

static PyObject* PyPointlessBitvector_is_any_set(PyPointlessBitvector* self)
{
	if (!self->is_pointless) {
		if (self->primitive_n_one > 0) {
			Py_RETURN_TRUE;
		} else {
			Py_RETURN_FALSE;
		}
	}

	void* bits = 0;

	if (self->pointless_v->type == POINTLESS_BITVECTOR)
		bits = pointless_reader_bitvector_buffer(&self->pointless_pp->p, self->pointless_v);

	if (pointless_bitvector_is_any_set(self->pointless_v->type, &self->pointless_v->data, bits)) {
		Py_RETURN_TRUE;
	} else {
		Py_RETURN_FALSE;
	}
}

static int PyPointlessBitvector_extend_by(PyPointlessBitvector* self, uint32_t n, int is_true)
{
	// if extend would make us grow beyond UINT32_MAX
	uint32_t next_primitive_n_bits = self->primitive_n_bits + n;

	if (next_primitive_n_bits < self->primitive_n_bits || next_primitive_n_bits < n) {
		PyErr_SetString(PyExc_ValueError, "BitVector would grow beyond 2**32-1 items");
		return 0;
	}

	uint32_t next_bytes = self->primitive_n_bytes_alloc;

	while (next_bytes < ICEIL(next_primitive_n_bits, 8)) {
		next_bytes = next_size(next_bytes);

		// overflow
		if (next_bytes < self->primitive_n_bytes_alloc) {
			next_bytes = ICEIL(UINT32_MAX, 8);
			assert(next_bytes * 8 >= next_primitive_n_bits);
		}
	}

	if (next_bytes != self->primitive_n_bytes_alloc) {
		void* next_data = pointless_realloc(self->primitive_bits, next_bytes);

		if (next_data == 0) {
			PyErr_NoMemory();
			return 0;
		}

		uint64_t i;

		for (i = self->primitive_n_bytes_alloc; i < next_bytes; i++)
			((char*)next_data)[i] = 0;

		self->primitive_n_bytes_alloc = next_bytes;
		self->primitive_bits = next_data;
	}

	uint32_t i;

	for (i = 0; i < n; i++) {
		if (is_true) {
			bm_set_(self->primitive_bits, self->primitive_n_bits + i);
			self->primitive_n_one += 1;
		} else { 
			bm_reset_(self->primitive_bits, self->primitive_n_bits + i);
		}
	}

	self->primitive_n_bits += n;
	return 1;
}


static PyObject* PyPointlessBitvector_extend_(PyPointlessBitvector* self, PyObject* args, int is_true)
{
	int n = 0;

	if (!PyArg_ParseTuple(args, "i", &n))
		return 0;

	if (self->is_pointless) {
		PyErr_SetString(PyExc_ValueError, "BitVector is pointless based, and thus read-only");
		return 0;
	}

	if (n < 0) {
		PyErr_SetString(PyExc_ValueError, "negative value");
		return 0;
	}

	// overflow
	if (n >= 0 && (uint64_t)n > UINT32_MAX) {
		PyErr_SetString(PyExc_ValueError, "resulting bitvector would be too large");
		return 0;
	}

	// extend by n
	if (!PyPointlessBitvector_extend_by(self, (uint32_t)n, is_true))
		return 0;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyPointlessBitvector_extend_true(PyPointlessBitvector* self, PyObject* args)
{
	return PyPointlessBitvector_extend_(self, args, 1);
}

static PyObject* PyPointlessBitvector_extend_false(PyPointlessBitvector* self, PyObject* args)
{
	return PyPointlessBitvector_extend_(self, args, 0);
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

	// extend by 1
	if (!PyPointlessBitvector_extend_by(self, 1, v == Py_True))
		return 0;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyPointlessBitvector_pop(PyPointlessBitvector* self)
{
	if (self->is_pointless) {
		PyErr_SetString(PyExc_ValueError, "BitVector is pointless based, and thus read-only");
		return 0;
	}

	if (self->primitive_n_bits == 0) {
		PyErr_SetString(PyExc_IndexError, "pop from empty vector");
		return 0;
	}

	int is_set = (bm_is_set_(self->primitive_bits, self->primitive_n_bits - 1) != 0);

	self->primitive_n_bits -= 1;

	if (is_set) {
		Py_RETURN_TRUE;
	} else {
		Py_RETURN_FALSE;
	}
}

static PyObject* PyPointlessBitvector_copy(PyPointlessBitvector* self)
{
	void* bits = 0;
	uint32_t n_bits = 0, i;
	size_t n_bytes = 0;

	if (self->is_pointless)
		n_bits = pointless_reader_bitvector_n_bits(&self->pointless_pp->p, self->pointless_v);
	else
		n_bits = self->primitive_n_bits;

	n_bytes = ICEIL(n_bits, 8);
	bits = pointless_calloc(n_bytes, 1);

	if (bits == 0) {
		PyErr_NoMemory();
		return 0;
	}

	if (self->is_pointless) {
		if (self->pointless_v->type == POINTLESS_BITVECTOR) {
			void* source_bits = (void*)((uint32_t*)pointless_reader_bitvector_buffer(&self->pointless_pp->p, self->pointless_v) + 1);
			memcpy(bits, source_bits, n_bytes);
		} else {
			for (i = 0; i < n_bits; i++) {
				if (pointless_reader_bitvector_is_set(&self->pointless_pp->p, self->pointless_v, i))
					bm_set_(bits, i);
			}
		}
	} else {
		memcpy(bits, self->primitive_bits, n_bytes);
	}

	PyPointlessBitvector* pv = PyObject_New(PyPointlessBitvector, &PyPointlessBitvectorType);

	if (pv == 0) {
		pointless_free(bits);
		return 0;
	}

	pv->is_pointless = 0;
	pv->pointless_pp = 0;
	pv->pointless_v = 0;
	pv->primitive_n_bytes_alloc = n_bytes;
	pv->primitive_n_bits = n_bits;
	pv->primitive_bits = bits;
	pv->primitive_n_one = 0;

	for (i = 0; i < n_bits; i++) {
		if (bm_is_set_(bits, i))
			pv->primitive_n_one += 1;
	}

	return (PyObject*)pv;
}

static PyObject* PyPointlessBitvector_sizeof(PyPointlessBitvector* self)
{
	size_t s = sizeof(PyPointlessBitvector);

	if (!self->is_pointless)
		s += self->primitive_n_bytes_alloc;

	return PyLong_FromSize_t(s);
}

static PyMethodDef PyPointlessBitvector_methods[] = {
	{"NumZeroPrefix", (PyCFunction)PyPointlessBitvector_n_zero_prefix,  METH_NOARGS,  ""},
	{"NumZeroPostfix",(PyCFunction)PyPointlessBitvector_n_zero_postfix, METH_NOARGS,  ""},
	{"NumOnePrefix",  (PyCFunction)PyPointlessBitvector_n_one_prefix,   METH_NOARGS,  ""},
	{"NumOnePostfix", (PyCFunction)PyPointlessBitvector_n_one_postfix,  METH_NOARGS,  ""},
	{"IsAnySet",      (PyCFunction)PyPointlessBitvector_is_any_set,     METH_NOARGS,  ""},
	{"append",        (PyCFunction)PyPointlessBitvector_append,         METH_VARARGS, ""},
	{"extend_false",  (PyCFunction)PyPointlessBitvector_extend_false,   METH_VARARGS, ""},
	{"extend_true",   (PyCFunction)PyPointlessBitvector_extend_true,    METH_VARARGS, ""},
	{"pop",           (PyCFunction)PyPointlessBitvector_pop,            METH_NOARGS,  ""},
	{"copy",          (PyCFunction)PyPointlessBitvector_copy,           METH_NOARGS,  ""},
	{"__sizeof__",    (PyCFunction)PyPointlessBitvector_sizeof,         METH_NOARGS,   ""},
	{NULL, NULL}
};

static PyMappingMethods PyPointlessBitvector_as_mapping = {
	(lenfunc)PyPointlessBitvector_length,
	(binaryfunc)PyPointlessBitvector_subscript,
	(objobjargproc)PyPointlessBitvector_ass_subscript
};

static long PyPointlessBitVector_hash(PyPointlessBitvector* self)
{
	// this is a Python hash, so we go for the 64-bit version
	if (self->is_pointless) {
		void* buffer = 0;

		if (self->pointless_v->type == POINTLESS_BITVECTOR)
			buffer = pointless_reader_bitvector_buffer(&self->pointless_pp->p, self->pointless_v);

		return (long)pointless_bitvector_hash_64(self->pointless_v->type, &self->pointless_v->data, buffer);
	}

	uint32_t n_bits = self->primitive_n_bits;
	void* bits = self->primitive_bits;

	return (long)pointless_bitvector_hash_n_bits_bits_64(n_bits, bits);
}

PyObject* PyBitvector_repr(PyPointlessBitvector* self)
{
	if (!self->is_pointless && !self->allow_print) {
		return PyUnicode_FromFormat("<%s object at %p>", Py_TYPE(self)->tp_name, (void*)self);
	}

	return PyPointless_repr((PyObject*)self);
}

PyObject* PyBitvector_str(PyPointlessBitvector* self)
{
	if (!self->is_pointless && !self->allow_print) {
		return PyUnicode_FromFormat("<%s object at %p>", Py_TYPE(self)->tp_name, (void*)self);
	}

	return PyPointless_str((PyObject*)self);
}

PyTypeObject PyPointlessBitvectorType = {
	PyVarObject_HEAD_INIT(NULL, 0)
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
	(hashfunc)PyPointlessBitVector_hash,      /*tp_hash */
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
	PyVarObject_HEAD_INIT(NULL, 0)
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

	Py_INCREF(pp);

	pp->n_bitvector_refs += 1;

	pv->is_pointless = 1;
	pv->pointless_pp = pp;
	pv->pointless_v = v;
	pv->primitive_n_bits = 0;
	pv->primitive_bits = 0;

	return pv;
}

uint32_t pointless_pybitvector_hash_32(PyPointlessBitvector* bitvector)
{
	if (bitvector->is_pointless) {
		void* buffer = 0;

		if (bitvector->pointless_v->type == POINTLESS_BITVECTOR)
			buffer = pointless_reader_bitvector_buffer(&bitvector->pointless_pp->p, bitvector->pointless_v);

		return pointless_bitvector_hash_32(bitvector->pointless_v->type, &bitvector->pointless_v->data, buffer);
	}

	uint32_t n_bits = bitvector->primitive_n_bits;
	void* bits = bitvector->primitive_bits;

	return pointless_bitvector_hash_n_bits_bits_32(n_bits, bits);
}

