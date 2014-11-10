#ifndef __POINTLESS__INT__OPS__H__
#define __POINTLESS__INT__OPS__H__

#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

// values
typedef struct {
	int is_overflow;
	size_t value;
} intop_sizet_t;

typedef struct {
	int is_overflow;
	uint32_t value;
} intop_u32_t;

typedef struct {
	int is_overflow;
	uint64_t value;
} intop_u64_t;

// multiply
intop_sizet_t intop_sizet_mult(intop_sizet_t a, intop_sizet_t b);
intop_u64_t intop_u64_mult(intop_u64_t a, intop_u64_t b);
intop_u32_t intop_u32_mult(intop_u32_t, intop_u32_t b);

// add
intop_sizet_t intop_sizet_add(intop_sizet_t a, intop_sizet_t b);
intop_u32_t intop_u32_add(intop_u32_t a, intop_u32_t b);
intop_u64_t intop_u64_add(intop_u64_t a, intop_u64_t b);

// value constructors
intop_sizet_t intop_sizet_init(size_t v);
intop_u32_t intop_u32_init(uint32_t v);
intop_u64_t intop_u64_init(uint64_t v);

// expression evalator, roughly the grammar:
// E = T+T, T = E*E, F = (E)|number|x
// x is a standin for a positional variable

#define INTOP_EVAL_NUMBER   0
#define INTOP_EVAL_ADD      1
#define INTOP_EVAL_SUB      2
#define INTOP_EVAL_MUL      3
#define INTOP_EVAL_DIV      4
#define INTOP_EVAL_LPAREN   5
#define INTOP_EVAL_RPAREN   6
#define INTOP_EVAL_VARIABLE 7

typedef struct {
	int type;
	intop_u64_t number;
	int var_index;
} intop_eval_token_t;

#define INTOP_EVAL_MAX_N 512

typedef struct {
	// token list, and current token
	int i;
	int n;
	intop_eval_token_t tokens[INTOP_EVAL_MAX_N];

	// compile stack
	intop_eval_token_t stack[INTOP_EVAL_MAX_N];
	int s_n;

	// eval stack
	intop_eval_token_t eval[INTOP_EVAL_MAX_N];
	int e_n;

	// error
	const char* s_error;
	int i_error;
} intop_eval_context_t;

int intop_eval_compile(const char* s, intop_eval_context_t* context, const char** error);
int intop_eval_eval(intop_eval_context_t* context, uint64_t* r, const char** error, ...);

#endif
