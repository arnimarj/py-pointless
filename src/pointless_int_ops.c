#include <pointless/pointless_int_ops.h>

// the multiply overflow check is from "Secure Coding in C and C++", chapter 5

intop_sizet_t intop_sizet_mult(intop_sizet_t a, intop_sizet_t b)
{
	intop_sizet_t c;

	if (a.is_overflow || b.is_overflow || (a.value != 0 && b.value > SIZE_MAX / a.value) || (b.value != 0 && a.value > SIZE_MAX / b.value)) {
		c.is_overflow = 1;
		c.value = 1;
	} else {
		c.is_overflow = 0;
		c.value = a.value * b.value;
	}

	return c;
}

intop_u64_t intop_u64_mult(intop_u64_t a, intop_u64_t b)
{
	intop_u64_t c;

	if (a.is_overflow || b.is_overflow || (a.value != 0 && b.value > UINT64_MAX / a.value) || (b.value != 0 && a.value > UINT64_MAX / b.value)) {
		c.is_overflow = 1;
		c.value = 1;
	} else {
		c.is_overflow = 0;
		c.value = a.value * b.value;
	}

	return c;
}

intop_u32_t uintop_32_mult(intop_u32_t a, intop_u32_t b)
{
	intop_u32_t c;

	if (a.is_overflow || b.is_overflow || (a.value != 0 && b.value > UINT32_MAX / a.value) || (b.value != 0 && a.value > UINT32_MAX / b.value)) {
		c.is_overflow = 1;
		c.value = 1;
	} else {
		c.is_overflow = 0;
		c.value = a.value * b.value;
	}

	return c;
}

intop_sizet_t intop_sizet_add(intop_sizet_t a, intop_sizet_t b)
{
	intop_sizet_t c;

	if (a.is_overflow || b.is_overflow) {
		c.is_overflow = 1;
		c.value = 1;
	} else {
		// sum = lhs + rhs
		// overflow iff: sum < lhs or sum < rhs
		c.value = a.value + b.value;
		c.is_overflow = (c.value < a.value && c.value < b.value);
	}

	return c;
}

intop_u32_t intop_u32_add(intop_u32_t a, intop_u32_t b)
{
	intop_u32_t c;

	if (a.is_overflow || b.is_overflow) {
		c.is_overflow = 1;
		c.value = 1;
	} else {
		// sum = lhs + rhs
		// overflow iff: sum < lhs or sum < rhs
		c.value = a.value + b.value;
		c.is_overflow = (c.value < a.value && c.value < b.value);
	}

	return c;
}

