#include <pointless/pointless_int_ops.h>

// the multiply overflow check is from "Secure Coding in C and C++", chapter 5

intop_sizet sizet_mult(intop_sizet a, intop_sizet b)
{
	intop_sizet c;

	if (a.is_overflow || b.is_overflow || (a.value != 0 && b.value > SIZE_MAX / a.value) || (b.value != 0 && a.value > SIZE_MAX / b.value)) {
		c.is_overflow = 1;
		c.value = 0;
	} else {
		c.is_overflow = 0;
		c.value = a.value * b.value;
	}

	return c;
}

intop_u64 u64_mult(intop_u64 a, intop_u64 b)
{
	intop_u64 c;

	if (a.is_overflow || b.is_overflow || (a.value != 0 && b.value > UINT64_MAX / a.value) || (b.value != 0 && a.value > UINT64_MAX / b.value)) {
		c.is_overflow = 1;
		c.value = 0;
	} else {
		c.is_overflow = 0;
		c.value = a.value * b.value;
	}

	return c;
}

intop_u32 u32_mult(intop_u32 a, intop_u32 b)
{
	intop_u32 c;

	if (a.is_overflow || b.is_overflow || (a.value != 0 && b.value > UINT32_MAX / a.value) || (b.value != 0 && a.value > UINT32_MAX / b.value)) {
		c.is_overflow = 1;
		c.value = 0;
	} else {
		c.is_overflow = 0;
		c.value = a.value * b.value;
	}

	return c;
}

intop_sizet sizet_add(intop_sizet a, intop_sizet b)
{
	intop_sizet c;

	if (a.is_overflow || b.is_overflow) {
		c.is_overflow = 1;
		c.value = 0;
	} else {
		// sum = lhs + rhs
		// overflow iff: sum < lhs or sum < rhs
		c.value = a.value + b.value;
		c.is_overflow = (c.value < a.value && c.value < b.value);
	}

	return c;

}

intop_u32 u32_add(intop_u32 a, intop_u32 b)
{
	intop_u32 c;

	if (a.is_overflow || b.is_overflow) {
		c.is_overflow = 1;
		c.value = 0;
	} else {
		// sum = lhs + rhs
		// overflow iff: sum < lhs or sum < rhs
		c.value = a.value + b.value;
		c.is_overflow = (c.value < a.value && c.value < b.value);
	}

	return c;

}

intop_u64 u64_add(intop_u64 a, intop_u64 b)
{
	intop_u64 c;

	if (a.is_overflow || b.is_overflow) {
		c.is_overflow = 1;
		c.value = 0;
	} else {
		// sum = lhs + rhs
		// overflow iff: sum < lhs or sum < rhs
		c.value = a.value + b.value;
		c.is_overflow = (c.value < a.value && c.value < b.value);
	}

	return c;

}

// I took the effort to write a Python program to validate these. All critique welcome.
/*
#!/usr/bin/python

import itertools

def add_overflow_good(a, b):
	assert(0 <= a < 2**16)
	assert(0 <= b < 2**16)
	c = a + b
	ok = (c < 2**16)
	c = c % (2**16)
	return (ok, c)

def add_overflow_c(a, b):
	assert(0 <= a < 2**16)
	assert(0 <= b < 2**16)
	c = (a + b) % (2**16)
	#ok = ((c - a) == b)
	ok = (c >=a and c >= b)
	return (ok, c)

def mult_overflow_good(a, b):
	assert(0 <= a < 2**16)
	assert(0 <= b < 2**16)
	c = a * b
	ok = (c < 2**16)
	c = c % 2**16
	return (ok, c)

def mult_overflow_c(a, b):
	if (a == 0):
		ok = True
	else:
		ok = (b <= (2**16-1) / a)
	c = (a * b) % 2**16
	return (ok, c)

#stride = 16
stride = 1

for c, (i, j) in enumerate(itertools.product(xrange(0, 2**16, stride), xrange(0, 2**16, stride))):
	if c % 1000000 == 0:
		print 'I', i

	a, _a = add_overflow_good(i, j)
	b, _b = add_overflow_c(i, j)

	assert(a == b)
	assert(_a == _b)

	a, _a = mult_overflow_good(i, j)
	b, _b = mult_overflow_c(i, j)

	assert(a == b)
	assert(_a == _b)

*/

