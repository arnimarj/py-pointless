#include "pointless_ext.h"

int PyPointless_IMPORT(int magic)
{
	// 0) already imported, and bad magic
	if (PyPointlessAPI != 0 && PyPointlessAPI_magic != magic) {
		PyErr_SetString(PyExc_ImportError, "pointless already imported with some other magic");
		return 0;
	}

	if (PyPointlessAPI)
		return 1;

	// 1) import module
	PyObject* m = PyImport_ImportModule("pointless");

	if (m == 0) {
		PyPointlessAPI = 0;
		return 0;
	}

	// 2) get CAPI attribute
	PyObject* c = PyObject_GetAttrString(m, "pointless_CAPI");

	if (c == 0) {
		PyPointlessAPI = 0;
		return 0;
	}

	PyPointlessAPI = PyCObject_AsVoidPtr(c);
	Py_DECREF(c);

	if (PyPointlessAPI == 0)
		return 0;

	void* desc = PyCObject_GetDesc(c);

	if (desc == 0) {
		PyPointlessAPI = 0;
		return 0;
	}

	if (desc != (void*)POINTLESS_API_MAGIC) {
		PyErr_SetString(PyExc_ImportError, "pointless magic does not match");
		PyPointlessAPI = 0;
		return 0;
	}

	return 1;
}

