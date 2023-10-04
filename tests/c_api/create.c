#include "test.h"

void create_wrapper(const char* fname, create_cb cb)
{
	pointless_create_t c;
	const char* error = 0;

	clock_t t_0 = clock();
	pointless_create_begin_64(&c);

	(*cb)(&c);

	if (!pointless_create_output_and_end_f(&c, fname, &error)) {
		fprintf(stderr, "pointless_create_output() failure: %s\n", error);
		exit(EXIT_FAILURE);
	}

	clock_t t_1 = clock();

	printf("create time: %.2lfs\n", (double)(t_1 - t_0) / (double)CLOCKS_PER_SEC);
}

void query_wrapper(const char* fname, query_cb cb)
{
	pointless_t p;
	const char* error;

	if (!pointless_open_f(&p, fname, &error)) {
		fprintf(stderr, "pointless_open_f() failure: %s\n", error);
		exit(EXIT_FAILURE);
	}

	clock_t t_0 = clock();
	(*cb)(&p);
	clock_t t_1 = clock();

	printf("query time: %.2lfs\n", (double)(t_1 - t_0) / (double)CLOCKS_PER_SEC);

	pointless_close(&p);
}

void create_tuple(pointless_create_t* c)
{
	// create vector
	uint32_t vector_handle = pointless_create_vector_value(c);
	CHECK_HANDLE(vector_handle);

	pointless_create_set_root(c, vector_handle);
}

void create_very_simple(pointless_create_t* c)
{
	// uint32_2
	uint32_t u32_handle = pointless_create_u32(c, 0);
	CHECK_HANDLE(u32_handle);
	pointless_create_set_root(c, u32_handle);
}

void create_simple(pointless_create_t* c)
{
	// error string
	const char* error = 0;

	// vector
	uint32_t vector_handle = pointless_create_vector_value(c);
	CHECK_HANDLE(vector_handle);

	// bitvector
	uint32_t value = 123;
	uint32_t bitvector_handle = pointless_create_bitvector(c, &value, 27);
	CHECK_HANDLE(bitvector_handle);

	// string
	uint32_t string_handle = pointless_create_string_ascii(c, (uint8_t*)"Arni Mar Jonsson");

	if (string_handle == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_string_ascii() failure\n");
		exit(EXIT_FAILURE);
	}

	uint32_t unicode_handle = pointless_create_unicode_ascii(c, "Arni Mar Jonsson", &error);

	if (string_handle == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_unicode_ascii() failure: %s\n", error);
		exit(EXIT_FAILURE);
	}

	// add to vector
	uint32_t i, handles[] = { bitvector_handle, vector_handle, string_handle, unicode_handle };

	for (i = 0; i < 4; i++) {
		if (pointless_create_vector_value_append(c, vector_handle, handles[i]) == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "pointless_create_vector_append() failure\n");
			exit(EXIT_FAILURE);
		}
	}

	pointless_create_set_root(c, vector_handle);
}

#define N_INTEGERS 10
#define SET_DUPLICATES 0

void create_set(pointless_create_t* c)
{
	// some integers
	uint32_t i, integers[N_INTEGERS];

	for (i = 0; i < N_INTEGERS; i++) {
		// duplicate keys, must result in an error
		if (SET_DUPLICATES) {
			integers[i] = i < (N_INTEGERS / 2) ? pointless_create_u32(c, i) : pointless_create_float(c, (float)i * 0.5f);
		// non-duplicate keys
		} else {
			integers[i] = pointless_create_u32(c, i);
		}

		if (integers[i] == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "pointless_create_xxx(): out of memory\n");
			exit(EXIT_FAILURE);
		}
	}

	// create a set
	uint32_t set_handle = pointless_create_set(c);

	if (set_handle == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_set(): out of memory\n");
		exit(EXIT_FAILURE);
	}

	// add to set
	for (i = 0; i < N_INTEGERS; i++) {
		if (!pointless_create_set_add(c, set_handle, integers[i])) {
			fprintf(stderr, "pointless_create_set_add(): out of memory\n");
			exit(EXIT_FAILURE);
		}
	}

	pointless_create_set_root(c, set_handle);
}

