#ifndef __POINTLESS__INT__OPS__H__
#define __POINTLESS__INT__OPS__H__

#include <stdlib.h>
#include <stdint.h>

// values
typedef struct {
	int is_overflow;
	size_t value;
} intop_sizet;

typedef struct {
	int is_overflow;
	uint32_t value;
} intop_u32;

typedef struct {
	int is_overflow;
	uint64_t value;
} intop_u64;

// multiply
intop_sizet sizet_mult(intop_sizet a, intop_sizet b);
intop_u64 u64_mult(intop_u64 a, intop_u64 b);
intop_u32 u32_mult(intop_u32, intop_u32 b);

// add
intop_sizet sizet_add(intop_sizet a, intop_sizet b);

// value constructors
intop_sizet intop_sizet_(size_t v);
intop_u32 intop_u32_(uint32_t v);
intop_u64 intop_u64_(uint64_t v);

#endif

