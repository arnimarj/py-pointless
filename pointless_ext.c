#include "pointless_ext.h"

uint32_t PyPointlessBitvector_is_set(PyPointlessBitvector* self, uint32_t i);
uint32_t PyPointlessBitvector_n_items(PyPointlessBitvector* self);

struct pointless_module_state {
//	PyObject *error;
};

#define GETSTATE(m) ((struct pointless_module_state*)PyModule_GetState(m))

static struct PyPointless_CAPI CAPI = {
	POINTLESS_API_MAGIC,
	sizeof(struct PyPointless_CAPI),
	pointless_dynarray_init,
	pointless_dynarray_n_items,
	pointless_dynarray_pop,
	pointless_dynarray_push,
	pointless_dynarray_push_bulk,
	pointless_dynarray_clear,
	pointless_dynarray_destroy,
	pointless_dynarray_item_at,
	pointless_calloc,
	pointless_malloc,
	pointless_free,
	pointless_realloc,
	pointless_strdup,
	PyPointlessPrimVector_from_T_vector,
	PyPointlessPrimVector_from_buffer,
	bentley_sort_,
	&PyPointlessType,
	&PyPointlessVectorType,
	&PyPointlessBitvectorType,
	&PyPointlessSetType,
	&PyPointlessMapType,
	&PyPointlessPrimVectorType,
	PyPointlessBitvector_is_set,
	PyPointlessBitvector_n_items,
	pypointless_value
};

PyObject* pointless_write_object(PyObject* self, PyObject* args, PyObject* kwds);
extern const char pointless_write_object_doc[];

PyObject* pointless_write_object_to_primvector(PyObject* self, PyObject* args, PyObject* kwds);
PyObject* pointless_write_object_to_bytearray(PyObject* self, PyObject* args, PyObject* kwds);
extern const char pointless_write_object_to_buffer_doc[];

PyObject* pointless_pyobject_hash_32(PyObject* self, PyObject* args);
extern const char pointless_pyobject_hash_32_doc[];

PyObject* pointless_cmp(PyObject* self, PyObject* args);
extern const char pointless_cmp_doc[];

PyObject* pointless_is_eq(PyObject* self, PyObject* args);
extern const char pointless_is_eq_doc[];

static PyMethodDef pointless_module_methods[] =
{
	{"serialize",              (PyCFunction)pointless_write_object,               METH_VARARGS | METH_KEYWORDS, pointless_write_object_doc               },
	{"serialize_to_buffer",    (PyCFunction)pointless_write_object_to_primvector, METH_VARARGS | METH_KEYWORDS, pointless_write_object_to_buffer_doc     },
	{"serialize_to_bytearray", (PyCFunction)pointless_write_object_to_bytearray,  METH_VARARGS | METH_KEYWORDS, pointless_write_object_to_buffer_doc     },
	{"pyobject_hash",          (PyCFunction)pointless_pyobject_hash_32,           METH_VARARGS,                 pointless_pyobject_hash_32_doc           },
	{"pyobject_hash_32",       (PyCFunction)pointless_pyobject_hash_32,           METH_VARARGS,                 pointless_pyobject_hash_32_doc           },
	{"pointless_cmp",          (PyCFunction)pointless_cmp,                        METH_VARARGS,                 pointless_cmp_doc                        },
	{"pointless_is_eq",        (PyCFunction)pointless_is_eq,                      METH_VARARGS,                 pointless_is_eq_doc                      },
	{NULL, NULL},
};

static int pointless_module_traverse(PyObject* m, visitproc visit, void* arg) {
//	Py_VISIT(GETSTATE(m)->error);
	return 0;
}

static int pointless_module_clear(PyObject* m) {
//	Py_CLEAR(GETSTATE(m)->error);
	return 0;
}

static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"pointless",
	NULL,
	sizeof(struct pointless_module_state),
	pointless_module_methods,
	NULL,
	pointless_module_traverse,
	pointless_module_clear,
	NULL
};

#define MODULEINITERROR return NULL
PyMODINIT_FUNC

PyInit_pointless(void)
{
	if (sizeof(Word_t) != sizeof(void*)) {
		PyErr_SetString(PyExc_ValueError, "word size mismatch");
		MODULEINITERROR;
	}

	PyObject* module_pointless = 0;

	module_pointless = PyModule_Create(&moduledef);

	if (module_pointless == 0) {
		MODULEINITERROR;
	}

	struct {
		PyTypeObject* type;
		const char* name;
	} types[15] = {
		{&PyPointlessType,                  "Pointless"                  },
		{&PyPointlessVectorType,            "PointlessVector"            },
		{&PyPointlessVectorIterType,        "PointlessVectorIter"        },
		{&PyPointlessVectorRevIterType,     "PointlessVectorRevIter"     },
		{&PyPointlessBitvectorType,         "PointlessBitvector"         },
		{&PyPointlessBitvectorIterType,     "PointlessBitvectorIter"     },
		{&PyPointlessSetType,               "PointlessSet"               },
		{&PyPointlessSetIterType,           "PointlessSetIter"           },
		{&PyPointlessMapType,               "PointlessMap"               },
		{&PyPointlessMapKeyIterType,        "PointlessMapKeyIter"        },
		{&PyPointlessMapValueIterType,      "PointlessMapValueIter"      },
		{&PyPointlessMapItemIterType,       "PointlessMapItemIter"       },
		{&PyPointlessPrimVectorType,        "PointlessPrimVector"        },
		{&PyPointlessPrimVectorIterType,    "PointlessPrimVectorIter"    },
		{&PyPointlessPrimVectorRevIterType, "PointlessPrimVectorRevIter" },
	};

	int i;

	for (i = 0; i < 15; i++) {
		if (PyType_Ready(types[i].type) < 0) {
			Py_DECREF(module_pointless);
			MODULEINITERROR;
		}

		Py_INCREF((PyObject*)types[i].type);

		if (PyModule_AddObject(module_pointless, types[i].name, (PyObject*)types[i].type) != 0) {
			Py_DECREF(module_pointless);
			MODULEINITERROR;
		}
	}

	PyObject* c_api = PyCapsule_New(&CAPI, "pointless_CAPI", 0);

	if (c_api == 0) {
		Py_DECREF(module_pointless);
		MODULEINITERROR;
	}

	if (PyCapsule_SetContext(c_api, (void*)POINTLESS_MAGIC_CONTEXT) != 0) {
		Py_DECREF(module_pointless);
		MODULEINITERROR;
	}

	if (PyModule_AddObject(module_pointless, "pointless_CAPI", c_api) != 0) {
		Py_DECREF(module_pointless);
		MODULEINITERROR;
	}

    return module_pointless;
}

#undef MODULEINITERROR
