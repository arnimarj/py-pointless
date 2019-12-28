#include "../pointless_ext.h"

static void PyPointless_dealloc(PyPointless* self)
{
	if (self->is_open) {
		Py_BEGIN_ALLOW_THREADS
		pointless_close(&self->p);
		Py_END_ALLOW_THREADS
		self->is_open = 0;
	}

	self->allow_print = 0;

	if (self->n_root_refs != 0 ||
		self->n_vector_refs != 0 ||
		self->n_bitvector_refs != 0 ||
		self->n_map_refs != 0 ||
		self->n_set_refs != 0) {

		printf("WTF A: %zu\n", self->n_root_refs);
		printf("WTF B: %zu\n", self->n_vector_refs);
		printf("WTF C: %zu\n", self->n_bitvector_refs);
		printf("WTF D: %zu\n", self->n_map_refs);
		printf("WTF E: %zu\n", self->n_set_refs);
		printf("-------------------------------------\n");
	}

	self->n_root_refs = 0;
	self->n_vector_refs = 0;
	self->n_bitvector_refs = 0;
	self->n_map_refs = 0;
	self->n_set_refs = 0;

	Py_TYPE(self)->tp_free(self);
}

static PyObject* PyPointless_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointless* self = (PyPointless*)type->tp_alloc(type, 0);

	if (self) {
		self->allow_print = 0;
		self->is_open = 0;
		self->n_root_refs = 0;
		self->n_vector_refs = 0;
		self->n_bitvector_refs = 0;
		self->n_map_refs = 0;
		self->n_set_refs = 0;
	}

	return (PyObject*)self;
}

static PyObject* PyPointless_GetRoot(PyPointless* self)
{
	return pypointless_value(self, &self->p.header->root);
}

static PyObject* PyPointless_GetINode(PyPointless* self)
{
	if (self->p.fd == 0) {
		PyErr_Format(PyExc_ValueError, "pointless object is buffer-based");
		return 0;
	}

	int f = fileno(self->p.fd);
	struct stat buf;

	if (f != -1)
		f = fstat(f, &buf);

	if (f == -1) {
		PyErr_SetFromErrno(PyExc_OSError);
		return 0;
	}

	return PyLong_FromUnsignedLong(buf.st_ino);
}

static PyObject* PyPointless_GetFileNo(PyPointless* self)
{
	if (self->p.fd == 0) {
		PyErr_Format(PyExc_ValueError, "pointless object is buffer-based");
		return 0;
	}

	int f = fileno(self->p.fd);

	if (f == -1) {
		PyErr_SetFromErrno(PyExc_OSError);
		return 0;
	}

	return PyLong_FromUnsignedLong((unsigned long long)f);
}

static PyObject* PyPointless_GetRefs(PyPointless* self)
{
	return Py_BuildValue("{s:n,s:n,s:n,s:n,s:n}",
		"n_root_refs", self->n_root_refs,
		"n_vector_refs", self->n_vector_refs,
		"n_bitvector_refs", self->n_bitvector_refs,
		"n_map_refs", self->n_map_refs,
		"n_set_refs", self->n_set_refs
	);
}

static PyObject* PyPointless_sizeof(PyPointless* self)
{
	if (self->p.fd == 0)
		return PyLong_FromUnsignedLongLong(sizeof(PyPointless) + self->p.buflen);
	else
		return PyLong_FromUnsignedLongLong(sizeof(PyPointless) + self->p.fd_len);
}

static PyMethodDef PyPointless_methods[] = {
	{"__sizeof__", (PyCFunction)PyPointless_sizeof,    METH_NOARGS, "get size in bytes of backing file or buffer"},
	{"GetRoot",    (PyCFunction)PyPointless_GetRoot,   METH_NOARGS, "get pointless root object" },
	{"GetINode",   (PyCFunction)PyPointless_GetINode,  METH_NOARGS, "get inode of file descriptor" },
	{"GetFileNo",  (PyCFunction)PyPointless_GetFileNo, METH_NOARGS, "get file descriptor" },
	{"GetRefs",    (PyCFunction)PyPointless_GetRefs,   METH_NOARGS, "get inside-reference count to base object" },
	{NULL}
};