intop_sizet intop_sizet_(size_t v)
{
	intop_sizet r = {0, v};
	return r;
}

intop_u32 intop_u32_(uint32_t v)
{
	intop_u32 r = {0, v};
	return r;
}

intop_u64 intop_u64_(uint64_t v)
{
	intop_u64 r = {0, v};
	return r;
}

// E = T+T
// T = E*E
// F = (E)|fall(E)|tala
//
// becomes...
//
// E  := TE'
// E' := +TE'|e
// T  := FT'
// T' := *FT'|e
// F  := (E)|tala|variable

static void intop_eval_E(intop_eval_context_t* context);
static void intop_eval_Em(intop_eval_context_t* context);
static void intop_eval_T(intop_eval_context_t* context);
static void intop_eval_Tm(intop_eval_context_t* context);
static void intop_eval_F(intop_eval_context_t* context);

static int intop_check_error(intop_eval_context_t* context)
{
	return (context->s_error != 0);
}

static int intop_eval_check_bounds(intop_eval_context_t* context)
{
	return (context->i < intop_eval_MAX_N);
}

static void intop_eval_push(intop_eval_context_t* context, intop_eval_token_t* token)
{
	if (context->s_n == intop_eval_MAX_N) {
		context->s_error = "result stack overflow";
		context->i_error = context->i;
		return;
	}

	context->stack[context->s_n++] = *token;
}

// E  := TE'
static void intop_eval_E(intop_eval_context_t* context)
{
	if (intop_check_error(context))
		return;

	intop_eval_T(context);
	intop_eval_Em(context);
}

// E' := +TE'|e
static void intop_eval_Em(intop_eval_context_t* context)
{
	if (intop_check_error(context))
		return;

	if (intop_eval_check_bounds(context) && context->tokens[context->i].type == intop_eval_PLUS) {
		int i = context->i;
		context->i += 1;
		intop_eval_T(context);
		intop_eval_Em(context);

		intop_eval_push(context, &context->tokens[i]);
	}
}

// T  := FT'
static void intop_eval_T(intop_eval_context_t* context)
{
	if (intop_check_error(context))
		return;

	intop_eval_F(context);
	intop_eval_Tm(context);
}

// T' := *FT'|e
static void intop_eval_Tm(intop_eval_context_t* context)
{
	if (intop_check_error(context))
		return;

	if (intop_eval_check_bounds(context) && context->tokens[context->i].type == intop_eval_MULT) {
		int i = context->i;
		context->i += 1;
		intop_eval_F(context);
		intop_eval_Tm(context);

		intop_eval_push(context, &context->tokens[i]);
	}
}

static void intop_eval_F(intop_eval_context_t* context)
{
	if (intop_check_error(context))
		return;

	if (!intop_eval_check_bounds(context))
		goto failure;

	if (context->tokens[context->i].type == intop_eval_NUMBER) {
		intop_eval_push(context, &context->tokens[context->i]);
		goto success;
	}

	if (context->tokens[context->i].type == intop_eval_VARIABLE) {
		intop_eval_push(context, &context->tokens[context->i]);
		goto success;
	}

	if (context->tokens[context->i].type != intop_eval_LPAREN)
		goto failure;

	context->i += 1;
	intop_eval_E(context);

	if (intop_check_error(context))
		return;

	if (!intop_eval_check_bounds(context) || context->tokens[context->i].type != intop_eval_RPAREN) {
		context->s_error = "expected ')'";
		context->i_error = context->i;
		return;
	}

success:
	context->i += 1;
	return;

failure:

	context->s_error = "expected '(', number or variable";
	context->i_error = context->i;

}

static int parse_number(const char* s, uint64_t* n)
{
	char* endptr = 0;
	errno = 0;
	unsigned long long int n_ = strtoull(s, &endptr, 10);

	if ((n_ == ULLONG_MAX && errno != 0) || *endptr != 0)
		return 0;

	*n = (uint64_t)n_;
	return 1;
}

