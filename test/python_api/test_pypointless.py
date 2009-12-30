#!/usr/bin/python
# coding=latin-1

import sys, types, random, cStringIO, time, unittest, operator




def test_serialize(pointless):
	fname = 'test_serialize.map'

	for i, v in enumerate(test_serialize_test_cases(pointless)):
		#print 'TEST: serialize case %i' % (i + 1,), fname
		pointless.serialize(v, fname)
		p = pointless.Pointless(fname)
		root = p.GetRoot()

		# make sure file handle is closed, windows
		# really doesn't like us writing over open files
		del root
		del p


def test_set_contains(pointless):
	def list_to_tuple_rec(i):
		if type(i) == types.ListType:
			return tuple(map(list_to_tuple_rec, i))
		else:
			return i

	fname_a = 'test_set_contains_a.map'
	fname_b = 'test_set_contains_b.map'
	fname_c = 'test_set_contains_c.map'

	vectors = [
		[], [[]], [[], []],
		[[[], False], False, False, False],
		[0], ['hello world'],
		[0, u'hello'], ['hello world', (1, 2, 3)],
		[False, True, 1.0, -1.0, 0.0, 1, 200],
		[0, 1, 2, 3, 4],
		['a', 'b', 'c', ['d', 'e', 1.0]],
		[None, [None]],
		[range(10), range(100), range(1000), [range(10), ['asdf'] * 10]],
		[[0]], [[-1]], [[-1.0, 0.0]], [[0, False]], [[0, True]],
		[range(10, -100, -1), [[range(10)]]]
	]

	for test_i, v in enumerate(vectors):
		#print 'TEST: set.contains() %i' % (test_i + 1,)
		vv = list_to_tuple_rec(v)
		pointless.serialize(set(vv), fname_a)
		root_a = pointless.Pointless(fname_a).GetRoot()

		for i in v:
			ii = list_to_tuple_rec(i)

			pointless.serialize(i, fname_b)
			root_b = pointless.Pointless(fname_b).GetRoot()

			pointless.serialize(ii, fname_c)
			root_c = pointless.Pointless(fname_c).GetRoot()

			if ii not in root_a:
				raise PointlessTestFailure('set.contains() failure A')

			if root_b not in root_a:
				raise PointlessTestFailure('set.contains() failure B')

			if root_c not in root_a:
				raise PointlessTestFailure('set.contains() failure C')

			del root_b
			del root_c

		del root_a

def test_map(pointless):
	# maps
	maps = [
		{1: 0},
		{2: 'asdf'},
		{'asdf':2},
		{1:2, 2:3, 4:4, 'asdf': 123, (1,2):[4,5,'hello world']}
	]

	maps[3][(1,2)].append(maps)

	fname = 'test_map.map'

	for test_i, m in enumerate(maps):
		#print 'TEST: map.contains()'

		pointless.serialize(m, fname)
		root_a = pointless.Pointless(fname).GetRoot()

		# simple for now
		for k in m.iterkeys():
			if k not in root_a:
				raise PointlessTestFailure('map.contains() failure A')

			v_m = m[k]
			v_a = root_a[k]

			c = pointless.pointless_cmp(v_m, v_a)

			if c != 0:
				#print type(v_m), v_m
				#print type(v_a), v_a
				raise PointlessTestFailure('map.contains() failure B')

		del root_a

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

def test_bitvector_cmp(pointless):
	fname = 'test_bitvector_cmp.map'

	for i, py in enumerate(BITVECTORS(pointless)):
		pointless.serialize(py, fname)
		po = pointless.Pointless(fname).GetRoot()

		c = pointless.pointless_cmp(py, po)

		if c != 0:
			raise PointlessTestFailure('bitvector comparison failed, case %i' % (i,))

		del po

def test_map_unicode_str_lookup(pointless):
	v = {
		'abcdef': 1,
		u'abc': 2
	}

	fname = 'test_uncode_str_lookup.map'

	pointless.serialize(v, fname)
	vv = pointless.Pointless(fname).GetRoot()

	for a, b in v.iteritems():
		if a not in vv:
			raise PointlessTestFailure('key not found in map, as it should')

		bb = vv[a]

		if b != bb:
			raise PointlessTestFailure('map values do not match')

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
			a = (type(i) in (types.IntType, types.LongType))
			b = (type(j) in (types.IntType, types.LongType))

			if a and b:
				return i == j

			if type(i) != types.FloatType:
				return False

			if type(j) != types.FloatType:
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
