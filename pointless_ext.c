#include "pointless_ext.h"

static PyPointless_CAPI CAPI = {
	pointless_dynarray_init,
	pointless_dynarray_n_items,
	pointless_dynarray_pop,
	pointless_dynarray_push,
	pointless_dynarray_clear,
	pointless_dynarray_destroy,
	pointless_calloc,
	pointless_malloc,
	pointless_free,
	pointless_realloc,
	PyPointlessPrimVector_from_T_vector,
	PyPointlessPrimVector_from_buffer,
	&PyPointlessType,
	&PyPointlessVectorType,
	&PyPointlessBitvectorType,
	&PyPointlessSetType,
	&PyPointlessMapType,
	&PyPointlessPrimVectorType
};

static PyMethodDef pointless_methods[] =
{
	POINTLESS_FUNC_DEF("serialize",       pointless_write_object),
	POINTLESS_FUNC_DEF("pyobject_hash",   pointless_pyobject_hash),
	POINTLESS_FUNC_DEF("pointless_cmp",   pointless_cmp),
	POINTLESS_FUNC_DEF("pointless_is_eq", pointless_is_eq),
	POINTLESS_FUNC_DEF("db_array_sort",   pointless_db_array_sort),
	{NULL, NULL},
};

// helpers
#define TRY_MODULE_INIT(module, description) PyObject* module##_module = 0; if ((module##_module = Py_InitModule4(#module, module##_methods, description, 0, PYTHON_API_VERSION)) == 0) { return; }
#define TRY_MODULE_ADD_OBJECT(module, key, value) if (PyModule_AddObject(module, key, value) != 0) return;

PyMODINIT_FUNC
initpointless(void)
{
	TRY_MODULE_INIT(pointless, "Pointless Python API");

	if (PyType_Ready(&PyPointlessType) < 0)
		return;

	if (PyType_Ready(&PyPointlessVectorType) < 0)
		return;

	if (PyType_Ready(&PyPointlessVectorIterType) < 0)
		return;

	if (PyType_Ready(&PyPointlessBitvectorType) < 0)
		return;

	if (PyType_Ready(&PyPointlessBitvectorIterType) < 0)
		return;

	if (PyType_Ready(&PyPointlessSetType) < 0)
		return;

	if (PyType_Ready(&PyPointlessSetIterType) < 0)
		return;

	if (PyType_Ready(&PyPointlessMapType) < 0)
		return;

	if (PyType_Ready(&PyPointlessMapKeyIterType) < 0)
		return;

	if (PyType_Ready(&PyPointlessMapValueIterType) < 0)
		return;

	if (PyType_Ready(&PyPointlessMapItemIterType) < 0)
		return;

	if (PyType_Ready(&PyPointlessPrimVectorType) < 0)
		return;

	if (PyType_Ready(&PyPointlessPrimVectorIterType) < 0)
		return;

	// add custom types to the different modules
	Py_INCREF(&PyPointlessType);
	Py_INCREF(&PyPointlessVectorType);
	Py_INCREF(&PyPointlessVectorIterType);
	Py_INCREF(&PyPointlessBitvectorType);
	Py_INCREF(&PyPointlessBitvectorIterType);
	Py_INCREF(&PyPointlessSetType);
	Py_INCREF(&PyPointlessSetIterType);
	Py_INCREF(&PyPointlessMapType);
	Py_INCREF(&PyPointlessMapKeyIterType);
	Py_INCREF(&PyPointlessMapValueIterType);
	Py_INCREF(&PyPointlessMapItemIterType);
	Py_INCREF(&PyPointlessPrimVectorType);
	Py_INCREF(&PyPointlessPrimVectorIterType);

	TRY_MODULE_ADD_OBJECT(pointless_module, "Pointless",               (PyObject*)&PyPointlessType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessVector",         (PyObject*)&PyPointlessVectorType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessVectorIter",     (PyObject*)&PyPointlessVectorIterType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessBitvector",      (PyObject*)&PyPointlessBitvectorType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessBitvectorIter",  (PyObject*)&PyPointlessBitvectorIterType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessSet",            (PyObject*)&PyPointlessSetType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessSetIter",        (PyObject*)&PyPointlessSetIterType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessMap",            (PyObject*)&PyPointlessMapType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessMapKeyIter",     (PyObject*)&PyPointlessMapKeyIterType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessMapValueIter",   (PyObject*)&PyPointlessMapValueIterType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessMapItemIter",    (PyObject*)&PyPointlessMapItemIterType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessPrimVector",     (PyObject*)&PyPointlessPrimVectorType);
	TRY_MODULE_ADD_OBJECT(pointless_module, "PointlessPrimVectorIter", (PyObject*)&PyPointlessPrimVectorIterType);

	PyObject* c_api = PyCObject_FromVoidPtrAndDesc(&CAPI, (void*)POINTLESS_API_MAGIC, NULL);

	if (c_api == 0)
		return;

	TRY_MODULE_ADD_OBJECT(pointless_module, "pointless_CAPI", c_api);
}