static int PyPointless_init(PyPointless* self, PyObject* args, PyObject* kwds)
{
	PyObject* fname_or_buffer = 0;
	PyObject* string_of_unicode = 0;
	PyPointlessPrimVector* vector = 0;
	void* buf = 0;
	size_t buflen = 0;
	const char* fname_ = 0;
	const char* error = 0;
	int i = 0;

	if (self->is_open) {
		Py_BEGIN_ALLOW_THREADS
		pointless_close(&self->p);
		Py_END_ALLOW_THREADS
		self->is_open = 0;
	}

	self->allow_print = 1;

	if (self->n_root_refs != 0 ||
		self->n_vector_refs != 0 ||
		self->n_bitvector_refs != 0 ||
		self->n_map_refs != 0 ||
		self->n_set_refs != 0) {
		printf("_WTF A: %zu\n", self->n_root_refs);
		printf("_WTF B: %zu\n", self->n_vector_refs);
		printf("_WTF C: %zu\n", self->n_bitvector_refs);
		printf("_WTF D: %zu\n", self->n_map_refs);
		printf("_WTF E: %zu\n", self->n_set_refs);
	}

	self->n_root_refs = 0;
	self->n_vector_refs = 0;
	self->n_bitvector_refs = 0;
	self->n_map_refs = 0;
	self->n_set_refs = 0;

	PyObject* allow_print = Py_True;
	PyObject* validate = Py_True;
	static char* kwargs[] = {"filename_or_buffer", "allow_print", "validate", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O!O!", kwargs, &fname_or_buffer, &PyBool_Type, &allow_print, &PyBool_Type, &validate))
		return -1;

	if (allow_print == Py_False)
		self->allow_print = 0;

	int do_validate = (validate == Py_True);

#ifdef Py_UNICODE_WIDE
	int force_ucs2 = 0;
#else
	int force_ucs2 = 1;
#endif

	if (PyUnicode_Check(fname_or_buffer)) {
		string_of_unicode = PyUnicode_AsLatin1String(fname_or_buffer);

		if (string_of_unicode == 0)
			return -1;

		fname_ = PyBytes_AS_STRING(string_of_unicode);

	} else if (PyPointlessPrimVector_Check(fname_or_buffer)) {
		vector = (PyPointlessPrimVector*)fname_or_buffer;

		if (vector->type != POINTLESS_PRIM_VECTOR_TYPE_U8) {
			PyErr_SetString(PyExc_ValueError, "buffer must be primvector with uint8");
			return -1;
		}

		buf = pointless_dynarray_buffer(&vector->array);
		buflen = pointless_dynarray_n_items(&vector->array);
	} else if (PyByteArray_Check(fname_or_buffer)) {
		buf = PyByteArray_AS_STRING(fname_or_buffer);
		buflen = (size_t)PyByteArray_GET_SIZE(fname_or_buffer);
	} else {
		PyErr_SetString(PyExc_ValueError, "filename_or_buffer must be string/unicode/bytearray/primvector-with-uint-8");
		return -1;
	}

	Py_BEGIN_ALLOW_THREADS

	if (do_validate) {
		if (fname_)
			i = pointless_open_f(&self->p, fname_, force_ucs2, &error);
		else
			i = pointless_open_b(&self->p, buf, buflen, force_ucs2, &error);
	} else {
		if (fname_)
			i = pointless_open_f_skip_validate(&self->p, fname_, force_ucs2, &error);
		else
			i = pointless_open_b_skip_validate(&self->p, buf, buflen, force_ucs2, &error);
	}

	Py_END_ALLOW_THREADS

	if (!i) {
		if (fname_)
			PyErr_Format(PyExc_IOError, "error opening [%s]: %s", fname_, error);
		else
			PyErr_Format(PyExc_IOError, "error parsing file from buffer: %s", error);

		Py_XDECREF(string_of_unicode);

		return -1;
	}

	Py_XDECREF(string_of_unicode);

	self->is_open = 1;
	return 0;
}

PyTypeObject PyPointlessType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointless",         /*tp_name*/
	sizeof(PyPointless),             /*tp_basicsize*/
	0,                               /*tp_itemsize*/
	(destructor)PyPointless_dealloc, /*tp_dealloc*/
	0,                               /*tp_print*/
	0,                               /*tp_getattr*/
	0,                               /*tp_setattr*/
	0,                               /*tp_compare*/
	0,                               /*tp_repr*/
	0,                               /*tp_as_number*/
	0,                               /*tp_as_sequence*/
	0,                               /*tp_as_mapping*/
	0,                               /*tp_hash */
	0,                               /*tp_call*/
	0,                               /*tp_str*/
	0,                               /*tp_getattro*/
	0,                               /*tp_setattro*/
	0,                               /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,              /*tp_flags*/
	"PyPointless wrapper",           /*tp_doc */
	0,                               /*tp_traverse */
	0,                               /*tp_clear */
	0,                               /*tp_richcompare */
	0,                               /*tp_weaklistoffset */
	0,                               /*tp_iter */
	0,                               /*tp_iternext */
	PyPointless_methods,             /*tp_methods */
	0,                               /*tp_members */
	0,                               /*tp_getset */
	0,                               /*tp_base */
	0,                               /*tp_dict */
	0,                               /*tp_descr_get */
	0,                               /*tp_descr_set */
	0,                               /*tp_dictoffset */
	(initproc)PyPointless_init,      /*tp_init */
	0,                               /*tp_alloc */
	PyPointless_new,                 /*tp_new */
};
