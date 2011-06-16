#ifndef __POINTLESS__INT__OPS__H__
#define __POINTLESS__INT__OPS__H__

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

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
intop_u32 u32_add(intop_u32 a, intop_u32 b);
intop_u64 u64_add(intop_u64 a, intop_u64 b);

// value constructors
intop_sizet intop_sizet_(size_t v);
intop_u32 intop_u32_(uint32_t v);
intop_u64 intop_u64_(uint64_t v);

// expression evalator, roughly the grammar:
// E = T+T, T = E*E, F = (E)|number|x
// x is a standin for a positional variable

#define intop_eval_NUMBER   0
#define intop_eval_PLUS     1
#define intop_eval_MULT     2
#define intop_eval_LPAREN   3
#define intop_eval_RPAREN   4
#define intop_eval_VARIABLE 5

typedef struct {
	int type;
	intop_u64 number;
	int var_index;
} intop_eval_token_t;

#define intop_eval_MAX_N 512

typedef struct {
	// token list, and current token
	int i;
	int n;
	intop_eval_token_t tokens[intop_eval_MAX_N];

	// result stack
	intop_eval_token_t stack[intop_eval_MAX_N];
	int s_n;

	// error
	const char* s_error;
	int i_error;
} intop_eval_context_t;

int intop_eval_compile(const char* s, intop_eval_context_t* context, const char** error);
int intop_eval_eval(intop_eval_context_t* context, const char** error, ...);

#endif
