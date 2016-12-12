#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <pointless/pointless_int_ops.h>

int main(int argc, const char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: ./test_intop_eval expression\n");
		exit(EXIT_FAILURE);
	}

	intop_eval_context_t context;
	const char* error = 0;
	uint64_t r = 0;
	int i = intop_eval_compile(argv[1], &context, &error);


	if (i) {
		printf("COMPILE: success\n");
	} else {
		printf("COMPILE: %s\n", error);
	}

	i = intop_eval_eval(&context, &r, &error);

	if (i) {
		printf("EVAL: success [%llu]\n", (unsigned long long)r);
	} else {
		printf("EVAL: %s\n", error);
	}

	exit(EXIT_SUCCESS);
}
