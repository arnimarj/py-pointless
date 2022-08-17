#ifndef __POINTLESS__MODULE__H__
#define __POINTLESS__MODULE__H__

#include <Python.h>

#include "structmember.h"

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <pointless/pointless.h>
#include <pointless/pointless_dynarray.h>

STATIC_ASSERT(Py_UNICODE_SIZE == 2 || Py_UNICODE_SIZE == 4, "Py_UNICODE_SIZE must be 2 or 4");

typedef struct {
	PyObject_HEAD
	int is_open;
	int allow_print;
	Py_ssize_t n_root_refs;
	Py_ssize_t n_vector_refs;
	Py_ssize_t n_bitvector_refs;
	Py_ssize_t n_map_refs;
	Py_ssize_t n_set_refs;
	pointless_t p;
} PyPointless;

typedef struct {
	PyObject_HEAD
	PyPointless* pp;
	pointless_value_t* v;
	unsigned long container_id;
	int is_hashable;

	// slice params, must be respected at all times
	uint32_t slice_i;
	uint32_t slice_n;
} PyPointlessVector;

typedef struct {
	PyObject_HEAD
	PyPointlessVector* vector;
	uint32_t iter_state;
} PyPointlessVectorIter;

typedef struct {
	PyObject_HEAD
	PyPointlessVector* vector;
	uint32_t iter_state;
} PyPointlessVectorRevIter;

typedef struct {
	PyObject_HEAD
	int is_pointless;
	int allow_print;

	// pointless stuff
	PyPointless* pointless_pp;
	pointless_value_t* pointless_v;

	// other stuff
	uint32_t primitive_n_bits;
	void* primitive_bits;
	uint32_t primitive_n_bytes_alloc;
	size_t primitive_n_one;
} PyPointlessBitvector;

typedef struct {
	PyObject_HEAD
	PyPointlessBitvector* bitvector;
	uint32_t iter_state;
} PyPointlessBitvectorIter;

typedef struct {
	PyObject_HEAD
	PyPointless* pp;
	pointless_value_t* v;
	unsigned long container_id;
} PyPointlessSet;

typedef struct {
	PyObject_HEAD
	PyPointlessSet* set;
	uint32_t iter_state;
} PyPointlessSetIter;

typedef struct {
	PyObject_HEAD
	PyPointless* pp;
	pointless_value_t* v;
	unsigned long container_id;
} PyPointlessMap;

typedef struct {
	PyObject_HEAD
	PyPointlessMap* map;
	uint32_t iter_state;
} PyPointlessMapKeyIter;

typedef struct {
	PyObject_HEAD
	PyPointlessMap* map;
	uint32_t iter_state;
} PyPointlessMapValueIter;

typedef struct {
	PyObject_HEAD
	PyPointlessMap* map;
	uint32_t iter_state;
} PyPointlessMapItemIter;

#define POINTLESS_PRIM_VECTOR_TYPE_I8 0
#define POINTLESS_PRIM_VECTOR_TYPE_U8 1
#define POINTLESS_PRIM_VECTOR_TYPE_I16 2
#define POINTLESS_PRIM_VECTOR_TYPE_U16 3
#define POINTLESS_PRIM_VECTOR_TYPE_I32 4
#define POINTLESS_PRIM_VECTOR_TYPE_U32 5
#define POINTLESS_PRIM_VECTOR_TYPE_FLOAT 6
#define POINTLESS_PRIM_VECTOR_TYPE_I64 7
#define POINTLESS_PRIM_VECTOR_TYPE_U64 8

typedef struct {
	PyObject_HEAD
	int allow_print;
	int ob_exports;
	// vector
	pointless_dynarray_t array;
	uint8_t type;
} PyPointlessPrimVector;

typedef struct {
	PyObject_HEAD
	PyPointlessPrimVector* vector;
	uint32_t iter_state;
} PyPointlessPrimVectorIter;

typedef struct {
	PyObject_HEAD
	PyPointlessPrimVector* vector;
	uint32_t iter_state;
} PyPointlessPrimVectorRevIter;

PyPointlessVector* PyPointlessVector_New(PyPointless* pp, pointless_value_t* v, uint32_t slice_i, uint32_t slice_n);
PyPointlessBitvector* PyPointlessBitvector_New(PyPointless* pp, pointless_value_t* v);

PyPointlessSet* PyPointlessSet_New(PyPointless* pp, pointless_value_t* v);
PyPointlessMap* PyPointlessMap_New(PyPointless* pp, pointless_value_t* v);

PyObject* pypointless_i32(PyPointless* p, int32_t v);
PyObject* pypointless_u32(PyPointless* p, uint32_t v);
PyObject* pypointless_i64(PyPointless* p, int64_t v);
PyObject* pypointless_u64(PyPointless* p, uint64_t v);
PyObject* pypointless_float(PyPointless* p, float v);
PyObject* pypointless_value_string(pointless_t* p, pointless_value_t* v);
PyObject* pypointless_value_unicode(pointless_t* p, pointless_value_t* v);
PyObject* pypointless_value(PyPointless* p, pointless_value_t* v);

PyObject* PyPointless_str(PyObject* py_object);
PyObject* PyPointless_repr(PyObject* py_object);

uint32_t pypointless_cmp_eq(pointless_t* p, pointless_complete_value_t* v, PyObject* py_object, const char** error);
uint32_t pyobject_hash_32(PyObject* py_object, uint32_t version, const char** error);
uint32_t pointless_pybitvector_hash_32(PyPointlessBitvector* bitvector);

