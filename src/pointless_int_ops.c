#include <pointless/pointless_int_ops.h>

typedef unsigned int uint128_t __attribute__((mode(TI)));

#define __OVERFLOW_MULT(a, b, out, T, M) {uint128_t v = (uint128_t)(a) * (uint128_t)(b); *(out) = (T)v; return (v <= M); }
#define __OVERFLOW_ADD_2(a, b, out, T, M) {uint128_t v = (uint128_t)(a) + (uint128_t)(b); *(out) = (T)v; return (v <= M); }
#define __OVERFLOW_ADD_3(a, b, c, out, T, M) {uint128_t v = (uint128_t)(a) + (uint128_t)(b) + (uint128_t)(c); *(out) = (T)v; return (v <= M); }

int sizet_mult(size_t a, size_t b, size_t* out)
{
	__OVERFLOW_MULT(a, b, out, size_t, SIZE_MAX);
}

int u64_mult(uint64_t a, uint64_t b, uint64_t* out)
{
	__OVERFLOW_MULT(a, b, out, uint64_t, UINT64_MAX);
}

int u32_mult(uint32_t a, uint32_t b, uint32_t* out)
{
	__OVERFLOW_MULT(a, b, out, uint32_t, UINT32_MAX);
}

int sizet_add_3(size_t a, size_t b, size_t c, size_t* out)
{
	__OVERFLOW_ADD_3(a, b, c, out, size_t, SIZE_MAX);
}

#undef __OVERFLOW_MULT
#undef __OVERFLOW_ADD_2
#undef __OVERFLOW_ADD_3

#undef uint128_t
