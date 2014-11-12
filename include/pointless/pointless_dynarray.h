#ifndef __POINTLESS__DYNARRAY__H__
#define __POINTLESS__DYNARRAY__H__

#include <stdlib.h>
#include <assert.h>

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

#include <pointless/pointless_malloc.h>
#include <pointless/pointless_int_ops.h>

typedef struct {
	void* _data;
	size_t n_items;
	size_t n_alloc;
	size_t item_size;
} pointless_dynarray_t;

#define pointless_dynarray_ITEM_AT(T, A, I) ((T*)(A)->_data)[I]

void pointless_dynarray_init(pointless_dynarray_t* a, size_t item_size);
size_t pointless_dynarray_n_items(pointless_dynarray_t* a);
size_t pointless_dynarray_n_heap_bytes(pointless_dynarray_t* a);
void pointless_dynarray_pop(pointless_dynarray_t* a);
int pointless_dynarray_push(pointless_dynarray_t* a, void* i);
int pointless_dynarray_push_bulk(pointless_dynarray_t* a, void* i, size_t n_items);
void pointless_dynarray_clear(pointless_dynarray_t* a);
void pointless_dynarray_destroy(pointless_dynarray_t* a);
void* pointless_dynarray_item_at(pointless_dynarray_t* a, size_t i);
void* pointless_dynarray_buffer(pointless_dynarray_t* a);
void pointless_dynarray_swap(pointless_dynarray_t* a, size_t i, size_t j);
void pointless_dynarray_give_data(pointless_dynarray_t* a, void* data, size_t n_items);

#endif