void query_set(pointless_t* p)
{
	pointless_value_t* set = pointless_root(p);
	const char* error = 0;

	if (set->type != POINTLESS_SET_VALUE) {
		fprintf(stderr, "root is not a set\n");
		exit(EXIT_FAILURE);
	}

	uint32_t i;
	int32_t c;

	for (i = 0; i < N_INTEGERS; i++) {
		pointless_value_t k;

		if (SET_DUPLICATES) {
			k = i < (N_INTEGERS / 2) ? pointless_value_create_as_read_u32(i) : pointless_value_create_as_read_float((float)i * 0.5f);
		} else {
			k = pointless_value_create_as_read_u32(i);
		}

		pointless_value_t* kk = 0;
		pointless_reader_set_lookup(p, set, &k, &kk, &error);

		if (error) {
			fprintf(stderr, "pointless_reader_set_contains(): %s\n", error);
			exit(EXIT_FAILURE);
		}

		if (kk == 0) {
			fprintf(stderr, "set does not contain the expected value\n");
			exit(EXIT_FAILURE);
		}

		// they must be equal
		pointless_complete_value_t _k = pointless_value_to_complete(&k);
		pointless_complete_value_t _kk = pointless_value_to_complete(kk);
		c = pointless_cmp_reader(0, &_k, p, &_kk, &error);

		if (error) {
			fprintf(stderr, "pointless_cmp_reader(): %s\n", error);
			exit(EXIT_FAILURE);
		}

		if (c != 0) {
			fprintf(stderr, "containment query did not return an equivalent item\n");
			exit(EXIT_FAILURE);
		}
	}
}

void create_special_a(pointless_create_t* c)
{
	// following gave an error in Python wrapper
	// d = {(0, 1, 2, 3): 0}
	uint32_t d = pointless_create_vector_value(c);

	if (d == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_vector_value() failure\n");
		exit(EXIT_FAILURE);
	}

	uint32_t numbers[4], i;

	for (i = 0; i < 4; i++) {
		numbers[i] = pointless_create_u32(c, i);

		if (numbers[i] == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "pointless_create_u32() failure\n");
			exit(EXIT_FAILURE);
		}

		if (pointless_create_vector_value_append(c, d, numbers[i]) == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "pointless_create_vector_value_append() failure\n");
			exit(EXIT_FAILURE);
		}
	}

	uint32_t m = pointless_create_map(c);

	if (m == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_map() failure\n");
		exit(EXIT_FAILURE);
	}

	uint32_t zero = pointless_create_u32(c, 0);

	if (zero == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_u32() failure\n");
		exit(EXIT_FAILURE);
	}

	if (pointless_create_map_add(c, m, d, zero) == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_map_add() failure\n");
		exit(EXIT_FAILURE);
	}

	pointless_create_set_root(c, m);
}

void create_special_b(pointless_create_t* c)
{
	// the following gave an infinite recursion data structure
	// vectors = [['a', ['a']]]
	uint32_t vector_out = pointless_create_vector_value(c);
	uint32_t vector_in = pointless_create_vector_value(c);

	if (vector_out == POINTLESS_CREATE_VALUE_FAIL || vector_in == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_vector_value() failure\n");
		exit(EXIT_FAILURE);
	}

	uint32_t a_string[] = {'a', 0};

	uint32_t a = pointless_create_unicode_ucs4(c, a_string);
	uint32_t aa = pointless_create_unicode_ucs4(c, a_string);

	if (a == POINTLESS_CREATE_VALUE_FAIL || aa == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_unicode_ucs4() failure\n");
		exit(EXIT_FAILURE);
	}

	if (pointless_create_vector_value_append(c, vector_in, aa) == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_vector_value_append() failure\n");
		exit(EXIT_FAILURE);
	}

	if (pointless_create_vector_value_append(c, vector_out, a) == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_vector_value_append() failure\n");
		exit(EXIT_FAILURE);
	}

	if (pointless_create_vector_value_append(c, vector_out, vector_in) == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_vector_value_append() failure\n");
		exit(EXIT_FAILURE);
	}

	pointless_create_set_root(c, vector_out);
}