// custom types
extern PyTypeObject PyPointlessType;
extern PyTypeObject PyPointlessVectorType;
extern PyTypeObject PyPointlessVectorIterType;
extern PyTypeObject PyPointlessVectorRevIterType;
extern PyTypeObject PyPointlessBitvectorType;
extern PyTypeObject PyPointlessBitvectorIterType;
extern PyTypeObject PyPointlessSetType;
extern PyTypeObject PyPointlessSetIterType;
extern PyTypeObject PyPointlessMapType;
extern PyTypeObject PyPointlessMapKeyIterType;
extern PyTypeObject PyPointlessMapValueIterType;
extern PyTypeObject PyPointlessMapItemIterType;
extern PyTypeObject PyPointlessPrimVectorType;
extern PyTypeObject PyPointlessPrimVectorIterType;
extern PyTypeObject PyPointlessPrimVectorRevIterType;

#define PyPointless_Check(op) PyObject_TypeCheck(op, &PyPointlessType)
#define PyPointlessVector_Check(op) PyObject_TypeCheck(op, &PyPointlessVectorType)
#define PyPointlessBitvector_Check(op) PyObject_TypeCheck(op, &PyPointlessBitvectorType)
#define PyPointlessSet_Check(op) PyObject_TypeCheck(op, &PyPointlessSetType)
#define PyPointlessMap_Check(op) PyObject_TypeCheck(op, &PyPointlessMapType)
#define PyPointlessPrimVector_Check(op) PyObject_TypeCheck(op, &PyPointlessPrimVectorType)

// C-API
PyPointlessPrimVector* PyPointlessPrimVector_from_T_vector(pointless_dynarray_t* v, uint32_t t);
PyPointlessPrimVector* PyPointlessPrimVector_from_buffer(void* buffer, size_t n_buffer);

#define POINTLESS_API_MAGIC "pointless.pointless_CAPI 1.01"
#define POINTLESS_MAGIC_CONTEXT 0x1ABEEFFF

struct PyPointless_CAPI {
	// version info
	char* magic; // set to POINTLESS_API_MAGIC
	size_t size; // set to sizeof(struct PyPointless_CAPI)

	// prim-vector operations
	void(*primvector_init)(pointless_dynarray_t* a, size_t item_size);
	size_t(*primvector_n_items)(pointless_dynarray_t* a);
	void(*primvector_pop)(pointless_dynarray_t* a);
	int(*primvector_push)(pointless_dynarray_t* a, void* i);
	int(*primvector_push_bulk)(pointless_dynarray_t* a, void* i, size_t n_items);
	void(*primvector_clear)(pointless_dynarray_t* a);
	void(*primvector_destroy)(pointless_dynarray_t* a);
	void*(*primvector_item_at)(pointless_dynarray_t* a, size_t i);

	// malloc routines
	void*(*pointless_calloc)(size_t nmemb, size_t size);
	void*(*pointless_malloc)(size_t size);
	void (*pointless_free)(void* ptr);
	void*(*pointless_realloc)(void* ptr, size_t size);
	char*(*pointless_strdup)(const char* s);

	// prim-vector object constructors
	PyPointlessPrimVector*(*primvector_from_vector)(pointless_dynarray_t* v, uint32_t t);
	PyPointlessPrimVector*(*primvector_from_buffer)(void* buffer, size_t n_buffer);

	// utilities
	int(*pointless_sort)(int n, qsort_cmp_ cmp, qsort_swap_ swap, void* user);

	// types
	PyTypeObject* PyPointlessType_ptr;
	PyTypeObject* PyPointlessVectorType_ptr;
	PyTypeObject* PyPointlessBitvectorType_ptr;
	PyTypeObject* PyPointlessSetType_ptr;
	PyTypeObject* PyPointlessMapType_ptr;
	PyTypeObject* PyPointlessPrimVectorType_ptr;

	// bitvector utilities
	uint32_t(*PyPointlessBitvector_is_set)(PyPointlessBitvector* self, uint32_t i);
	uint32_t(*PyPointlessBitvector_n_items)(PyPointlessBitvector* self);

	// object instantiation
	PyObject*(*create_pypointless_value)(PyPointless* p, pointless_value_t* v);
};

static struct PyPointless_CAPI* PyPointless_IMPORT_CAPI(void)
{
	PyObject* m = PyImport_ImportModule("pointless");

	if (m) {
		PyObject* c = PyObject_GetAttrString(m, "pointless_CAPI");                                                               \

		if (c == 0)
			return 0;

		if (!PyCapsule_IsValid(c, "pointless_CAPI")) {
			Py_DECREF(c);
			PyErr_Format(PyExc_ImportError, "pointless.pointless_API is an invalid capsule");
			return 0;
		}

		void* context = PyCapsule_GetContext(c);

		if (context != (void*)POINTLESS_MAGIC_CONTEXT) {
			Py_DECREF(c);
			PyErr_Format(PyExc_ImportError, "invalid capsule context, was %ld, expected %ld", (long)context, (long)POINTLESS_MAGIC_CONTEXT);
			return 0;
		}

		void* pointer = PyCapsule_GetPointer(c, "pointless_CAPI");
		Py_DECREF(c);
		return pointer;
	}

	return 0;
}

#endif