int intop_eval_compile(const char* s, intop_eval_context_t* context, const char** error)
{
	// initialize context
	context->i = 0;
	context->n = 0;
	context->s_n = 0;
	context->e_n = 0;
	context->s_error = 0;
	context->i_error = 0;

	int n_variables = 0;

	// tokenize
	char token[32];
	int i_token = 0;

	while (*s) {
		if (context->n >= intop_eval_MAX_N) {
			*error = "too many tokens";
			return 0;
		}

		switch (*s) {
			case '(':
				context->tokens[context->n].type = intop_eval_LPAREN;
				context->n += 1;
				s += 1;
				break;
			case ')':
				context->tokens[context->n].type = intop_eval_RPAREN;
				context->n += 1;
				s += 1;
				break;
			case ' ':
			case '\t':
				s += 1;
				break;
			case '+':
				context->tokens[context->n].type = intop_eval_PLUS;
				context->n += 1;
				s += 1;
				break;
			case '*':
				context->tokens[context->n].type = intop_eval_MULT;
				context->n += 1;
				s += 1;
				break;
			case 'x':
				context->tokens[context->n].type = intop_eval_VARIABLE;
				context->tokens[context->n].var_index = n_variables;
				context->n += 1;
				s += 1;
				n_variables += 1;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				while (*s && isdigit(*s) && i_token < 31) {
					token[i_token] = *s;
					i_token += 1;
					s += 1;
				}

				if (i_token == 31) {
					*error = "too big of a number";
					return 0;
				}

				token[i_token] = 0;

				context->tokens[context->n].type = intop_eval_NUMBER;
				context->tokens[context->n].number.is_overflow = 0;
				context->tokens[context->n].number.value = atoi(token);

				if (!parse_number(token, &context->tokens[context->n].number.value)) {
					*error = "invalid number";
					return 0;
				}

				i_token = 0;
				context->n += 1;

				break;

			default:
				*error = "invalid token";
				return 0;
		}
	}

	if (context->n == 0) {
		*error = "no tokens";
		return 0;
	}

	intop_eval_E(context);

	if (intop_check_error(context)) {
		*error = context->s_error;
		return 0;
	}

	if (context->i != context->n) {
		*error = "some parsing error";
		return 0;
	}

	return 1;
}

int intop_eval_eval(intop_eval_context_t* context, uint64_t* r, const char** error, ...)
{
	// reset context
	context->e_n = 0;
	context->s_error = 0;
	context->i_error = 0;

	intop_u64 a, b;
	int i;

	for (i = 0; i < context->s_n; i++) {
		switch (context->stack[i].type) {
			case intop_eval_NUMBER:
				context->eval[context->e_n] = context->stack[i];
				context->e_n += 1;
				break;
			case intop_eval_PLUS:
				assert(context->e_n >= 2);
				assert(context->eval[context->e_n - 1].type == intop_eval_NUMBER);
				assert(context->eval[context->e_n - 2].type == intop_eval_NUMBER);
				a = context->eval[context->e_n - 1].number;
				b = context->eval[context->e_n - 2].number;
				context->eval[context->e_n - 2].type = intop_eval_NUMBER;
				context->eval[context->e_n - 2].number = u64_add(a, b);
				context->e_n -= 1;
				break;
			case intop_eval_MULT:
				assert(context->e_n >= 2);
				assert(context->eval[context->e_n - 1].type == intop_eval_NUMBER);
				assert(context->eval[context->e_n - 2].type == intop_eval_NUMBER);
				a = context->eval[context->e_n - 1].number;
				b = context->eval[context->e_n - 2].number;
				context->eval[context->e_n - 2].type = intop_eval_NUMBER;
				context->eval[context->e_n - 2].number = u64_mult(a, b);
				context->e_n -= 1;
				break;

			default:
				*error = "not supported yet";
				return 0;
		}
	}

	if (context->e_n != 1 || context->eval[0].type != intop_eval_NUMBER) {
		*error = "compile/eval error";
		return 0;
	}

	if (context->eval[0].number.is_overflow) {
		*error = "eval overflow";
		return 0;
	}

	*r = context->eval[0].number.value;
	return 1;
}
