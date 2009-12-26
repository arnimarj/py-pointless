#include "../pointless.h"

static int64_t pointless_prim_vector_int_item_at(PyPointlessPrimVector* v, uint32_t i)
{
	void* data = pointless_dynarray_item_at(&v->array, i);

	switch (v->type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			return (int64_t)(*((int8_t*)data));
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			return (int64_t)(*((uint8_t*)data));
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			return (int64_t)(*((int16_t*)data));
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			return (int64_t)(*((uint16_t*)data));
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			return (int64_t)(*((int32_t*)data));
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			return (int64_t)(*((uint32_t*)data));
	}

	assert(0);
	return 0;
}

typedef struct {
	uint32_t offset;

	PyPointlessPrimVector* array;
	PyPointlessPrimVector* key_array;

	uint32_t store;
} sort_state_t;

// sort time key-comparison
#define MY_CMP_RET(T)\
	sort_state_t* state = (sort_state_t*)user;\
	uint32_t* aa = (uint32_t*)(state->array->array._data);\
	T* data = (T*)(state->key_array->array._data);\
	uint32_t ia = aa[a+state->offset];\
	uint32_t ib = aa[b+state->offset];\
	*c = SIMPLE_CMP(data[ia], data[ib]);\
	return 1;

static int my_cmp_i8(int a, int b, int* c, void* user)
	{ MY_CMP_RET(int8_t); }
static int my_cmp_u8(int a, int b, int* c, void* user)
	{ MY_CMP_RET(uint8_t); }
static int my_cmp_i16(int a, int b, int* c, void* user)
	{ MY_CMP_RET(int16_t); }
static int my_cmp_u16(int a, int b, int* c, void* user)
	{ MY_CMP_RET(uint16_t); }
static int my_cmp_i32(int a, int b, int* c, void* user)
	{ MY_CMP_RET(int32_t); }
static int my_cmp_u32(int a, int b, int* c, void* user)
	{ MY_CMP_RET(uint32_t); }
static int my_cmp_f(int a, int b, int* c, void* user)
	{ MY_CMP_RET(float); }

static void my_swap(int a, int b, void* user)
{
	sort_state_t* state = (sort_state_t*)user;
	a += state->offset;
	b += state->offset;
	uint32_t* aa = (uint32_t*)(state->array->array._data);
	uint32_t t = aa[a];
	aa[a] = aa[b];
	aa[b] = t;
}

static qsort_cmp pointless_db_utils_get_cmp(uint32_t type)
{
	switch (type) {
		case POINTLESS_PRIM_VECTOR_TYPE_I8:
			return my_cmp_i8;
		case POINTLESS_PRIM_VECTOR_TYPE_U8:
			return my_cmp_u8;
		case POINTLESS_PRIM_VECTOR_TYPE_I16:
			return my_cmp_i16;
		case POINTLESS_PRIM_VECTOR_TYPE_U16:
			return my_cmp_u16;
		case POINTLESS_PRIM_VECTOR_TYPE_I32:
			return my_cmp_i32;
		case POINTLESS_PRIM_VECTOR_TYPE_U32:
			return my_cmp_u32;
		case POINTLESS_PRIM_VECTOR_TYPE_FLOAT:
			return my_cmp_f;
	}

	assert(0);
	return 0;
}

const char pointless_db_array_sort_doc[] =
"pointless_db_array_sort(array, key_array)\n"
"\n"
"Sorts 'array', which contains indices into the arrays in 'key_array'. The sort keys are the keys"
"in 'key_array', in the order they appear.\n"
"\n"
"  array: the indice array\n"
"  key_array: a list primitive value vector\n"
;

PyObject* pointless_db_array_sort(PyObject* o, PyObject* args)
{
	PyPointlessPrimVector* array = 0;
	PyObject* key_array = 0;

	if (!PyArg_ParseTuple(args, "O!O!", &PyPointlessPrimVectorType, &array, &PyList_Type, &key_array)) {
		PyErr_Format(PyExc_TypeError, "%s: (array, key_array)", __func__);
		return 0;
	}

	if (array->type != POINTLESS_PRIM_VECTOR_TYPE_U32) {
		PyErr_Format(PyExc_TypeError, "%s: array must be u32", __func__);
		return 0;
	}

	Py_ssize_t i;
	uint32_t k;

	uint32_t n_items_0 = 0, n_items_i = 0;

	for (i = 0; i < PyList_GET_SIZE(key_array); i++) {
		PyPointlessPrimVector* v_0 = (PyPointlessPrimVector*)PyList_GET_ITEM(key_array, 0);
		PyPointlessPrimVector* v_i = (PyPointlessPrimVector*)PyList_GET_ITEM(key_array, i);

		if (!PyPointlessPrimVector_Check(v_i)) {
			PyErr_Format(PyExc_TypeError, "%s: key_array must be a list primitive vectors", __func__);
			return 0;
		}

		n_items_0 = pointless_dynarray_n_items(&v_0->array);
		n_items_i = pointless_dynarray_n_items(&v_i->array);

		if (n_items_0 != n_items_i) {
			PyErr_Format(PyExc_ValueError, "%s: all vectors must be of the same length", __func__);
			return 0;
		}
	}

	for (k = 0; k < pointless_dynarray_n_items(&array->array); k++) {
		int64_t v = pointless_prim_vector_int_item_at(array, k);

		if (!(0 <= v && v < (int64_t)n_items_0)) {
			PyErr_Format(PyExc_TypeError, "%s: array element out of bounds", __func__);
			return 0;
		}
	}

	if (PyList_GET_SIZE(key_array) == 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	// sort on first key
	sort_state_t state;
	state.offset = 0;
	state.array = array;
	state.key_array = (PyPointlessPrimVector*)PyList_GET_ITEM(key_array, 0);

	qsort_cmp cur_cmp = pointless_db_utils_get_cmp(state.key_array->type);
	qsort_cmp prev_cmp = 0;

	bentley_sort((int)pointless_dynarray_n_items(&array->array), cur_cmp, my_swap, (void*)&state);

	// sort on remaining keys
	uint32_t n_array = pointless_dynarray_n_items(&array->array);

	for (i = 1; i < PyList_GET_SIZE(key_array); i++) {
		uint32_t lo = 0, hi = 0;

		PyPointlessPrimVector* prev_key = (PyPointlessPrimVector*)PyList_GET_ITEM(key_array, i - 1);
		PyPointlessPrimVector* cur_key = (PyPointlessPrimVector*)PyList_GET_ITEM(key_array, i);

		prev_cmp = pointless_db_utils_get_cmp(prev_key->type);
		cur_cmp = pointless_db_utils_get_cmp(cur_key->type);

		while (lo < n_array) {
			hi = lo + 1;

			state.offset = 0;
			state.key_array = prev_key;

			while (hi < n_array) {
				int cc = 0;

				(*prev_cmp)(lo, hi, &cc, (void*)&state);

				if (cc)
					break;

				hi++;
			}

			// sort subset
			state.offset = lo;
			state.key_array = cur_key;

			bentley_sort(hi - lo, cur_cmp, my_swap, (void*)&state);

			lo = hi;
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}