void create_special_c(pointless_create_t* c)
{
	// the following gave a valgrind error, there are 32 vectors
	// [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
	//                                 ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
	uint32_t vectors[32], i;

	for (i = 0; i < 32; i++) {
		vectors[i] = pointless_create_vector_value(c);

		if (vectors[i] == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "pointless_create_vector_value() failure\n");
			exit(EXIT_FAILURE);
		}
	}

	// reverse order
	for (i = 31; i >= 1; i--) {
		if (pointless_create_vector_value_append(c, vectors[i - 1], vectors[i]) == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "pointless_create_vector_value_append() failure\n");
			exit(EXIT_FAILURE);
		}
	}

	pointless_create_set_root(c, vectors[0]);
}

#define SPECIAL_D_N 16
#define SPECIAL_D_I 2

static uint32_t create_special_d_0(pointless_create_t* c)
{
	uint32_t v = pointless_create_vector_u32(c), i;

	if (v == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "create_special_d_0(): pointless_create_vector_u32() failure\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < SPECIAL_D_N; i++) {
		if (pointless_create_vector_u32_append(c, v, i) == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "create_special_d_0(): pointless_create_vector_u32_append() failure\n");
			exit(EXIT_FAILURE);
		}
	}

	return v;
}

static uint32_t create_special_d_1(pointless_create_t* c, uint32_t* buffer)
{
	//  1) caller owned uint32_t vector
	uint32_t v, i;

	for (i = 0; i < SPECIAL_D_N; i++)
		buffer[i] = i;

	v = pointless_create_vector_u32_owner(c, buffer, SPECIAL_D_N);

	if (v == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "create_special_d_1(): pointless_create_vector_u32_owner() failure\n");
		exit(EXIT_FAILURE);
	}

	return v;
}

static uint32_t create_special_d_2(pointless_create_t* c)
{
	uint32_t* u32_buffer = (uint32_t*)pointless_malloc(sizeof(uint32_t) * SPECIAL_D_N);

	if (u32_buffer == 0) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	uint32_t v, i;

	for (i = 0; i < SPECIAL_D_N; i++)
		u32_buffer[i] = i;

	v = pointless_create_vector_u32(c);

	if (v == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "create_special_d_2(): pointless_create_vector_u32() failure\n");
		exit(EXIT_FAILURE);
	}

	if (pointless_create_vector_u32_transfer(c, v, u32_buffer, SPECIAL_D_N) == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "create_special_d_2(): pointless_create_vector_u32_transfer() failure\n");
		exit(EXIT_FAILURE);
	}

	return v;
}

static uint32_t create_special_d_3(pointless_create_t* c)
{
	uint32_t* value_buffer = (uint32_t*)pointless_malloc(sizeof(uint32_t) * SPECIAL_D_N);

	if (value_buffer == 0) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}

	uint32_t v, i;

	for (i = 0; i < SPECIAL_D_N; i++) {
		value_buffer[i] = pointless_create_u32(c, i);

		if (value_buffer[i] == POINTLESS_CREATE_VALUE_FAIL) {
			fprintf(stderr, "pointless_create_u32() failure\n");
			exit(EXIT_FAILURE);
		}
	}

	v = pointless_create_vector_value(c);

	if (v == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_vector_value() failure\n");
		exit(EXIT_FAILURE);
	}

	if (pointless_create_vector_value_transfer(c, v, value_buffer, SPECIAL_D_N) == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_vector_value_transfer() failure\n");
		exit(EXIT_FAILURE);
	}

	return v;
}

