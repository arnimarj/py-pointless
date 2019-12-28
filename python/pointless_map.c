#include "../pointless_ext.h"

static void PyPointlessMap_dealloc(PyPointlessMap* self)
{
	if (self->pp) {
		self->pp->n_map_refs -= 1;
		Py_DECREF(self->pp);
	}

	self->pp = 0;
	self->v = 0;
	self->container_id = 0;
	PyObject_Del(self);
}

static void PyPointlessMapKeyIter_dealloc(PyPointlessMapKeyIter* self)
{
	Py_XDECREF(self->map);
	self->map = 0;
	self->iter_state = 0;
	Py_TYPE(self)->tp_free(self);
}

static void PyPointlessMapValueIter_dealloc(PyPointlessMapValueIter* self)
{
	Py_XDECREF(self->map);
	self->map = 0;
	self->iter_state = 0;
	Py_TYPE(self)->tp_free(self);
}

static void PyPointlessMapItemIter_dealloc(PyPointlessMapItemIter* self)
{
	Py_XDECREF(self->map);
	self->map = 0;
	self->iter_state = 0;
	Py_TYPE(self)->tp_free(self);
}

PyObject* PyPointlessMap_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessMap* self = (PyPointlessMap*)type->tp_alloc(type, 0);

	if (self) {
		self->pp = 0;
		self->v = 0;
		self->container_id = 0;
	}

	return (PyObject*)self;
}

PyObject* PyPointlessMapKeyIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessMapKeyIter* self = (PyPointlessMapKeyIter*)type->tp_alloc(type, 0);

	if (self) {
		self->map = 0;
		self->iter_state = 0;
	}

	return (PyObject*)self;
}

PyObject* PyPointlessMapValueIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessMapValueIter* self = (PyPointlessMapValueIter*)type->tp_alloc(type, 0);

	if (self) {
		self->map = 0;
		self->iter_state = 0;
	}

	return (PyObject*)self;
}

PyObject* PyPointlessMapItemIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyPointlessMapItemIter* self = (PyPointlessMapItemIter*)type->tp_alloc(type, 0);

	if (self) {
		self->map = 0;
		self->iter_state = 0;
	}

	return (PyObject*)self;
}

static int PyPointlessMap_init(PyPointlessMap* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessMap directly");
	return -1;
}

static int PyPointlessMapKeyIter_init(PyPointlessMap* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessMapKeyIter directly");
	return -1;
}

static int PyPointlessMapValueIter_init(PyPointlessMap* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessMapValueIter directly");
	return -1;
}

static int PyPointlessMapItemIter_init(PyPointlessMap* self, PyObject* args)
{
	PyErr_SetString(PyExc_TypeError, "unable to instantiate PyPointlessMapItemIter directly");
	return -1;
}

static Py_ssize_t PyPointlessMap_length(PyPointlessSet* self)
{
	return (Py_ssize_t)pointless_reader_map_n_items(&self->pp->p, self->v);
}

