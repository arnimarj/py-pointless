#include "../pointless_ext.h"

static void PyPointlessVector_dealloc(PyPointlessVector* self)
{
	if (self->pp) {
		self->pp->n_vector_refs -= 1;
		Py_DECREF(self->pp);
	}

	self->pp = 0;
	self->v = 0;
	self->container_id = 0;
	self->is_hashable = 0;
	self->slice_i = 0;
	self->slice_n = 0;
	Py_TYPE(self)->tp_free(self);
}

static void PyPointlessVectorIter_dealloc(PyPointlessVectorIter* self)
{
	PyObject_GC_UnTrack(self);
	Py_XDECREF(self->vector);
	self->vector = 0;
	self->iter_state = 0;
	PyObject_GC_Del(self);
}

static int PyPointlessVectorIter_traverse(PyPointlessVectorIter* self, visitproc visit, void* arg)
{
	Py_VISIT(self->vector);
	return 0;
}

static void PyPointlessVectorRevIter_dealloc(PyPointlessVectorRevIter* self)
{
	PyObject_GC_UnTrack(self);
	Py_XDECREF(self->vector);
	self->vector = 0;
	self->iter_state = 0;
	PyObject_GC_Del(self);
}

static int PyPointlessVectorRevIter_traverse(PyPointlessVectorRevIter* self, visitproc visit, void* arg)
{
	Py_VISIT(self->vector);
	return 0;
}

PyObject* PyPointlessVector_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessVector* self = (PyPointlessVector*)type->tp_alloc(type, 0);

	if (self) {
		self->pp = 0;
		self->v = 0;
		self->container_id = 0;
		self->is_hashable = 0;
		self->slice_i = 0;
		self->slice_n = 0;
	}

	return (PyObject*)self;
}

PyObject* PyPointlessVectorIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessVectorIter* self = (PyPointlessVectorIter*)type->tp_alloc(type, 0);

	if (self) {
		self->vector = 0;
		self->iter_state = 0;
	}

	return (PyObject*)self;
}

PyObject* PyPointlessVectorRevIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessVectorRevIter* self = (PyPointlessVectorRevIter*)type->tp_alloc(type, 0);

	if (self) {
		self->vector = 0;
		self->iter_state = 0;
	}

	return (PyObject*)self;
}

static int PyPointlessVector_init(PyPointlessVector* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessVector directly");
	return -1;
}

static int PyPointlessVectorIter_init(PyPointlessVector* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessVectorIter directly");
	return -1;
}

static int PyPointlessVectorRevIter_init(PyPointlessVector* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessVectorRevIter directly");
	return -1;
}

static Py_ssize_t PyPointlessVector_length(PyPointlessVector* self)
{
	return (Py_ssize_t)self->slice_n;
}

static int PyPointlessVector_check_index(PyPointlessVector* self, PyObject* item, Py_ssize_t* i)
{
	// if this is not an index: throw an exception
	if (!PyIndex_Check(item)) {
		PyErr_Format(PyExc_TypeError, "PointlessVector: integer indexes please, got <%s>\n", item->ob_type->tp_name);
		return 0;
	}

	// if index value is not an integer: throw an exception
	*i = PyNumber_AsSsize_t(item, PyExc_IndexError);

	if (*i == -1 && PyErr_Occurred())
		return 0;

	// if index is negative, it is relative to vector end
	if (*i < 0)
		*i += PyPointlessVector_length(self);

	// if it is out of bounds: throw an exception
	if (!(0 <= *i && *i < PyPointlessVector_length(self))) {
		PyErr_SetString(PyExc_IndexError, "index is out of bounds");
		return 0;
	}

	return 1;
}

