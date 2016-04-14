#include "pointless_ext.h"

uint32_t PyPointlessBitvector_is_set(PyPointlessBitvector* self, uint32_t i);
uint32_t PyPointlessBitvector_n_items(PyPointlessBitvector* self);


static PyPointless_CAPI CAPI = {
	pointless_dynarray_init,
	pointless_dynarray_n_items,
	pointless_dynarray_pop,
	pointless_dynarray_push,
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

PyObject* pointless_write_object_to_buffer(PyObject* self, PyObject* args, PyObject* kwds);
extern const char pointless_write_object_to_buffer_doc[];

PyObject* pointless_pyobject_hash_32(PyObject* self, PyObject* args);
extern const char pointless_pyobject_hash_32_doc[];

PyObject* pointless_cmp(PyObject* self, PyObject* args);
extern const char pointless_cmp_doc[];

PyObject* pointless_is_eq(PyObject* self, PyObject* args);
extern const char pointless_is_eq_doc[];

static PyMethodDef pointless_methods[] =
{
	{"serialize",           (PyCFunction)pointless_write_object,           METH_VARARGS | METH_KEYWORDS, pointless_write_object_doc               },
	{"serialize_to_buffer", (PyCFunction)pointless_write_object_to_buffer, METH_VARARGS | METH_KEYWORDS, pointless_write_object_to_buffer_doc     },
	{"pyobject_hash",       (PyCFunction)pointless_pyobject_hash_32,       METH_VARARGS,                 pointless_pyobject_hash_32_doc           },
	{"pyobject_hash_32",    (PyCFunction)pointless_pyobject_hash_32,       METH_VARARGS,                 pointless_pyobject_hash_32_doc           },
	{"pointless_cmp",       (PyCFunction)pointless_cmp,                    METH_VARARGS,                 pointless_cmp_doc                        },
	{"pointless_is_eq",     (PyCFunction)pointless_is_eq,                  METH_VARARGS,                 pointless_is_eq_doc                      },
	{NULL, NULL},
};

PyMODINIT_FUNC
initpointless(void)
{
	if (sizeof(Word_t) != sizeof(void*)) {
		PyErr_SetString(PyExc_ValueError, "word size mismatch");
		return;
	}

	PyObject* module_pointless = 0;

	if ((module_pointless = Py_InitModule4("pointless", pointless_methods, "Pointless Python API", 0, PYTHON_API_VERSION)) == 0)
		return;

	struct {
		PyTypeObject* type;
		const char* name;
	} types[13] = {
		{&PyPointlessType,               "Pointless"               },
		{&PyPointlessVectorType,         "PointlessVector"         },
		{&PyPointlessVectorIterType,     "PointlessVectorIter"     },
		{&PyPointlessBitvectorType,      "PointlessBitvector"      },
		{&PyPointlessBitvectorIterType,  "PointlessBitvectorIter"  },
		{&PyPointlessSetType,            "PointlessSet"            },
		{&PyPointlessSetIterType,        "PointlessSetIter"        },
		{&PyPointlessMapType,            "PointlessMap"            },
		{&PyPointlessMapKeyIterType,     "PointlessMapKeyIter"     },
		{&PyPointlessMapValueIterType,   "PointlessMapValueIter"   },
		{&PyPointlessMapItemIterType,    "PointlessMapItemIter"    },
		{&PyPointlessPrimVectorType,     "PointlessPrimVector"     },
		{&PyPointlessPrimVectorIterType, "PointlessPrimVectorIter" }
	};

	int i;

	for (i = 0; i < 13; i++) {
		if (PyType_Ready(types[i].type) < 0)
			return;

		Py_INCREF((PyObject*)types[i].type);

		if (PyModule_AddObject(module_pointless, types[i].name, (PyObject*)types[i].type) != 0)
			return;
	}

	PyObject* c_api = PyCObject_FromVoidPtrAndDesc(&CAPI, (void*)POINTLESS_API_MAGIC, NULL);

	if (c_api == 0)
		return;

	if (PyModule_AddObject(module_pointless, "pointless_CAPI", c_api) != 0)
		return;
}