static PyObject* PyPointlessMap_iter(PyObject* map)
{
	if (!PyPointlessMap_Check(map)) {
		PyErr_BadInternalCall();
		return 0;
	}

	PyPointlessMapKeyIter* iter = PyObject_New(PyPointlessMapKeyIter, &PyPointlessMapKeyIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(map);

	iter->map = (PyPointlessMap*)map;
	iter->iter_state = 0;

	return (PyObject*)iter;
}

static PyObject* PyPointlessMapKeyIter_iternext(PyPointlessMapKeyIter* iter)
{
	// iterator already reached end
	if (iter->map == 0)
		return 0;

	pointless_value_t* k = 0;
	pointless_value_t* v = 0;

	if (pointless_reader_map_iter(&iter->map->pp->p, iter->map->v, &k, &v, &iter->iter_state))
		return pypointless_value(iter->map->pp, k);

	Py_DECREF(iter->map);
	iter->map = 0;
	return 0;
}

static PyObject* PyPointlessMapValueIter_iternext(PyPointlessMapValueIter* iter)
{
	// iterator already reached end
	if (iter->map == 0)
		return 0;

	pointless_value_t* k = 0;
	pointless_value_t* v = 0;

	if (pointless_reader_map_iter(&iter->map->pp->p, iter->map->v, &k, &v, &iter->iter_state))
		return pypointless_value(iter->map->pp, v);

	Py_DECREF(iter->map);
	iter->map = 0;
	return 0;
}

static PyObject* PyPointlessMapItemIter_iternext(PyPointlessMapItemIter* iter)
{
	// iterator already reached end
	if (iter->map == 0)
		return 0;

	pointless_value_t* k = 0;
	pointless_value_t* v = 0;

	if (pointless_reader_map_iter(&iter->map->pp->p, iter->map->v, &k, &v, &iter->iter_state)) {
		PyObject* kk = pypointless_value(iter->map->pp, k);
		PyObject* vv = pypointless_value(iter->map->pp, v);

		if (kk == 0 || vv == 0) {
			Py_XDECREF(kk);
			Py_XDECREF(vv);
			return 0;
		}

		return Py_BuildValue("(NN)", kk, vv);
	}

	Py_DECREF(iter->map);
	iter->map = 0;
	return 0;
}

static uint32_t PyPointlessMap_eq_cb(pointless_t* p, pointless_complete_value_t* v, void* user, const char** error)
{
	return pypointless_cmp_eq(p, v, (PyObject*)user, error);
}

static int PyPointlessMap_contains_(PyPointlessMap* m, PyObject* key)
{
	const char* error = 0;
	uint32_t hash = pyobject_hash_32(key, m->pp->p.header->version, &error);

	if (error) {
		PyErr_Format(PyExc_ValueError, "pointless hash error: %s", error);
		return -1;
	}

	pointless_value_t* k = 0;
	pointless_value_t* v = 0;

	pointless_reader_map_lookup_ext(&m->pp->p, m->v, hash, PyPointlessMap_eq_cb, (void*)key, &k, &v, &error);

	if (error) {
		PyErr_Format(PyExc_ValueError, "pointless map query error: %s", error);
		return -1;
	}

	return (k != 0);
}

static PyObject* PyPointlessMap_contains(PyPointlessMap* m, PyObject* k)
{
	int i = PyPointlessMap_contains_(m, k);

	if (i == -1)
		return 0;

	return PyBool_FromLong(i);
}

static PyObject* PyPointlessMap_keys(PyPointlessMap* m)
{
	PyPointlessMapKeyIter* iter = (PyPointlessMapKeyIter*)PyObject_New(PyPointlessMapKeyIter, &PyPointlessMapKeyIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(m);
	iter->map = m;
	iter->iter_state = 0;

	return (PyObject*)iter;
}

static PyObject* PyPointlessMap_values(PyPointlessMap* m)
{
	PyPointlessMapValueIter* iter = (PyPointlessMapValueIter*)PyObject_New(PyPointlessMapValueIter, &PyPointlessMapValueIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(m);
	iter->map = m;
	iter->iter_state = 0;

	return (PyObject*)iter;
}

static PyObject* PyPointlessMap_items(PyPointlessMap* m)
{
	PyPointlessMapItemIter* iter = (PyPointlessMapItemIter*)PyObject_New(PyPointlessMapItemIter, &PyPointlessMapItemIterType);

	if (iter == 0)
		return 0;

	Py_INCREF(m);
	iter->map = m;
	iter->iter_state = 0;

	return (PyObject*)iter;
}

static PyMemberDef PyPointlessMap_memberlist[] = {
	{"container_id",  T_ULONG, offsetof(PyPointlessMap, container_id), READONLY},
	{NULL}
};

static PyObject* PyPointlessMap_subscript(PyPointlessMap* m, PyObject* key)
{
	const char* error = 0;
	uint32_t hash = pyobject_hash_32(key, m->pp->p.header->version, &error);

	if (error) {
		PyErr_Format(PyExc_ValueError, "pointless hash error: %s", error);
		return 0;
	}

	pointless_value_t* k = 0;
	pointless_value_t* v = 0;

	pointless_reader_map_lookup_ext(&m->pp->p, m->v, hash, PyPointlessMap_eq_cb, (void*)key, &k, &v, &error);

	if (error) {
		PyErr_Format(PyExc_ValueError, "pointless map query error: %s", error);
		return 0;
	}

	if (k == 0) {
		PyErr_SetObject(PyExc_KeyError, key);
		return 0;
	}

	return pypointless_value(m->pp, v);
}

static PyMappingMethods PyPointlessMap_as_mapping = {
	(lenfunc)PyPointlessMap_length,       /*mp_length*/
	(binaryfunc)PyPointlessMap_subscript, /*mp_subscript*/
	(objobjargproc)0,                     /*mp_ass_subscript*/
};

static PyObject* PyPointlessMap_get(PyPointlessMap* m, PyObject* args)
{
	PyObject* key;
	PyObject* failobj = Py_None;

	if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &failobj))
		return NULL;

	const char* error = 0;
	uint32_t hash = pyobject_hash_32(key, m->pp->p.header->version, &error);

	if (error) {
		PyErr_Format(PyExc_ValueError, "pointless hash error: %s", error);
		return 0;
	}

	pointless_value_t* k = 0;
	pointless_value_t* v = 0;

	pointless_reader_map_lookup_ext(&m->pp->p, m->v, hash, PyPointlessMap_eq_cb, (void*)key, &k, &v, &error);

	if (error) {
		PyErr_Format(PyExc_ValueError, "pointless map query error: %s", error);
		return 0;
	}

	if (v == 0) {
		Py_INCREF(failobj);
		return failobj;
	}

	return pypointless_value(m->pp, v);
}


static PyMethodDef PyPointlessMap_methods[] = {
	{"__contains__", (PyCFunction)PyPointlessMap_contains,    METH_O | METH_COEXIST, ""},
	{"__getitem__",  (PyCFunction)PyPointlessMap_subscript,   METH_O | METH_COEXIST, ""},
	{"get",          (PyCFunction)PyPointlessMap_get,         METH_VARARGS, ""},
	{"keys",         (PyCFunction)PyPointlessMap_keys,        METH_NOARGS, ""},
	{"values",       (PyCFunction)PyPointlessMap_values,      METH_NOARGS, ""},
	{"items",        (PyCFunction)PyPointlessMap_items,       METH_NOARGS, ""},
	{NULL, NULL}
};

static PySequenceMethods PyPointlessMap_as_sequence = {
	0,                       /* sq_length */
	0,                       /* sq_concat */
	0,                       /* sq_repeat */
	0,                       /* sq_item */
	0,                       /* sq_slice */
	0,                       /* sq_ass_item */
	0,                       /* sq_ass_slice */
	(objobjproc)PyPointlessMap_contains_, /* sq_contains */
};

PyTypeObject PyPointlessMapType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessMap",          /*tp_name*/
	sizeof(PyPointlessMap),              /*tp_basicsize*/
	0,                                   /*tp_itemsize*/
	(destructor)PyPointlessMap_dealloc,  /*tp_dealloc*/
	0,                                   /*tp_print*/
	0,                                   /*tp_getattr*/
	0,                                   /*tp_setattr*/
	0,                                   /*tp_compare*/
	PyPointless_repr,                    /*tp_repr*/
	0,                                   /*tp_as_number*/
	&PyPointlessMap_as_sequence,         /*tp_as_sequence*/
	&PyPointlessMap_as_mapping,          /*tp_as_mapping*/
	0,                                   /*tp_hash */
	0,                                   /*tp_call*/
	PyPointless_str,                     /*tp_str*/
	0,                                   /*tp_getattro*/
	0,                                   /*tp_setattro*/
	0,                                   /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                  /*tp_flags*/
	"PyPointlessMap wrapper",            /*tp_doc */
	0,                                   /*tp_traverse */
	0,                                   /*tp_clear */
	0,                                   /*tp_richcompare */
	0,                                   /*tp_weaklistoffset */
	PyPointlessMap_iter,                 /*tp_iter */
	0,                                   /*tp_iternext */
	PyPointlessMap_methods,              /*tp_methods */
	PyPointlessMap_memberlist,           /*tp_members */
	0,                                   /*tp_getset */
	0,                                   /*tp_base */
	0,                                   /*tp_dict */
	0,                                   /*tp_descr_get */
	0,                                   /*tp_descr_set */
	0,                                   /*tp_dictoffset */
	(initproc)PyPointlessMap_init,       /*tp_init */
	0,                                   /*tp_alloc */
	PyPointlessMap_new,                  /*tp_new */
};

