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

void pointless_dynarray_pop(pointless_dynarray_t* a)
{
	assert(a->n_items > 0);
	a->n_items -= 1;
}

// debug stuff
#include <stdio.h>

static size_t next_size(size_t n_alloc)
{
	size_t small_add[] = {1, 1, 2, 2, 4, 4, 4, 8, 8, 10, 11, 12, 13, 14, 15, 16};
	size_t a = n_alloc / 16;
	size_t b = n_alloc;
	size_t c = (n_alloc < 16) ? small_add[n_alloc] : 0;

	return (a + b + c);
}

int pointless_dynarray_push(pointless_dynarray_t* a, void* i)
{
	if (a->n_items == a->n_alloc) {
		size_t next_n_alloc = next_size(a->n_alloc);
		void* next_data = realloc(a->_data, a->item_size * next_n_alloc);

		if (next_data == 0) {
			fprintf(stderr, "ERROR: failure to grow vector of %zu items to %zu items, each item being %zu bytes\n", a->n_alloc, next_n_alloc, a->item_size);
			return 0;
		}

		a->_data = next_data;
		a->n_alloc = next_n_alloc;
	}

	memcpy((char*)a->_data + a->item_size * a->n_items, i, a->item_size);
	a->n_items += 1;

	return 1;
}

void pointless_dynarray_clear(pointless_dynarray_t* a)
{
	pointless_dynarray_destroy(a);
	pointless_dynarray_init(a, a->item_size);
}

void pointless_dynarray_destroy(pointless_dynarray_t* a)
{
	free(a->_data);
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

	for (k = 0; k < a->item_size; k++) {
		uint8_t t = D[i];
		D[i] = D[j];
		D[j] = t;
	}
}

void pointless_dynarray_give_data(pointless_dynarray_t* a, void* data, size_t n_items)
{
	assert(a->n_items == 0);

	free(a->_data);

	a->_data = data;
	a->n_items = n_items;
	a->n_alloc = n_items;
}
