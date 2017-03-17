#include "../pointless_ext.h"

static void PyPointlessSet_dealloc(PyPointlessSet* self)
{
	if (self->pp) {
		self->pp->n_set_refs -= 1;
		Py_DECREF(self->pp);
	}

	self->pp = 0;
	self->v = 0;
	self->container_id = 0;
	Py_TYPE(self)->tp_free(self);
}

static void PyPointlessSetIter_dealloc(PyPointlessSetIter* self)
{
	Py_XDECREF(self->set);
	self->set = 0;
	self->iter_state = 0;
	Py_TYPE(self)->tp_free(self);
}

PyObject* PyPointlessSet_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessSet* self = (PyPointlessSet*)type->tp_alloc(type, 0);

	if (self) {
		self->pp = 0;
		self->v = 0;
		self->container_id = 0;
	}

	return (PyObject*)self;
}

PyObject* PyPointlessSetIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessSetIter* self = (PyPointlessSetIter*)type->tp_alloc(type, 0);

	if (self) {
		self->set = 0;
		self->iter_state = 0;
	}

	return (PyObject*)self;
}

static int PyPointlessSet_init(PyPointlessSet* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessSet directly");
	return -1;
}

static int PyPointlessSetIter_init(PyPointlessSet* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessSetIter directly");
	return -1;
}

static Py_ssize_t PyPointlessSet_length(PyPointlessSet* self)
{
	return (Py_ssize_t)pointless_reader_set_n_items(&self->pp->p, self->v);
}

static PyObject* PyPointlessSet_iter(PyObject* set)
{
	if (!PyPointlessSet_Check(set)) {
		PyErr_BadInternalCall();
		return 0;
	}

	PyPointlessSetIter* iter = PyObject_New(PyPointlessSetIter, &PyPointlessSetIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(set);

	iter->set = (PyPointlessSet*)set;
	iter->iter_state = 0;

	return (PyObject*)iter;
}

static PyObject* PyPointlessSetIter_iternext(PyPointlessSetIter* iter)
{
	// iterator already reached end
	if (iter->set == 0)
		return 0;

	pointless_value_t* v = 0;

	if (pointless_reader_set_iter(&iter->set->pp->p, iter->set->v, &v, &iter->iter_state))
		return pypointless_value(iter->set->pp, v);

	Py_DECREF(iter->set);
	iter->set = 0;
	return 0;
}

static uint32_t PyPointlessSet_eq_cb(pointless_t* p, pointless_complete_value_t* v, void* user, const char** error)
{
	return pypointless_cmp_eq(p, v, (PyObject*)user, error);
}

static int PyPointlessSet_contains(PyPointlessSet* s, PyObject* key)
{
	const char* error = 0;
	uint32_t hash = pyobject_hash_32(key, s->pp->p.header->version, &error);

	if (error) {
		PyErr_Format(PyExc_ValueError, "pointless hash error: %s", error);
		return -1;
	}

	pointless_value_t* kk = 0;
	pointless_reader_set_lookup_ext(&s->pp->p, s->v, hash, PyPointlessSet_eq_cb, (void*)key, &kk, &error);

	if (error) {
		PyErr_Format(PyExc_ValueError, "pointless set query error: %s", error);
		return -1;
	}

	return (kk != 0);
}

static PyMemberDef PyPointlessSet_memberlist[] = {
	{"container_id",  T_ULONG, offsetof(PyPointlessSet, container_id), READONLY},
	{NULL}
};

static PyMappingMethods PyPointlessSet_as_mapping = {
	(lenfunc)PyPointlessSet_length,
	0,
	0
};

static PySequenceMethods PyPointlessSet_as_sequence = {
	0,                                   /* sq_length */
	0,                                   /* sq_concat */
	0,                                   /* sq_repeat */
	0,                                   /* sq_item */
	0,                                   /* sq_slice */
	0,                                   /* sq_ass_item */
	0,                                   /* sq_ass_slice */
	(objobjproc)PyPointlessSet_contains, /* sq_contains */
};

PyTypeObject PyPointlessSetType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessSet",          /*tp_name*/
	sizeof(PyPointlessSet),              /*tp_basicsize*/
	0,                                   /*tp_itemsize*/
	(destructor)PyPointlessSet_dealloc,  /*tp_dealloc*/
	0,                                   /*tp_print*/
	0,                                   /*tp_getattr*/
	0,                                   /*tp_setattr*/
	0,                                   /*tp_compare*/
	PyPointless_repr,                    /*tp_repr*/
	0,                                   /*tp_as_number*/
	&PyPointlessSet_as_sequence,         /*tp_as_sequence*/
	&PyPointlessSet_as_mapping,          /*tp_as_mapping*/
	0,                                   /*tp_hash */
	0,                                   /*tp_call*/
	PyPointless_str,                     /*tp_str*/
	0,                                   /*tp_getattro*/
	0,                                   /*tp_setattro*/
	0,                                   /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                  /*tp_flags*/
	"PyPointlessSet wrapper",            /*tp_doc */
	0,                                   /*tp_traverse */
	0,                                   /*tp_clear */
	0,                                   /*tp_richcompare */
	0,                                   /*tp_weaklistoffset */
	PyPointlessSet_iter,                 /*tp_iter */
	0,                                   /*tp_iternext */
	0,                                   /*tp_methods */
	PyPointlessSet_memberlist,           /*tp_members */
	0,                                   /*tp_getset */
	0,                                   /*tp_base */
	0,                                   /*tp_dict */
	0,                                   /*tp_descr_get */
	0,                                   /*tp_descr_set */
	0,                                   /*tp_dictoffset */
	(initproc)PyPointlessSet_init,       /*tp_init */
	0,                                   /*tp_alloc */
	PyPointlessSet_new,                  /*tp_new */
};

