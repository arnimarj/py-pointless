#include <pointless/pointless_dynarray.h>

void pointless_dynarray_init(pointless_dynarray_t* a, size_t item_size)
{
	a->_data = 0;
	a->n_items = 0;
	a->n_alloc = 0;
	a->item_size = item_size;
}

size_t pointless_dynarray_n_items(pointless_dynarray_t* a)
{
	return a->n_items;
}

size_t pointless_dynarray_n_heap_bytes(pointless_dynarray_t* a)
{
	return (a->n_alloc * a->item_size);
}

void pointless_dynarray_pop(pointless_dynarray_t* a)
{
	assert(a->n_items > 0);
	a->n_items -= 1;
}

static intop_sizet_t next_size(size_t n_alloc)
{
	size_t small_add[] = {1, 1, 2, 2, 4, 4, 4, 8, 8, 10, 11, 12, 13, 14, 15, 16};
	size_t a = n_alloc / 16;
	size_t b = n_alloc;
	size_t c = (n_alloc < 16) ? small_add[n_alloc] : 0;

	// (a + b) + c
	return intop_sizet_add(intop_sizet_add(intop_sizet_init(a), intop_sizet_init(b)), intop_sizet_init(c));
}

static int pointless_dynarray_grow(pointless_dynarray_t* a)
{
	// get next allocation size, in terms of items and bytes, with overflow check
	intop_sizet_t next_n_alloc = next_size(a->n_alloc);
	intop_sizet_t next_n_bytes = intop_sizet_mult(next_n_alloc, intop_sizet_init(a->item_size));

	if (next_n_bytes.is_overflow)
		return 0;

	// we're good
	void* next_data = pointless_realloc(a->_data, next_n_bytes.value);

	if (next_data == 0)
		return 0;

	a->_data = next_data;
	a->n_alloc = next_n_alloc.value;

	return 1;
}

int pointless_dynarray_push(pointless_dynarray_t* a, void* i)
{
	return pointless_dynarray_push_bulk(a, i, 1);
}

int pointless_dynarray_push_bulk(pointless_dynarray_t* a, void* i, size_t n_items)
{
	while (a->n_items + n_items > a->n_alloc) {
		if (!pointless_dynarray_grow(a))
			return 0;
	}

	memcpy((char*)a->_data + a->item_size * a->n_items, i, a->item_size * n_items);
	a->n_items += n_items;
	return 1;
}

void pointless_dynarray_clear(pointless_dynarray_t* a)
{
	pointless_dynarray_destroy(a);
	pointless_dynarray_init(a, a->item_size);
}

void pointless_dynarray_destroy(pointless_dynarray_t* a)
{
	pointless_free(a->_data);
	a->_data = 0;
	a->n_items = 0;
	a->n_alloc = 0;
}

void* pointless_dynarray_item_at(pointless_dynarray_t* a, size_t i)
{
	assert(i < a->n_items);
	return (void*)((char*)a->_data + a->item_size * i);
}

void* pointless_dynarray_buffer(pointless_dynarray_t* a)
{
	return a->_data;
}

#define pointless_dynarray_vector_swap(data, i, j, T) { T* D = (T*)data; T t = D[i]; D[i] = D[j]; D[j] = t; }

void pointless_dynarray_swap(pointless_dynarray_t* a, size_t i, size_t j)
{
	assert(i < a->n_items);
	assert(j < a->n_items);

	switch (a->item_size) {
		case 1:
			pointless_dynarray_vector_swap(a->_data, i, j, uint8_t);
			return;
		case 2:
			pointless_dynarray_vector_swap(a->_data, i, j, uint16_t);
			return;
		case 4:
			pointless_dynarray_vector_swap(a->_data, i, j, uint32_t);
			return;
		case 8:
			pointless_dynarray_vector_swap(a->_data, i, j, uint64_t);
			return;
	}

	uint32_t k;

	i *= a->item_size;
	j *= a->item_size;

	uint8_t* D = (uint8_t*)a->_data;

	for (k = 0; k < a->item_size; k++, i++, j++) {
		uint8_t t = D[i];
		D[i] = D[j];
		D[j] = t;
	}
}

void pointless_dynarray_give_data(pointless_dynarray_t* a, void* data, size_t n_items)
{
	assert(a->n_items == 0);

	pointless_free(a->_data);

	a->_data = data;
	a->n_items = n_items;
	a->n_alloc = n_items;
}