static PyObject* PyPointlessVector_subscript_priv(PyPointlessVector* self, uint32_t i)
{
	i += self->slice_i;

	switch (self->v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			return pypointless_value(self->pp, pointless_reader_vector_value(&self->pp->p, self->v) + i);
		case POINTLESS_VECTOR_I8:
			return pypointless_i32(self->pp, (int32_t)pointless_reader_vector_i8(&self->pp->p, self->v)[i]);
		case POINTLESS_VECTOR_U8:
			return pypointless_u32(self->pp, (uint32_t)pointless_reader_vector_u8(&self->pp->p, self->v)[i]);
		case POINTLESS_VECTOR_I16:
			return pypointless_i32(self->pp, (int32_t)pointless_reader_vector_i16(&self->pp->p, self->v)[i]);
		case POINTLESS_VECTOR_U16:
			return pypointless_u32(self->pp, (uint32_t)pointless_reader_vector_u16(&self->pp->p, self->v)[i]);
		case POINTLESS_VECTOR_I32:
			return pypointless_i32(self->pp, pointless_reader_vector_i32(&self->pp->p, self->v)[i]);
		case POINTLESS_VECTOR_U32:
			return pypointless_u32(self->pp, pointless_reader_vector_u32(&self->pp->p, self->v)[i]);
		case POINTLESS_VECTOR_I64:
			return pypointless_i64(self->pp, pointless_reader_vector_i64(&self->pp->p, self->v)[i]);
		case POINTLESS_VECTOR_U64:
			return pypointless_u64(self->pp, pointless_reader_vector_u64(&self->pp->p, self->v)[i]);
		case POINTLESS_VECTOR_FLOAT:
			return pypointless_float(self->pp, pointless_reader_vector_float(&self->pp->p, self->v)[i]);
	}

	PyErr_Format(PyExc_TypeError, "strange array type");
	return 0;
}

static PyObject* PyPointlessVector_slice(PyPointlessVector* self, Py_ssize_t ilow, Py_ssize_t ihigh);

static PyObject* PyPointlessVector_subscript(PyPointlessVector* self, PyObject* item)
{
	// get the index
	Py_ssize_t i;

	if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength;

		if (PySlice_GetIndicesEx(item, (Py_ssize_t)self->slice_n, &start, &stop, &step, &slicelength) == -1)
			return 0;

		if (step != 1) {
			PyErr_SetString(PyExc_ValueError, "only slice-steps of 1 supported");
			return 0;
		}

		return PyPointlessVector_slice(self, start, stop);
	}

	if (!PyPointlessVector_check_index(self, item, &i))
		return 0;

	return PyPointlessVector_subscript_priv(self, (uint32_t)i);
}

static PyObject* PyPointlessVector_item(PyPointlessVector* self, Py_ssize_t i)
{
	if (!(0 <= i && i < self->slice_n)) {
		PyErr_SetString(PyExc_IndexError, "vector index out of range");
		return 0;
	}

	return PyPointlessVector_subscript_priv(self, (uint32_t)i);
}

static int PyPointlessVector_contains(PyPointlessVector* self, PyObject* b)
{
	uint32_t i, c;
	pointless_complete_value_t v;
	const char* error = 0;

	for (i = 0; i < self->slice_n; i++) {
		v = pointless_reader_vector_value_case(&self->pp->p, self->v, i + self->slice_i);
		c = pypointless_cmp_eq(&self->pp->p, &v, b, &error);

		if (error) {
			PyErr_Format(PyExc_ValueError, "comparison error: %s", error);
			return -1;
		}

		if (c)
			return 1;
	}

	return 0;
}

static PyObject* PyPointlessVector_slice(PyPointlessVector* self, Py_ssize_t ilow, Py_ssize_t ihigh)
{
	// clamp the limits
	uint32_t n_items = self->slice_n;

	if (ilow < 0)
		ilow = 0;
	else if (ilow > (Py_ssize_t)n_items)
		ilow = (Py_ssize_t)n_items;

	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > n_items)
		ihigh = (Py_ssize_t)n_items;

	uint32_t slice_i = self->slice_i + (uint32_t)ilow;
	uint32_t slice_n = (uint32_t)(ihigh - ilow);

	return (PyObject*)PyPointlessVector_New(self->pp, self->v, slice_i, slice_n);
}