PyTypeObject PyPointlessMapKeyIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessMapKeyIter",            /*tp_name*/
	sizeof(PyPointlessMapKeyIter),                /*tp_basicsize*/
	0,                                            /*tp_itemsize*/
	(destructor)PyPointlessMapKeyIter_dealloc,    /*tp_dealloc*/
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
	"PyPointlessMapKeyIter",                      /*tp_doc */
	0,                                            /*tp_traverse */
	0,                                            /*tp_clear */
	0,                                            /*tp_richcompare */
	0,                                            /*tp_weaklistoffset */
	PyObject_SelfIter,                            /*tp_iter */
	(iternextfunc)PyPointlessMapKeyIter_iternext, /*tp_iternext */
	0,                                            /*tp_methods */
	0,                                            /*tp_members */
	0,                                            /*tp_getset */
	0,                                            /*tp_base */
	0,                                            /*tp_dict */
	0,                                            /*tp_descr_get */
	0,                                            /*tp_descr_set */
	0,                                            /*tp_dictoffset */
	(initproc)PyPointlessMapKeyIter_init,         /*tp_init */
	0,                                            /*tp_alloc */
	PyPointlessMapKeyIter_new,                    /*tp_new */
};

PyTypeObject PyPointlessMapValueIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessMapValueIter",            /*tp_name*/
	sizeof(PyPointlessMapValueIter),                /*tp_basicsize*/
	0,                                              /*tp_itemsize*/
	(destructor)PyPointlessMapValueIter_dealloc,    /*tp_dealloc*/
	0,                                              /*tp_print*/
	0,                                              /*tp_getattr*/
	0,                                              /*tp_setattr*/
	0,                                              /*tp_compare*/
	0,                                              /*tp_repr*/
	0,                                              /*tp_as_number*/
	0,                                              /*tp_as_sequence*/
	0,                                              /*tp_as_mapping*/
	0,                                              /*tp_hash */
	0,                                              /*tp_call*/
	0,                                              /*tp_str*/
	0,                                              /*tp_getattro*/
	0,                                              /*tp_setattro*/
	0,                                              /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                             /*tp_flags*/
	"PyPointlessMapValueIter",                      /*tp_doc */
	0,                                              /*tp_traverse */
	0,                                              /*tp_clear */
	0,                                              /*tp_richcompare */
	0,                                              /*tp_weaklistoffset */
	PyObject_SelfIter,                              /*tp_iter */
	(iternextfunc)PyPointlessMapValueIter_iternext, /*tp_iternext */
	0,                                              /*tp_methods */
	0,                                              /*tp_members */
	0,                                              /*tp_getset */
	0,                                              /*tp_base */
	0,                                              /*tp_dict */
	0,                                              /*tp_descr_get */
	0,                                              /*tp_descr_set */
	0,                                              /*tp_dictoffset */
	(initproc)PyPointlessMapValueIter_init,         /*tp_init */
	0,                                              /*tp_alloc */
	PyPointlessMapValueIter_new,                    /*tp_new */
};

