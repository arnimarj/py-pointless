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
	int i = intop_eval_compile(argv[1], &context, &error);


	if (i) {
		printf("RESULT: success\n");
	} else {
		printf("FAILURE: %s\n", error);
	}

	exit(EXIT_SUCCESS);
}
