#include "test.h"

#define ONE_MILLION 2000000

void create_1M_set(pointless_create_t* c)
{
	uint32_t i, j, t, s;
	uint32_t* data = (uint32_t*)pointless_malloc(sizeof(uint32_t) * ONE_MILLION);

	if (data == 0) {
		fprintf(stderr, "create_1M_set(): out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < ONE_MILLION; i++)
		data[i] = i;

	for (i = 0; i < ONE_MILLION; i++) {
		j = rand() % ONE_MILLION;
		t = data[j];
		data[j] = data[i];
		data[i] = t;
	}

	s = pointless_create_set(c);

	if (s == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "create_1M_set(): out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < ONE_MILLION; i++) {
		t = pointless_create_u32(c, data[i]);

		if (t == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "create_1M_set(): out of memory\n");
			exit(EXIT_FAILURE);
		}

		if (!pointless_create_set_add(c, s, t)) {
			fprintf(stderr, "create_1M_set(): out of memory\n");
			exit(EXIT_FAILURE);
		}
	}

	pointless_free(data);

	pointless_create_set_root(c, s);
}

void query_1M_set(pointless_t* p)
{
	pointless_value_t* set = pointless_root(p);
	const char* error = 0;

	if (set->type != POINTLESS_SET_VALUE) {
		fprintf(stderr, "query_1M_set(): root is not a set\n");
		exit(EXIT_FAILURE);
	}

	pointless_value_t k;
	uint32_t i;

	for (i = 0; i < ONE_MILLION; i++) {
		k = pointless_value_create_as_read_u32(i);
		pointless_value_t* kk = 0;
		pointless_reader_set_lookup(p, set, &k, &kk, &error);

		if (error) {
			fprintf(stderr, "query_1M_set(): pointless_reader_set_contains() failure: %s\n", error);
			exit(EXIT_FAILURE);
		}

		if (kk == 0) {
			fprintf(stderr, "query_1M_set(): set does not contain the expected value\n");
			exit(EXIT_FAILURE);
		}
	}
}
