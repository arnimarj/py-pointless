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