PyTypeObject PyPointlessSetIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessSetIter",            /*tp_name*/
	sizeof(PyPointlessSetIter),                /*tp_basicsize*/
	0,                                         /*tp_itemsize*/
	(destructor)PyPointlessSetIter_dealloc,    /*tp_dealloc*/
	0,                                         /*tp_print*/
	0,                                         /*tp_getattr*/
	0,                                         /*tp_setattr*/
	0,                                         /*tp_compare*/
	0,                                         /*tp_repr*/
	0,                                         /*tp_as_number*/
	0,                                         /*tp_as_sequence*/
	0,                                         /*tp_as_mapping*/
	0,                                         /*tp_hash */
	0,                                         /*tp_call*/
	0,                                         /*tp_str*/
	0,                                         /*tp_getattro*/
	0,                                         /*tp_setattro*/
	0,                                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                        /*tp_flags*/
	"PyPointlessSetIter",                      /*tp_doc */
	0,                                         /*tp_traverse */
	0,                                         /*tp_clear */
	0,                                         /*tp_richcompare */
	0,                                         /*tp_weaklistoffset */
	PyObject_SelfIter,                         /*tp_iter */
	(iternextfunc)PyPointlessSetIter_iternext, /*tp_iternext */
	0,                                         /*tp_methods */
	0,                                         /*tp_members */
	0,                                         /*tp_getset */
	0,                                         /*tp_base */
	0,                                         /*tp_dict */
	0,                                         /*tp_descr_get */
	0,                                         /*tp_descr_set */
	0,                                         /*tp_dictoffset */
	(initproc)PyPointlessSetIter_init,         /*tp_init */
	0,                                         /*tp_alloc */
	PyPointlessSetIter_new,                    /*tp_new */
};

PyPointlessSet* PyPointlessSet_New(PyPointless* pp, pointless_value_t* v)
{
	PyPointlessSet* pv = PyObject_New(PyPointlessSet, &PyPointlessSetType);

	if (pv == 0)
		return 0;

	Py_INCREF(pp);

	pp->n_set_refs += 1;

	pv->pp = pp;
	pv->v = v;

	// the container ID is a unique between all non-empty vectors, maps and sets
	pv->container_id = pointless_container_id(&pp->p, v);

	return pv;
}