PyTypeObject PyPointlessMapItemIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pointless.PyPointlessMapItemIter",            /*tp_name*/
	sizeof(PyPointlessMapItemIter),                /*tp_basicsize*/
	0,                                             /*tp_itemsize*/
	(destructor)PyPointlessMapItemIter_dealloc,    /*tp_dealloc*/
	0,                                             /*tp_print*/
	0,                                             /*tp_getattr*/
	0,                                             /*tp_setattr*/
	0,                                             /*tp_compare*/
	0,                                             /*tp_repr*/
	0,                                             /*tp_as_number*/
	0,                                             /*tp_as_sequence*/
	0,                                             /*tp_as_mapping*/
	0,                                             /*tp_hash */
	0,                                             /*tp_call*/
	0,                                             /*tp_str*/
	0,                                             /*tp_getattro*/
	0,                                             /*tp_setattro*/
	0,                                             /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,                            /*tp_flags*/
	"PyPointlessMapItemIter",                      /*tp_doc */
	0,                                             /*tp_traverse */
	0,                                             /*tp_clear */
	0,                                             /*tp_richcompare */
	0,                                             /*tp_weaklistoffset */
	PyObject_SelfIter,                             /*tp_iter */
	(iternextfunc)PyPointlessMapItemIter_iternext, /*tp_iternext */
	0,                                             /*tp_methods */
	0,                                             /*tp_members */
	0,                                             /*tp_getset */
	0,                                             /*tp_base */
	0,                                             /*tp_dict */
	0,                                             /*tp_descr_get */
	0,                                             /*tp_descr_set */
	0,                                             /*tp_dictoffset */
	(initproc)PyPointlessMapItemIter_init,         /*tp_init */
	0,                                             /*tp_alloc */
	PyPointlessMapItemIter_new,                    /*tp_new */
};

PyPointlessMap* PyPointlessMap_New(PyPointless* pp, pointless_value_t* v)
{
	PyPointlessMap* pv = PyObject_New(PyPointlessMap, &PyPointlessMapType);

	if (pv == 0)
		return 0;

	Py_INCREF(pp);
	pp->n_map_refs += 1;
	pv->pp = pp;
	pv->v = v;

	// the container ID is a unique between all non-empty vectors, maps and sets
	pv->container_id = pointless_container_id(&pp->p, v);

	return pv;
}