intop_u64_t intop_u64_add(intop_u64_t a, intop_u64_t b)
{
	intop_u64_t c;

	if (a.is_overflow || b.is_overflow) {
		c.is_overflow = 1;
		c.value = 1;
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

intop_sizet_t intop_sizet_init(size_t v)
{
	intop_sizet_t r = {0, v};
	return r;
}

intop_u32_t intop_u32_init(uint32_t v)
{
	intop_u32_t r = {0, v};
	return r;
}

intop_u64_t intop_u64_init(uint64_t v)
{
	intop_u64_t r = {0, v};
	return r;
}

// E = T+T
// T = E*E
// F = (E)|x|number
//
// becomes...
//
// E  := TE'
// E' := +TE'|e
// T  := FT'
// T' := *FT'|e
// F  := (E)|x|number

static void intop_eval_E(intop_eval_context_t* c);
static void intop_eval_Em(intop_eval_context_t* c);
static void intop_eval_T(intop_eval_context_t* c);
static void intop_eval_Tm(intop_eval_context_t* c);
static void intop_eval_F(intop_eval_context_t* c);

static int intop_check_error(intop_eval_context_t* c)
{
	return (c->s_error != 0);
}

static int intop_eval_check_bounds(intop_eval_context_t* c)
{
	return (c->i < INTOP_EVAL_MAX_N);
}

static void intop_eval_push(intop_eval_context_t* c, intop_eval_token_t* token)
{
	if (c->s_n == INTOP_EVAL_MAX_N) {
		c->s_error = "result stack overflow";
		c->i_error = c->i;
		return;
	}

	c->stack[c->s_n++] = *token;
}

// E  := TE'
static void intop_eval_E(intop_eval_context_t* c)
{
	if (intop_check_error(c))
		return;

	intop_eval_T(c);
	intop_eval_Em(c);
}

// E' := +TE'|-TE'|e
static void intop_eval_Em(intop_eval_context_t* c)
{
	if (intop_check_error(c))
		return;

	if (intop_eval_check_bounds(c) && (c->tokens[c->i].type == INTOP_EVAL_ADD || c->tokens[c->i].type == INTOP_EVAL_SUB)) {
		int i = c->i;
		c->i += 1;
		intop_eval_T(c);
		intop_eval_Em(c);

		intop_eval_push(c, &c->tokens[i]);
	}
}

// T  := FT'
static void intop_eval_T(intop_eval_context_t* c)
{
	if (intop_check_error(c))
		return;

	intop_eval_F(c);
	intop_eval_Tm(c);
}

// T' := *FT'|/FT'e
static void intop_eval_Tm(intop_eval_context_t* c)
{
	if (intop_check_error(c))
		return;

	if (intop_eval_check_bounds(c) && (c->tokens[c->i].type == INTOP_EVAL_MUL || c->tokens[c->i].type == INTOP_EVAL_DIV)) {
		int i = c->i;
		c->i += 1;
		intop_eval_F(c);
		intop_eval_Tm(c);

		intop_eval_push(c, &c->tokens[i]);
	}
}

static void intop_eval_F(intop_eval_context_t* c)
{
	if (intop_check_error(c))
		return;

	if (!intop_eval_check_bounds(c))
		goto failure;

	if (c->tokens[c->i].type == INTOP_EVAL_NUMBER) {
		intop_eval_push(c, &c->tokens[c->i]);
		goto success;
	}

	if (c->tokens[c->i].type == INTOP_EVAL_VARIABLE) {
		intop_eval_push(c, &c->tokens[c->i]);
		goto success;
	}

	if (c->tokens[c->i].type != INTOP_EVAL_LPAREN)
		goto failure;

	c->i += 1;
	intop_eval_E(c);

	if (intop_check_error(c))
		return;

	if (!intop_eval_check_bounds(c) || c->tokens[c->i].type != INTOP_EVAL_RPAREN) {
		c->s_error = "expected ')'";
		c->i_error = c->i;
		return;
	}

success:
	c->i += 1;
	return;

failure:

	c->s_error = "expected '(', number or variable";
	c->i_error = c->i;

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

int intop_eval_compile(const char* s, intop_eval_context_t* c, const char** error)
{
	// initialize context
	c->i = 0;
	c->n = 0;
	c->s_n = 0;
	c->e_n = 0;
	c->s_error = 0;
	c->i_error = 0;

	int n_variables = 0;

	// tokenize
	char token[32];
	int i_token = 0;

	while (*s) {
		if (c->n >= INTOP_EVAL_MAX_N) {
			*error = "too many tokens";
			return 0;
		}

		switch (*s) {
			case '(':
				c->tokens[c->n].type = INTOP_EVAL_LPAREN;
				c->n += 1;
				s += 1;
				break;
			case ')':
				c->tokens[c->n].type = INTOP_EVAL_RPAREN;
				c->n += 1;
				s += 1;
				break;
			case ' ':
			case '\t':
				s += 1;
				break;
			case '-':
				c->tokens[c->n].type = INTOP_EVAL_SUB;
				c->n += 1;
				s += 1;
				break;
			case '+':
				c->tokens[c->n].type = INTOP_EVAL_ADD;
				c->n += 1;
				s += 1;
				break;
			case '/':
				c->tokens[c->n].type = INTOP_EVAL_DIV;
				c->n += 1;
				s += 1;
				break;
			case '*':
				c->tokens[c->n].type = INTOP_EVAL_MUL;
				c->n += 1;
				s += 1;
				break;
			case 'x':
				c->tokens[c->n].type = INTOP_EVAL_VARIABLE;
				c->tokens[c->n].var_index = n_variables;
				c->n += 1;
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

				c->tokens[c->n].type = INTOP_EVAL_NUMBER;
				c->tokens[c->n].number.is_overflow = 0;
				c->tokens[c->n].number.value = atoi(token);

				if (!parse_number(token, &c->tokens[c->n].number.value)) {
					*error = "invalid number";
					return 0;
				}

				i_token = 0;
				c->n += 1;

				break;

			default:
				*error = "invalid token";
				return 0;
		}
	}

	if (c->n == 0) {
		*error = "no tokens";
		return 0;
	}

	intop_eval_E(c);

	if (intop_check_error(c)) {
		*error = c->s_error;
		return 0;
	}

	if (c->i != c->n) {
		*error = "some parsing error";
		return 0;
	}

	return 1;
}

int intop_eval_eval(intop_eval_context_t* c, uint64_t* r, const char** error, ...)
{
	// reset context
	c->e_n = 0;
	c->s_error = 0;
	c->i_error = 0;

	intop_u64_t a, b;
	int i;

	for (i = 0; i < c->s_n; i++) {
		switch (c->stack[i].type) {
			case INTOP_EVAL_NUMBER:
				c->eval[c->e_n] = c->stack[i];
				c->e_n += 1;
				break;
			case INTOP_EVAL_VARIABLE:
				*error = "not supported yet";
				return 0;
			case INTOP_EVAL_SUB:
			case INTOP_EVAL_ADD:
			case INTOP_EVAL_DIV:
			case INTOP_EVAL_MUL:
				assert(c->e_n >= 2);
				assert(c->eval[c->e_n - 1].type == INTOP_EVAL_NUMBER);
				assert(c->eval[c->e_n - 2].type == INTOP_EVAL_NUMBER);
				a = c->eval[c->e_n - 1].number;
				b = c->eval[c->e_n - 2].number;
				c->eval[c->e_n - 2].type = INTOP_EVAL_NUMBER;

				if (c->stack[i].type == INTOP_EVAL_SUB) {
					if (b.value > a.value) {
						*error = "underflow";
						return 0;
					}

					c->eval[c->e_n - 2].number.is_overflow = a.is_overflow || b.is_overflow;
					c->eval[c->e_n - 2].number.value = a.value - b.value;

				} else if (c->stack[i].type == INTOP_EVAL_ADD) {
					c->eval[c->e_n - 2].number = intop_u64_add(a, b);
				} else if (c->stack[i].type == INTOP_EVAL_DIV) {
					if (b.value == 0) {
						*error = "division by zero";
						return 0;
					}

					c->eval[c->e_n - 2].number.is_overflow = a.is_overflow || b.is_overflow;
					c->eval[c->e_n - 2].number.value = a.value / b.value;
				} else {
					c->eval[c->e_n - 2].number = intop_u64_mult(a, b);
				}

				c->e_n -= 1;
				break;
			default:
				*error = "invalid token";
				return 0;
		}
	}

	if (c->e_n != 1 || c->eval[0].type != INTOP_EVAL_NUMBER) {
		*error = "compile/eval error";
		return 0;
	}

	if (c->eval[0].number.is_overflow) {
		*error = "eval overflow";
		return 0;
	}

	*r = c->eval[0].number.value;
	return 1;
}
