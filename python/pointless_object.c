#include "../pointless_ext.h"

static void PyPointless_dealloc(PyPointless* self)
{
	if (self->is_open) {
		Py_BEGIN_ALLOW_THREADS
		pointless_close(&self->p);
		Py_END_ALLOW_THREADS
		self->is_open = 0;
	}

	self->ob_type->tp_free((PyObject*)self);
}

static PyObject* PyPointless_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointless* self = (PyPointless*)type->tp_alloc(type, 0);

	if (self)
		self->is_open = 0;

	return (PyObject*)self;
}

static PyObject* PyPointless_GetRoot(PyPointless* self)
{
	return pypointless_value(self, &self->p.header->root);
}

static PyObject* PyPointless_sizeof(PyPointless* self)
{
	return PyLong_FromUnsignedLongLong(self->p.fd_len);
}

static PyMethodDef PyPointless_methods[] = {
	{"__sizeof__", (PyCFunction)PyPointless_sizeof,  METH_NOARGS, "get size in bytes of backing file or buffer"},
	{"GetRoot",    (PyCFunction)PyPointless_GetRoot, METH_NOARGS, "get pointless root object" },
	{NULL}
};

static int PyPointless_init(PyPointless* self, PyObject* args)
{
	const char* fname = 0;
	const char* error = 0;
	int i = 0;

	if (self->is_open) {
		pointless_close(&self->p);
		self->is_open = 0;
	}

	if (!PyArg_ParseTuple(args, "s", &fname))
		return -1;

	Py_BEGIN_ALLOW_THREADS
	i = pointless_open_f(&self->p, fname, &error);
	Py_END_ALLOW_THREADS

	if (!i) {
		PyErr_Format(PyExc_IOError, "error opening [%s]: %s", fname, error);
		return -1;
	}

	self->is_open = 1;
	return 0;
}

PyTypeObject PyPointlessType = {
	PyObject_HEAD_INIT(NULL)
	0,                               /*ob_size*/
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
