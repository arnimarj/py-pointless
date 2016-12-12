#include "test.h"

void validate_hash_semantics()
{
	uint64_t i = 0;
	int32_t v = INT32_MIN;

	uint32_t h_i32, h_u32, h_f;

	union {
		int32_t i32;
		uint32_t u32;
		float f;
	} hash;

	while (i <= 0xFFFFFFFF) {
		i += 1;

		hash.i32 = v;
		h_i32 = pointless_hash_i32_32(hash.i32);

		hash.i32 = v;
		h_u32 = pointless_hash_u32_32(hash.u32);

		hash.f = (float)v;
		h_f = pointless_hash_float_32(hash.f);

		if (h_i32 != h_u32) {
			fprintf(stderr, "v: %i i32/u32 hash mismatch: %u/%u\n", v, h_i32, h_u32);
			exit(EXIT_FAILURE);
		}

		if (h_u32 != h_f) {
			fprintf(stderr, "v: %i u32/f hash mismatch: %u/%u\n", v, h_u32, h_f);
			exit(EXIT_FAILURE);
		}

		if (h_i32 != h_f) {
			fprintf(stderr, "v: %i i32/f hash mismatch: %u/%u\n", v, h_i32, h_f);
			exit(EXIT_FAILURE);
		}

		if (i % (1024*1024*16) == 0) {
			printf("0x%08x\n", (uint32_t)i);
			fflush(stdout);
		}
	}
}
