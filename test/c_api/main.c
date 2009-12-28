#include "test.h"

static void print_map(const char* fname)
{
	pointless_t p;
	const char* error = 0;

	if (!pointless_open_f(&p, fname, &error)) {
		fprintf(stderr, "pointless_open_f() failure: %s\n", error);
		exit(EXIT_FAILURE);
	}

	if (!pointless_debug_print(&p, stdout, &error)) {
		fprintf(stderr, "pointless_debug_print() failure: %s\n", error);
		exit(EXIT_FAILURE);
	}

	pointless_close(&p);
}

static void measure_load_time(const char* fname)
{
	pointless_t p;
	const char* error = 0;


	clock_t t_0 = clock();

	if (!pointless_open_f(&p, fname, &error)) {
		fprintf(stderr, "pointless_open_f() failure: %s\n", error);
		exit(EXIT_FAILURE);
	}

	clock_t t_1 = clock();

	printf("INFO: load time: %.3f\n", (double)(t_1 - t_0) / (double)CLOCKS_PER_SEC);

	pointless_close(&p);
}

static void run_re_create(const char* fname_in, const char* fname_out)
{
	const char* error = 0;

	if (!pointless_recreate(fname_in, fname_out, &error)) {
		fprintf(stderr, "pointless_recreate() failure: %s\n", error);
		exit(EXIT_FAILURE);
	}
}

static void print_usage_exit()
{
	fprintf(stderr, "usage: ./pointless_util OPTIONS\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "   --unit-test | --measure-load-time pointless.map | --test-hash | --dump-file pointless.map | --re-create fname_in fname_out)\n");
	exit(EXIT_FAILURE);
}

static void run_unit_test()
{
	create_wrapper("very_simple.map", create_very_simple);
	print_map("very_simple.map");

	create_wrapper("simple.map", create_simple);
	print_map("simple.map");

	create_wrapper("tuple.map", create_tuple);
	print_map("tuple.map");

	create_wrapper("set.map", create_set);
	query_wrapper("set.map", query_set);
	print_map("set.map");

	create_wrapper("special_a.map", create_special_a);
	print_map("special_a.map");

	create_wrapper("special_b.map", create_special_b);
	print_map("special_b.map");

	create_wrapper("special_c.map", create_special_c);
	print_map("special_c.map");

	create_wrapper("special_d.map", create_special_d);
	print_map("special_d.map");
	query_wrapper("special_d.map", query_special_d);
	print_map("special_d.map");
}

int main(int argc, char** argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "--unit-test") == 0)
			run_unit_test();
		else if (strcmp(argv[1], "--test-hash") == 0)
			validate_hash_semantics();
		else
			print_usage_exit();
	} else if (argc == 3) {
		if (strcmp(argv[1], "--dump-file") == 0)
			print_map(argv[2]);
		else if (strcmp(argv[1], "--measure-load-time") == 0)
			measure_load_time(argv[2]);
		else
			print_usage_exit();
	} else if (argc == 4) {
		if (strcmp(argv[1], "--re-create") == 0)
			run_re_create(argv[2], argv[3]);
		else
			print_usage_exit();
	} else {
		print_usage_exit();
	}

	exit(EXIT_SUCCESS);
}