uint32_t buffer[SPECIAL_D_I][SPECIAL_D_N];

void create_special_d(pointless_create_t* c)
{
	// value vector of:
	//  0) regular uint32_t vector
	//  1) caller owned uint32_t vector
	//  2) transferred uint32_t vector
	//  3) transferred value vector

	uint32_t root = pointless_create_vector_value(c), i, j;

	if (root == POINTLESS_CREATE_VALUE_FAIL) {
		fprintf(stderr, "pointless_create_vector_value() failure\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < SPECIAL_D_I; i++) {
		for (j = 0; j < SPECIAL_D_N; j++)
			buffer[i][j] = j;
	}

	for (i = 0; i < SPECIAL_D_I; i++) {
		uint32_t vector[4];

		uint32_t* priv_buffer = buffer[i];

		vector[0] = create_special_d_0(c);
		vector[1] = create_special_d_1(c, priv_buffer);
		vector[2] = create_special_d_2(c);
		vector[3] = create_special_d_3(c);

		// add children to root
		for (j = 0; j < 4; j++) {
			if (pointless_create_vector_value_append(c, root, vector[j]) == POINTLESS_CREATE_VALUE_FAIL) {
				fprintf(stderr, "pointless_create_vector_value_append() failure\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	pointless_create_set_root(c, root);
}

void query_special_d(pointless_t* p)
{
	pointless_value_t* root = pointless_root(p);

	if (root->type != POINTLESS_VECTOR_VALUE_HASHABLE) {
		fprintf(stderr, "query_special_d(): root is not a hashable vector\n");
		exit(EXIT_FAILURE);
	}

	if (pointless_reader_vector_n_items(p, root) != 4 * SPECIAL_D_I) {
		fprintf(stderr, "query_special_d(): root has not %u items, but %u\n", 4 * SPECIAL_D_I, pointless_reader_vector_n_items(p, root));
		exit(EXIT_FAILURE);
	}

	uint32_t i, j;

	for (i = 0; i < 4 * SPECIAL_D_I; i += 4) {
		pointless_complete_value_t _v = pointless_reader_vector_value_case(p, root, i);
		pointless_value_t v = pointless_value_from_complete(&_v);

		if (!pointless_is_vector_type(v.type)) {
			fprintf(stderr, "query_special_d(): root[%u] is not a vector\n", i);
			exit(EXIT_FAILURE);
		}

		if (pointless_reader_vector_n_items(p, &v) != SPECIAL_D_N) {
			fprintf(stderr, "query_special_d(): | root[%u] | is not %u, but %u\n", i, SPECIAL_D_N, pointless_reader_vector_n_items(p, &v));
			exit(EXIT_FAILURE);
		}

		for (j = 0; j < SPECIAL_D_N; j++) {
			pointless_complete_value_t _vv = pointless_reader_vector_value_case(p, &v, j);
			pointless_value_t vv = pointless_value_from_complete(&_vv);

			if (vv.type != POINTLESS_U32 && vv.type != POINTLESS_I32) {
				fprintf(stderr, "query_special_d(): root[%u][%u] is not i32/u32 (%u)\n", i, j, vv.type);
				exit(EXIT_FAILURE);
			}

			if (vv.type == POINTLESS_I32 && pointless_value_get_i32(vv.type, &vv.data) != (int32_t)j) {
				fprintf(stderr, "query_special_d(): root[%u][%u] != %u, but %i\n", i, j, j, pointless_value_get_i32(vv.type, &vv.data));
				exit(EXIT_FAILURE);
			}

			if (vv.type == POINTLESS_U32 && pointless_value_get_u32(vv.type, &vv.data) != j) {
				fprintf(stderr, "query_special_d(): root[%u][%u] != %u, but %u\n", i, j, j, pointless_value_get_u32(vv.type, &vv.data));
				exit(EXIT_FAILURE);
			}
		}
	}
}
