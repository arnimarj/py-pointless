#!/usr/bin/python
# coding=latin-1

from twisted.trial import unittest
import sys, types, random, cStringIO, time, operator

def test_set_performance(pointless):
	#print 'INFO: set performance test'
	fname = 'set_performance.map'

	t_0 = time.time()
	n_items = 1000000
	v = range(n_items)
	random.shuffle(v)
	s = set(v)
	t_1 = time.time()

	print '\tdata generation time: %.2fs' % (t_1 - t_0,)

	t_0 = time.time()
	pointless.serialize(s, fname)
	t_1 = time.time()

	print '\tserialize time: %.2fs' % (t_1 - t_0,)

	t_0 = time.time()
	root = pointless.Pointless(fname).GetRoot()
	t_1 = time.time()

	print '\topen and validation time: %.2f' % (t_1 - t_0,)

	c = 0

	random.seed(0)
	random.shuffle(v)

	t_0 = time.time()

	for i in xrange(n_items):
		if i in root:
			c += 1

	t_1 = time.time()

	if c != n_items:
		raise PointlessTestFailure('error A during set performance test')

	print '\tpointless query time: %.2fs' % (t_1 - t_0,)

	t_0 = time.time()

	c = 0

	for i in xrange(n_items):
		if i in s:
			c += 1

	t_1 = time.time()

	if c != n_items:
		raise PointlessTestFailure('error A during set performance test')

	print '\tpython query time: %.2fs' % (t_1 - t_0,)

	print float(len(v)) / (t_1 - t_0), 'per second'

def test_vector_slice(pointless):
	fname = 'test_vector_slice.map'

	for v_py, v_po in VECTOR_SLICES(pointless, fname):
		c = pointless.pointless_cmp(v_py, v_po)

		if c != 0:
			raise PointlessTestFailure('slice comparison of arrays failure')

def test_serialize_leak(pointless):
	fname = 'test_serialize_leak.map'

	n = 10000
	n_items = 100000

	for i in xrange(n):
		#print 'iteration %i' % (i,)
		#sys.stdout.flush()

		v = range(n_items)
		pointless.serialize(v, fname)
		p = pointless.Pointless(fname)
		root = p.GetRoot()

		d = dict((i, '%i' % (i,)) for i in xrange(n))
		pointless.serialize(d, fname)
		p = pointless.Pointless(fname)
		root = p.GetRoot()

		s = set(xrange(n))
		pointless.serialize(s, fname)
		p = pointless.Pointless(fname)
		root = p.GetRoot()

	'''
	def testCompression(self):
		# None, or compressed vector type -> values in vector
		vm = [
			('i8',  range(-128, 128)),
			('u8',  range(0, 256)),
			('i16', range(-600, 600)),
			('u16', range(0, 1200)),
			('i32', range(-1, 33000)),
			('u32', range(0, 66000)),
			('f',   [0.0, 1.0]),
			(None,  []),
			(None,  ['asdf', 'foo'])
		]

		fname = 'typecode.map'

		for tc, v_a in vm:
			self.pointless.serialize(v_a, fname)
			v_b = self.pointless.Pointless(fname).GetRoot()

			if tc == None:
				self.assertRaises(ValueError, operator.attrgetter('typecode'), v_b)
			else:
				self.assert_(v_b.typecode == tc)

		del v_b

	'''
	def testMixedVectors(self):
		v = [
			[1.0, 2.0, 3  ],  # 001
			[1.0, 2  , 3.0],  # 010
			[1.0, 2  , 3  ],  # 011
			[1  , 2.0, 3.0],  # 100
			[1  , 2.0, 3  ],  # 101
			[1  , 2  , 3.0]   # 110
		]

		def eq_v(i, j):
			a = isinstance(i, (types.IntType, types.LongType))
			b = isinstance(i, (types.IntType, types.LongType))

			if a and b:
				return i == j

			if isinstance(i, types.FloatType):
				return False

			if isinstance(j, types.FloatType):
				return False

			return abs(i - j) < 0.001

		for v_a in v:
			self.pointless.serialize(v_a, 'mixed.map')
			v_b = self.pointless.Pointless('mixed.map').GetRoot()

			self.assert_(len(v_a) == 3 and len(v_b) == 3)
			self.assert_(all(eq_v(a, b) for a, b in zip(v_a, v_b)))

			del v_b


def main_():

	# test serialization
	# test_serialize(pointless)

	# test comparisons
	#test_cmp(pointless)

	# test printing
	#test_print(pointless)

	# test set containment
	#test_set_contains(pointless)

	# test set performance
	#test_set_performance(pointless)

	# test map
	#test_map(pointless)

	# test vector slice
	#test_vector_slice(pointless)

	# test bitvector comparison
	#test_bitvector_cmp(pointless)

	# test map lookup when mixing unicodes and strings
	#test_map_unicode_str_lookup(pointless)

	# test mixed vectors
	test_mixed_vectors(pointless)

	# test primitive vector
	test_prim_vector(pointless);
	test_prim_vector_slice(pointless)
	test_prim_vector_sort(pointless);
	test_prim_vector_sort_proj(pointless)

	# test typecode params
	test_typecode(pointless)

	# test prim vector serialiazation
	test_prim_vector_serialize(pointless)

	# test for obvious memory leaks, caller must observe memory usage using top or something (EXPENSIVE)
	# test_serialize_leak(pointless)

	# if no exceptions, we're good
	#print 'INFO: OK'
