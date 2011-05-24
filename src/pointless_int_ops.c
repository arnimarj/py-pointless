#include <pointless/pointless_int_ops.h>

// the multiply overflow check is from "Secure Coding in C and C++", chapter 5

int sizet_mult(size_t a, size_t b, size_t* out)
{
	if (a != 0 && b > SIZE_MAX / a)
		return 0;

	*out = a * b;
	return 1;
}

int u64_mult(uint64_t a, uint64_t b, uint64_t* out)
{
	if (a != 0 && b > UINT64_MAX / a)
		return 0;

	*out = a * b;
	return 1;
}

int u32_mult(uint32_t a, uint32_t b, uint32_t* out)
{
	if (a != 0 && b > UINT32_MAX / a)
		return 0;

	*out = a * b;
	return 1;
}

int sizet_add_2(size_t a, size_t b, size_t* out)
{
	// sum = lhs + rhs
	// overflow iff: sum < lhs or sum < rhs
	*out = a + b;
	return (*out >= a && *out >= b);
}

int sizet_add_3(size_t a, size_t b, size_t c, size_t* out)
{
	return (sizet_add_2(a, b, out) && sizet_add_2(*out, c, out));
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

*/