static PyObject* PyPointlessVector_iter(PyObject* vector)
{
	if (!PyPointlessVector_Check(vector)) {
		PyErr_BadInternalCall();
		return 0;
	}

	PyPointlessVectorIter* iter = PyObject_GC_New(PyPointlessVectorIter, &PyPointlessVectorIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(vector);

	iter->vector = (PyPointlessVector*)vector;
	iter->iter_state = 0;

	PyObject_GC_Track(iter);

	return (PyObject*)iter;
}

static PyObject* PyPointlessVector_rev_iter(PyObject* vector)
{
	if (!PyPointlessVector_Check(vector)) {
		PyErr_BadInternalCall();
		return 0;
	}

	PyPointlessVectorRevIter* iter = PyObject_GC_New(PyPointlessVectorRevIter, &PyPointlessVectorRevIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(vector);

	iter->vector = (PyPointlessVector*)vector;
	iter->iter_state = 0;

	PyObject_GC_Track(iter);

	return (PyObject*)iter;
}

static PyObject* PyPointlessVectorIter_iternext(PyPointlessVectorIter* iter)
{
	// iterator already reached end
	if (iter->vector == 0)
		return 0;

	// see if we have any items left
	uint32_t n_items = (uint32_t)PyPointlessVector_length(iter->vector);

	if (iter->iter_state < n_items) {
		PyObject* item = PyPointlessVector_subscript_priv(iter->vector, iter->iter_state);
		iter->iter_state += 1;
		return item;
	}

	// we reached end
	Py_DECREF(iter->vector);
	iter->vector = 0;
	return 0;
}

static PyObject* PyPointlessVectorRevIter_iternext(PyPointlessVectorIter* iter)
{
	// already at the end
	if (iter->vector == 0)
		return 0;

	// see if we have any items left
	uint32_t n_items = (uint32_t)PyPointlessVector_length(iter->vector);

	if (iter->iter_state < n_items) {
		PyObject* item = PyPointlessVector_subscript_priv(iter->vector, n_items - iter->iter_state - 1);
		iter->iter_state += 1;
		return item;
	}

	// we reached end
	Py_DECREF(iter->vector);
	iter->vector = 0;
	return 0;
}

static PyObject* PyPointlessVector_richcompare(PyObject* a, PyObject* b, int op)
{
	if (!PyPointlessVector_Check(a) || !PyPointlessVector_Check(b)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	uint32_t n_items_a = (uint32_t)PyPointlessVector_length((PyPointlessVector*)a);
	uint32_t n_items_b = (uint32_t)PyPointlessVector_length((PyPointlessVector*)b);
	uint32_t i, n_items = (n_items_a < n_items_b) ? n_items_a : n_items_b;
	int32_t c;

	if (n_items_a != n_items_b && (op == Py_EQ || op == Py_NE)) {
		if (op == Py_EQ)
			Py_RETURN_FALSE;
		else
			Py_RETURN_TRUE;
	}

	// first item where they differ
	for (i = 0; i < n_items; i++) {
		PyObject* obj_a = PyPointlessVector_subscript_priv((PyPointlessVector*)a, i);
		PyObject* obj_b = PyPointlessVector_subscript_priv((PyPointlessVector*)b, i);

		if (obj_a == 0 || obj_b == 0) {
			Py_XDECREF(obj_a);
			Py_XDECREF(obj_b);
			return 0;
		}

		int k = PyObject_RichCompareBool(obj_a, obj_b, Py_EQ);

		Py_DECREF(obj_a);
		Py_DECREF(obj_b);

		obj_a = obj_b = 0;

		if (k < 0)
			return 0;

		if (k == 0)
			break;
	}

	// if all items up to n_items are the same, just compare on lengths
	if (i >= n_items_a || i >= n_items_b) {
		switch (op) {
			case Py_LT: c = (n_items_a <  n_items_b); break;
			case Py_LE: c = (n_items_a <= n_items_b); break;
			case Py_EQ: c = (n_items_a == n_items_b); break;
			case Py_NE: c = (n_items_a != n_items_b); break;
			case Py_GT: c = (n_items_a >  n_items_b); break;
			case Py_GE: c = (n_items_a >= n_items_b); break;
			default: assert(0); return 0;
		}

		if (c)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}

	// items are different
	if (op == Py_EQ)
		Py_RETURN_FALSE;
	else if (op == Py_NE)
		Py_RETURN_TRUE;

	// dig a bit deeper
	PyObject* obj_a = PyPointlessVector_subscript_priv((PyPointlessVector*)a, i);
	PyObject* obj_b = PyPointlessVector_subscript_priv((PyPointlessVector*)b, i);

	if (obj_a == 0 || obj_b == 0) {
		Py_XDECREF(obj_a);
		Py_XDECREF(obj_b);
		return 0;
	}

	PyObject* comp = PyObject_RichCompare(obj_a, obj_b, op);

	Py_DECREF(obj_a);
	Py_DECREF(obj_b);

	return comp;
}

static PyObject* PyPointlessVector_get_typecode(PyPointlessVector* a, void* closure)
{
	const char* s = 0;
	const char* e = 0;

	switch (a->v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			e = "this is a value-based vector";
			break;
		case POINTLESS_VECTOR_EMPTY:
			e = "empty vectors are typeless";
			break;
		case POINTLESS_VECTOR_I8:    s = "i8"; break;
		case POINTLESS_VECTOR_U8:    s = "u8"; break;
		case POINTLESS_VECTOR_I16:   s = "i16"; break;
		case POINTLESS_VECTOR_U16:   s = "u16"; break;
		case POINTLESS_VECTOR_I32:   s = "i32"; break;
		case POINTLESS_VECTOR_U32:   s = "u32"; break;
		case POINTLESS_VECTOR_I64:   s = "i64"; break;
		case POINTLESS_VECTOR_U64:   s = "u64"; break;
		case POINTLESS_VECTOR_FLOAT: s = "f"; break;
		default:
			PyErr_BadInternalCall();
			return 0;
	}

	if (e) {
		PyErr_SetString(PyExc_ValueError, e);
		return 0;
	}

	return Py_BuildValue("s", s);
}

static int pointless_is_prim_vector(pointless_value_t* v)
{
	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			return 0;
		case POINTLESS_VECTOR_EMPTY:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U64:
		case POINTLESS_VECTOR_FLOAT:
			return 1;
	}

	assert(0);
	return 0;
}

static size_t pointless_vector_item_size(PyPointlessVector* self)
{
	size_t n_bytes = 0;

	switch (self->v->type) {
		// no support for value vector
		case POINTLESS_VECTOR_EMPTY: n_bytes = 0;                 break;
		case POINTLESS_VECTOR_I8:    n_bytes = sizeof(int8_t);    break;
		case POINTLESS_VECTOR_U8:    n_bytes = sizeof(uint8_t);   break;
		case POINTLESS_VECTOR_I16:   n_bytes = sizeof(int16_t);   break;
		case POINTLESS_VECTOR_U16:   n_bytes = sizeof(uint16_t);  break;
		case POINTLESS_VECTOR_I32:   n_bytes = sizeof(int32_t);   break;
		case POINTLESS_VECTOR_U32:   n_bytes = sizeof(uint32_t);  break;
		case POINTLESS_VECTOR_I64:   n_bytes = sizeof(int64_t);   break;
		case POINTLESS_VECTOR_U64:   n_bytes = sizeof(uint64_t);  break;
		case POINTLESS_VECTOR_FLOAT: n_bytes = sizeof(float);     break;
		default:                     assert(0); break;
	}

	return n_bytes;
}

static size_t pointless_vector_n_bytes(PyPointlessVector* self)
{
	return pointless_vector_item_size(self) * self->slice_n;
}

static void* pointless_prim_vector_base_ptr(PyPointlessVector* self)
{
	switch (self->v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			assert(0);
			return 0;
		case POINTLESS_VECTOR_EMPTY: return 0;
		case POINTLESS_VECTOR_I8:    return (void*)(pointless_reader_vector_i8(&self->pp->p, self->v) + self->slice_i);
		case POINTLESS_VECTOR_U8:    return (void*)(pointless_reader_vector_u8(&self->pp->p, self->v) + self->slice_i);
		case POINTLESS_VECTOR_I16:   return (void*)(pointless_reader_vector_i16(&self->pp->p, self->v) + self->slice_i);
		case POINTLESS_VECTOR_U16:   return (void*)(pointless_reader_vector_u16(&self->pp->p, self->v) + self->slice_i);
		case POINTLESS_VECTOR_I32:   return (void*)(pointless_reader_vector_i32(&self->pp->p, self->v) + self->slice_i);
		case POINTLESS_VECTOR_U32:   return (void*)(pointless_reader_vector_u32(&self->pp->p, self->v) + self->slice_i);
		case POINTLESS_VECTOR_I64:   return (void*)(pointless_reader_vector_i64(&self->pp->p, self->v) + self->slice_i);
		case POINTLESS_VECTOR_U64:   return (void*)(pointless_reader_vector_u64(&self->pp->p, self->v) + self->slice_i);
		case POINTLESS_VECTOR_FLOAT: return (void*)(pointless_reader_vector_float(&self->pp->p, self->v) + self->slice_i);
	}

	assert(0);
	return 0;
}

static int PyPointlessVector_getbuffer(PyPointlessVector* self, Py_buffer* view, int flags)
{
	if (view == 0)
		return 0;

	void* ptr = pointless_prim_vector_base_ptr(self);
	return PyBuffer_FillInfo(view, (PyObject*)self, ptr, pointless_vector_n_bytes(self), 0, flags);
}

static void PyPointlessVector_releasebuffer(PyPointlessVector* obj, Py_buffer *view)
{
}

static PyObject* PyPointlessVector_sizeof(PyPointlessVector* self)
{
	return PyLong_FromSize_t(sizeof(PyPointlessVector));
}

#define POINTLESS_VECTOR_MAX_LOOP(T, v, n) \
	{\
		m_i = 0;\
		for (i = 1; i < (n); i++) {\
			if (((T*)(v))[i] > ((T*)(v))[m_i])\
				m_i = i;\
		}\
	}\

#define POINTLESS_VECTOR_MIN_LOOP(T, v, n) \
	{\
		m_i = 0;\
		for (i = 1; i < (n); i++) {\
			if (((T*)(v))[i] < ((T*)(v))[m_i])\
				m_i = i;\
		}\
	}\

static PyObject* PyPointlessVector_max(PyPointlessVector* self)
{
	if (!pointless_is_prim_vector(self->v)) {
		PyErr_SetString(PyExc_ValueError, "only primitive vectors support this operation");
		return 0;
	}

	size_t i, m_i, n_items = self->slice_n;
	void* base_ptr = pointless_prim_vector_base_ptr(self);

	if (n_items == 0) {
		PyErr_SetString(PyExc_ValueError, "vector is empty");
		return 0;
	}

	switch (self->v->type) {
		case POINTLESS_VECTOR_I8:
			POINTLESS_VECTOR_MAX_LOOP(int8_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_U8:
			POINTLESS_VECTOR_MAX_LOOP(uint8_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_I16:
			POINTLESS_VECTOR_MAX_LOOP(int16_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_U16:
			POINTLESS_VECTOR_MAX_LOOP(uint16_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_I32:
			POINTLESS_VECTOR_MAX_LOOP(int32_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_U32:
			POINTLESS_VECTOR_MAX_LOOP(uint32_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_I64:
			POINTLESS_VECTOR_MAX_LOOP(int64_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_U64:
			POINTLESS_VECTOR_MAX_LOOP(uint64_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_FLOAT:
			POINTLESS_VECTOR_MAX_LOOP(float, base_ptr, n_items);
			break;
		default:
			PyErr_BadInternalCall();
			return 0;
	}

	return PyPointlessVector_subscript_priv(self, m_i);
}

static PyObject* PyPointlessVector_min(PyPointlessVector* self)
{
	if (!pointless_is_prim_vector(self->v)) {
		PyErr_SetString(PyExc_ValueError, "only primitive vectors support this operation");
		return 0;
	}

	size_t i, m_i, n_items = self->slice_n;
	void* base_ptr = pointless_prim_vector_base_ptr(self);

	if (n_items == 0) {
		PyErr_SetString(PyExc_ValueError, "vector is empty");
		return 0;
	}

	switch (self->v->type) {
		case POINTLESS_VECTOR_I8:
			POINTLESS_VECTOR_MIN_LOOP(int8_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_U8:
			POINTLESS_VECTOR_MIN_LOOP(uint8_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_I16:
			POINTLESS_VECTOR_MIN_LOOP(int16_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_U16:
			POINTLESS_VECTOR_MIN_LOOP(uint16_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_I32:
			POINTLESS_VECTOR_MIN_LOOP(int32_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_U32:
			POINTLESS_VECTOR_MIN_LOOP(uint32_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_I64:
			POINTLESS_VECTOR_MIN_LOOP(int64_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_U64:
			POINTLESS_VECTOR_MIN_LOOP(uint64_t, base_ptr, n_items);
			break;
		case POINTLESS_VECTOR_FLOAT:
			POINTLESS_VECTOR_MIN_LOOP(float, base_ptr, n_items);
			break;
		default:
			PyErr_BadInternalCall();
			return 0;
	}

	return PyPointlessVector_subscript_priv(self, m_i);
}

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

static PyObject* PyPointlessVector_bisect_left(PyPointlessVector* self, PyObject* args)
{
	int is_signed = 0;
	int64_t _ii = 0;
	uint64_t _uu = 0;

	if (PyTuple_Check(args) && PyTuple_GET_SIZE(args) == 1 && parse_pyobject_number(PyTuple_GET_ITEM(args, 0), &is_signed, &_ii, &_uu)) {
		if (is_signed) {
			PyErr_Format(PyExc_ValueError, "value is signed");
			return 0;
		}
	} else {
		PyErr_Format(PyExc_ValueError, "we need a number in the range [0, 2**64-1]");
		return 0;
	}

	if (self->v->type != POINTLESS_VECTOR_U64) {
		PyErr_Format(PyExc_ValueError, "vector must be u64");
		return 0;
	}

	uint64_t* a = pointless_prim_vector_base_ptr(self);
	int64_t i, j, mid;

	i = 0, j = self->slice_n; 

	while (i < j) {
		mid = (i + j) / 2;

		if (a[mid] < _uu)
			i = mid + 1;
		else
			j = mid;
	}

	return PyLong_FromLongLong((PY_LONG_LONG)i);
}

static PyGetSetDef PyPointlessVector_getsets [] = {
	{"typecode", (getter)PyPointlessVector_get_typecode, 0, "the typecode character used to create the vector"},
	{NULL}
};

static PyMemberDef PyPointlessVector_memberlist[] = {
	{"container_id",  T_ULONG, offsetof(PyPointlessVector, container_id), READONLY},
	{"is_hashable", T_INT, offsetof(PyPointlessVector, is_hashable), READONLY},
	{NULL}
};

static PyMethodDef PyPointlessVector_methods[] = {
	{"max",          (PyCFunction)PyPointlessVector_max,          METH_NOARGS,  ""},
	{"min",          (PyCFunction)PyPointlessVector_min,          METH_NOARGS,  ""},
	{"bisect_left",  (PyCFunction)PyPointlessVector_bisect_left,  METH_VARARGS, ""},
	{"__reversed__", (PyCFunction)PyPointlessVector_rev_iter,     METH_NOARGS,  ""},
	{"__sizeof__",   (PyCFunction)PyPointlessVector_sizeof,       METH_NOARGS,  ""},
	{NULL, NULL}
};

static PyMappingMethods PyPointlessVector_as_mapping = {
	(lenfunc)PyPointlessVector_length,
	(binaryfunc)PyPointlessVector_subscript,
	(objobjargproc)0
};

static PyBufferProcs PyPointlessVector_as_buffer = {
	(getbufferproc)PyPointlessVector_getbuffer,
	(releasebufferproc)PyPointlessVector_releasebuffer
};

static PySequenceMethods PyPointlessVector_as_sequence = {
	(lenfunc)PyPointlessVector_length,          /* sq_length */
	0,                                          /* sq_concat */
	0,                                          /* sq_repeat */
	(ssizeargfunc)PyPointlessVector_item,       /* sq_item */
    (ssizessizeargfunc)PyPointlessVector_slice, /* sq_slice */
	0,                                          /* sq_ass_item */
    0,                                          /* sq_ass_slice */
	(objobjproc)PyPointlessVector_contains,     /* sq_contains */
	0,                                          /* sq_inplace_concat */
	0,                                          /* sq_inplace_repeat */
};


PyTypeObject PyPointlessVectorType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessVector",                  /*tp_name*/
	sizeof(PyPointlessVector),                      /*tp_basicsize*/
	0,                                              /*tp_itemsize*/
	(destructor)PyPointlessVector_dealloc,          /*tp_dealloc*/
	0,                                              /*tp_print*/
	0,                                              /*tp_getattr*/
	0,                                              /*tp_setattr*/
	0,                                              /*tp_compare*/
	PyPointless_repr,                               /*tp_repr*/
	0,                                              /*tp_as_number*/
	&PyPointlessVector_as_sequence,                 /*tp_as_sequence*/
	&PyPointlessVector_as_mapping,                  /*tp_as_mapping*/
	0,                                              /*tp_hash */
	0,                                              /*tp_call*/
	PyPointless_str,                                /*tp_str*/
	0,                                              /*tp_getattro*/
	0,                                              /*tp_setattro*/
	&PyPointlessVector_as_buffer,                   /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                             /*tp_flags*/
	"PyPointlessVector wrapper",                    /*tp_doc */
	0,                                              /*tp_traverse */
	0,                                              /*tp_clear */
	PyPointlessVector_richcompare,                  /*tp_richcompare */
	0,                                              /*tp_weaklistoffset */
	PyPointlessVector_iter,                         /*tp_iter */
	0,                                              /*tp_iternext */
	PyPointlessVector_methods,                      /*tp_methods */
	PyPointlessVector_memberlist,                   /*tp_members */
	PyPointlessVector_getsets,                      /*tp_getset */
	0,                                              /*tp_base */
	0,                                              /*tp_dict */
	0,                                              /*tp_descr_get */
	0,                                              /*tp_descr_set */
	0,                                              /*tp_dictoffset */
	(initproc)PyPointlessVector_init,               /*tp_init */
	0,                                              /*tp_alloc */
	PyPointlessVector_new,                          /*tp_new */
};

PyTypeObject PyPointlessVectorIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessVectorIter",            /*tp_name*/
	sizeof(PyPointlessVectorIter),                /*tp_basicsize*/
	0,                                            /*tp_itemsize*/
	(destructor)PyPointlessVectorIter_dealloc,    /*tp_dealloc*/
	0,                                            /*tp_print*/
	0,                                            /*tp_getattr*/
	0,                                            /*tp_setattr*/
	0,                                            /*tp_compare*/
	0,                                            /*tp_repr*/
	0,                                            /*tp_as_number*/
	0,                                            /*tp_as_sequence*/
	0,                                            /*tp_as_mapping*/
	0,                                            /*tp_hash */
	0,                                            /*tp_call*/
	0,                                            /*tp_str*/
	0,                                            /*tp_getattro*/
	0,                                            /*tp_setattro*/
	0,                                            /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                           /*tp_flags*/
	"PyPointlessVectorIter",                      /*tp_doc */
	(traverseproc)PyPointlessVectorIter_traverse, /*tp_traverse */
	0,                                            /*tp_clear */
	0,                                            /*tp_richcompare */
	0,                                            /*tp_weaklistoffset */
	PyObject_SelfIter,                            /*tp_iter */
	(iternextfunc)PyPointlessVectorIter_iternext, /*tp_iternext */
	0,                                            /*tp_methods */
	0,                                            /*tp_members */
	0,                                            /*tp_getset */
	0,                                            /*tp_base */
	0,                                            /*tp_dict */
	0,                                            /*tp_descr_get */
	0,                                            /*tp_descr_set */
	0,                                            /*tp_dictoffset */
	(initproc)PyPointlessVectorIter_init,         /*tp_init */
	0,                                            /*tp_alloc */
	PyPointlessVectorIter_new,                    /*tp_new */
};

PyTypeObject PyPointlessVectorRevIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessVectorRevIter",         /*tp_name*/
	sizeof(PyPointlessVectorRevIter),             /*tp_basicsize*/
	0,                                            /*tp_itemsize*/
	(destructor)PyPointlessVectorRevIter_dealloc, /*tp_dealloc*/
	0,                                            /*tp_print*/
	0,                                            /*tp_getattr*/
	0,                                            /*tp_setattr*/
	0,                                            /*tp_compare*/
	0,                                            /*tp_repr*/
	0,                                            /*tp_as_number*/
	0,                                            /*tp_as_sequence*/
	0,                                            /*tp_as_mapping*/
	0,                                            /*tp_hash */
	0,                                            /*tp_call*/
	0,                                            /*tp_str*/
	0,                                            /*tp_getattro*/
	0,                                            /*tp_setattro*/
	0,                                            /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                           /*tp_flags*/
	"PyPointlessVectorRevIter",                   /*tp_doc */
	(traverseproc)PyPointlessVectorRevIter_traverse, /*tp_traverse */
	0,                                            /*tp_clear */
	0,                                            /*tp_richcompare */
	0,                                            /*tp_weaklistoffset */
	PyObject_SelfIter,                            /*tp_iter */
	(iternextfunc)PyPointlessVectorRevIter_iternext, /*tp_iternext */
	0,                                            /*tp_methods */
	0,                                            /*tp_members */
	0,                                            /*tp_getset */
	0,                                            /*tp_base */
	0,                                            /*tp_dict */
	0,                                            /*tp_descr_get */
	0,                                            /*tp_descr_set */
	0,                                            /*tp_dictoffset */
	(initproc)PyPointlessVectorRevIter_init,      /*tp_init */
	0,                                            /*tp_alloc */
	PyPointlessVectorRevIter_new,                 /*tp_new */
};

PyPointlessVector* PyPointlessVector_New(PyPointless* pp, pointless_value_t* v, uint32_t slice_i, uint32_t slice_n)
{
	if (!(slice_i + slice_n <= pointless_reader_vector_n_items(&pp->p, v))) {
		PyErr_SetString(PyExc_ValueError, "slice invariant error when creating PyPointlessVector");
		return 0;
	}

	PyPointlessVector* pv = PyObject_New(PyPointlessVector, &PyPointlessVectorType);

	if (pv == 0)
		return 0;

	Py_INCREF(pp);
	pp->n_vector_refs += 1;

	pv->pp = pp;
	pv->v = v;

	// the container ID is unique between all non-empty vectors, maps and sets
	pv->container_id = (unsigned long)pointless_container_id(&pp->p, v);
	pv->is_hashable = (int)pointless_is_hashable(v->type);

	pv->slice_i = slice_i;
	pv->slice_n = slice_n;

	return pv;
}
