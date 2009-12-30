#!/usr/bin/python
# coding=latin-1

import sys, types, random, cStringIO, time, unittest, operator


# many cases for vector slices
def VECTOR_SLICES(pointless, fname):
	py = range(10000)
	pointless.serialize(py, fname)
	po = pointless.Pointless(fname).GetRoot()

	vv = [ ]

	c = pointless.pointless_cmp(py, po)

	if c != 0:
		raise PointlessTestFailure('initial comparison of VECTOR_SLICES failure')

	vv.append((py, po))

	random.seed(0)

	for i in xrange(100):
		i_0 = random.randint(-100, 15000)
		i_1 = random.randint(-100, 15000)

		s_py = py[i_0:i_1]
		s_po = po[i_0:i_1]

		vv.append((s_py, s_po))

		for j in xrange(100):
			ii_0 = random.randint(-100, 6000)
			ii_1 = random.randint(-100, 6000)

			ss_py = s_py[ii_0:ii_1]
			ss_po = s_po[ii_0:ii_1]

			vv.append((ss_py, ss_po))

	return vv


def sign(i):
	return min(max(-1, i), +1)

def pointless_to_string(pointless, p):
	out = cStringIO.StringIO()
	pointless_print(pointless, p, out)
	return out.getvalue()

def pointless_test_cmp_python(values, fname_a, fname_b, pointless):
	for v_a in values:
		for v_b in values:
			pointless.serialize(v_a, fname_a)
			pointless.serialize(v_b, fname_b)

			p_a = pointless.Pointless(fname_a).GetRoot()
			p_b = pointless.Pointless(fname_b).GetRoot()

			p_cmp_a = sign(pointless.pointless_cmp(p_a, p_b))
			p_cmp_b = sign(pointless.pointless_cmp(p_a, v_b))
			p_cmp_c = sign(pointless.pointless_cmp(v_a, p_b))
			p_cmp_d = sign(pointless.pointless_cmp(v_a, v_b))

			if p_cmp_a != p_cmp_b:
				raise PointlessTestFailure('comparison test failure A')

			if p_cmp_a != p_cmp_c:
				raise PointlessTestFailure('comparison test failure B')

			if p_cmp_a != p_cmp_d:
				raise PointlessTestFailure('comparison test failure C')

			# test against python
			py_cmp = sign(cmp(v_a, v_b))

			# we're only interested in pointless/python compatibility for equality matcing
			if (py_cmp == 0 or p_cmp_a == 0) and p_cmp_a != py_cmp:
				raise PointlessTestFailure('comparison test failure D')

			del p_a
			del p_b

def pointless_test_cmp_total_order(values, fname_a, fname_b, fname_c, pointless):
	for v_a in values:
		for v_b in values:
			for v_c in values:
				pointless.serialize(v_a, fname_a)
				pointless.serialize(v_b, fname_b)
				pointless.serialize(v_c, fname_c)

				p_a_ = pointless.Pointless(fname_a)
				p_a = p_a_.GetRoot()
				p_b_ = pointless.Pointless(fname_b)
				p_b = p_b_.GetRoot()
				p_c_ = pointless.Pointless(fname_c)
				p_c = p_c_.GetRoot()

				cmp_ab = sign(pointless.pointless_cmp(p_a, p_b))
				cmp_ba = sign(pointless.pointless_cmp(p_b, p_a))

				cmp_ac = sign(pointless.pointless_cmp(p_a, p_c))
				cmp_ca = sign(pointless.pointless_cmp(p_c, p_a))

				cmp_bc = sign(pointless.pointless_cmp(p_b, p_c))
				cmp_cb = sign(pointless.pointless_cmp(p_c, p_b))

				if cmp_ab != -cmp_ba:
					raise PointlessTestFailure('comparison test failure E')

				if cmp_ac != -cmp_ca:
					raise PointlessTestFailure('comparison test failure F')

				if cmp_bc != -cmp_cb:
					raise PointlessTestFailure('comparison test failure G')

				# a <= b and b <= a -> a == b
				if cmp_ab in (-1, 0) and cmp_ba in (-1, 0) and cmp_ab != 0:
					raise PointlessTestFailure('comparison test failure H')

				# a <= b and b <= c -> a <= c
				if cmp_ab in (-1, 0) and cmp_bc in (-1, 0) and cmp_ac not in (-1, 0):
					raise PointlessTestFailure('comparison test failure I')

				# a <= b or b <= a
				if not (cmp_ab in (-1, 0) or cmp_ba in (-1, 0)):
					raise PointlessTestFailure('comparison test failure J')

				del p_a
				del p_b
				del p_c

				del p_a_
				del p_b_
				del p_c_

def test_cmp(pointless):
	fname_a = 'pointless_cmp_a.map'
	fname_b = 'pointless_cmp_b.map'
	fname_c = 'pointless_cmp_c.map'

	# numbers
	#print 'TEST: number comparison'
	numbers = [-1000, -100, 0, 100, 1.0, -1.0, 0.0, False, True]
	#pointless_test_cmp_python(numbers, fname_a, fname_b, pointless)
	#pointless_test_cmp_total_order(numbers, fname_a, fname_b, fname_c, pointless)

	# None
	#print 'TEST: none comparison'
	none = [None]
	#pointless_test_cmp_python(none, fname_a, fname_b, pointless)
	#pointless_test_cmp_total_order(none, fname_a, fname_b, fname_c, pointless)

	# strings
	#print 'TEST: string comparison'
	strings = ['hello world', u'Hello world', u'Hello world', '', u'']
	#pointless_test_cmp_python(strings, fname_a, fname_b, pointless)
	#pointless_test_cmp_total_order(strings, fname_a, fname_b, fname_c, pointless)

	# vectors
	#print 'TEST: vector comparison'
	vectors = [[0, 1, 2], [], [[]], ['asdf', None, True, False, [0, 1, 2, ['asdf']]], ['a', ['a']]]
	#pointless_test_cmp_python(vectors, fname_a, fname_b, pointless)
	#pointless_test_cmp_total_order(vectors, fname_a, fname_b, fname_c, pointless)

	# sliced vectors
	#print 'TEST: sliced vector comparisons'
	#sliced_vectors = [v_py for (v_py, v_po) in VECTOR_SLICES(pointless, fname_a)]

	# debug stuff
	sliced_vectors = [ ]
	#pointless_test_cmp_python(sliced_vectors, fname_a, fname_b, pointless)
	#pointless_test_cmp_total_order(sliced_vectors, fname_a, fname_b, fname_c, pointless)

	# bitvectors
	#print 'TEST: bitvector comparison'
	bitvectors = BITVECTORS(pointless)
	pointless_test_cmp_total_order(bitvectors, fname_a, fname_b, fname_c, pointless)

	# all in one soup, just to test total-order
	#print 'TEST: total order in comparison'
	soup = numbers + none + strings + vectors + sliced_vectors + bitvectors
	pointless_test_cmp_total_order(soup, fname_a, fname_b, fname_c, pointless)

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
