#!/usr/bin/python
# coding=latin-1

import sys, types, random, cStringIO, time, unittest, operator

# generate a random value vector of 'n' items
def RandomPrimVector(n, tc, pointless):
	ranges = {
		'i8':  [-128, 127],
		'u8':  [   0, 255],
		'i16': [-600, 600],
		'u16': [   0, 1200],
		'i32': [  -1, 33000],
		'u32': [   0, 66000],
		'f':   [None, None]
	}

	if tc == None:
		tc = random.choice(ranges.keys())

	i_min, i_max = ranges[tc]

	if tc == 'f':
		return pointless.PointlessPrimVector(tc, (random.uniform(-10000.0, 10000.0) for i in xrange(n)))

	return pointless.PointlessPrimVector(tc, (random.randint(i_min, i_max) for i in xrange(n)))


# all cases for bitvectors
def BITVECTORS(pointless):
	return [
		# case 0) all-0
		pointless.PointlessBitvector([0] * 1000),

		# case 1) all-1
		pointless.PointlessBitvector([1] * 1000),

		# case 2) 0-1
		pointless.PointlessBitvector([0] * 400 + [1] * 800),

		# case 3) 1-0
		pointless.PointlessBitvector([1] * 400 + [0] * 800),

		# case 4) less than 27 bits
		pointless.PointlessBitvector([0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1]),

		# case 5) a whole bunch of bits
		pointless.PointlessBitvector([0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1])
	]

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

def ImportPointlessExt():
	p = sys.path

	try:
		sys.path = ['/home/arni/py-pointless/trunk/build/lib.cygwin-1.7.1-i686-2.6/']
		import pointless as _pointless
	finally:
		sys.path = p

	return _pointless

def sign(i):
	return min(max(-1, i), +1)

def pointless_to_string(pointless, p):
	out = cStringIO.StringIO()
	pointless_print(pointless, p, out)
	return out.getvalue()

def pointless_print(pointless, p, out):
	def pointless_print_rec(v, stack):
		if v == None:
			out.write('None')
		elif type(v) == types.BooleanType:
			out.write('True' if v == True else 'False')
		elif type(v) == pointless.PointlessBitvector:
			out.write('')

			for i in xrange(len(v) - 1, -1, -1):
				b = v[i]
				assert(type(b) == types.BooleanType)
				out.write('1' if b else '0')

			out.write('b')
		elif type(v) == types.UnicodeType:
			out.write(repr(v))
		elif type(v) == types.FloatType:
			out.write('%f' % (v,))
		elif type(v) in (types.IntType, types.LongType):
			out.write('%i' % (v,))
		elif type(v) == pointless.PointlessVector:
			out.write('[')

			if len(v) > 0:
				if v.container_id in stack:
					out.write('...')
				else:
					for i, item in enumerate(v):
						assert(v[i] == item)
						stack.append(v.container_id)
						pointless_print_rec(item, stack)
						stack.pop()

						if i + 1 < len(v):
							out.write(', ')

			out.write(']')
		elif type(v) == pointless.PointlessSet:
			out.write('set([')

			if v.container_id in stack:
				out.write('...')
			else:
				for i, item in enumerate(v):
					stack.append(v.container_id)
					pointless_print_rec(item, stack)
					stack.pop()

					if i + 1 < len(v):
						out.write(', ')

			out.write('])')
		else:
			out.write('<TYPE: %s>' % (type(v),))

	pointless_print_rec(p, [])

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

def test_serialize_test_cases(pointless):
	# 1) deep vector
	yield [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]

	# 2) vector of vectors
	yield [range(10 + 1) for i in xrange(10)]

	# 3) randomized bitvector
	random.seed(0)
	v = pointless.PointlessBitvector(100)

	for i in xrange(40):
		v[random.randint(0, 100 - 1)] = 1

	yield v

	# 4) mixed vector
	yield [1.0, None, [1.0, True, False]]

	# 5) set
	yield set([0, 1, 2, 3, 4, 5, 6, 7, 8, 9])

	# 6) empty set
	yield set()

	# 7) single item set
	yield set([1])

	# 8) mixed set
	yield set([0, 0.0, 1.0, 1, True])

	# 9) bitvectors
	for bv in BITVECTORS(pointless):
		yield bv

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

def test_print(pointless):
	fname = 'test_print.map'

	for i, v in enumerate(test_serialize_test_cases(pointless)):
		#print 'TEST: print base %i' % (i + 1,)
		pointless.serialize(v, fname)
		p = pointless.Pointless(fname)
		root = p.GetRoot()
		s_root = cStringIO.StringIO()
		s_python = cStringIO.StringIO()

		print >> s_root, root,
		pointless_print(pointless, root, s_python)

		if s_root.getvalue() != s_python.getvalue():
			#print 'Pointless: "%s"' % (s_root.getvalue(),)
			#print 'Python:    "%s"' % (s_python.getvalue(),)
			raise PointlessTestFailure('print test failure, case %i' % (i + 1,))

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

class TestPrimVector(unittest.TestCase):
	def setUp(self):
		self.pointless = ImportPointlessExt()

	def testTypeCode(self):
		t = ['i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'f']

		for tc in t:
			r = range(0, 10) if tc != 'f' else [0.0, 1.0, 2.0, 3.0]
			v = self.pointless.PointlessPrimVector(tc, r)
			self.assert_(v.typecode == tc)

	def testPrimVector(self):
		# integer types, and their ranges
		int_info = [
			['i8', -2**7, 2**7-1],
			['u8', 0, 2**8-1],
			['i16', -2**15, 2**15-1],
			['u16', 0, 2**16-1],
			['i32', -2**31, 2**31-1],
			['u32', 0, 2**32-1]
		]

		# legal values, and their output, plus an iterator based creation, and item-by-item comparison
		for v_type, v_min, v_max in int_info:
			v = self.pointless.PointlessPrimVector(v_type)
			v.append(v_min)
			v.append(0)
			v.append(v_max)

			a, b, c = v

			self.assert_(a == v_min and b == 0 and c == v_max)

			vv = self.pointless.PointlessPrimVector(v_type, v)

			self.assert_(len(v) == len(vv))

			a, b, c = vv

			self.assert_(a == v_min and b == 0 and c == v_max)

			for aa, bb in zip(v, vv):
				self.assert_(aa == bb)

		# illegal values, which must fail
		for v_type, v_min, v_max in int_info:
			v = self.pointless.PointlessPrimVector(v_type)

			self.assertRaises(ValueError, v.append, v_min - 1)
			self.assertRaises(ValueError, v.append, v_max + 1)

		# floating point
		if True:
			v = self.pointless.PointlessPrimVector('f')
			v.append(-100.0)
			v.append(-0.5)
			v.append(0.0)
			v.append(+100.0)
			v.append(+0.5)

			vv = self.pointless.PointlessPrimVector('f', v)

			self.assert_(len(v) == len(vv))

			for aa, bb in zip(v, vv):
				self.assert_(aa == bb)

			v = self.pointless.PointlessPrimVector('f')

			self.assertRaises(TypeError, v.append, 0)

	def testSort(self):
		random.seed(0)

		i_limits = [
			('i8',  -128, 127),
			('u8',     0, 255),
			('i16', -600, 600),
			('u16',    0, 1200),
			('i32',   -1, 33000),
			('u32',    0, 66000)
		]

		def close_enough(v_a, v_b):
			return (abs(v_a - v_b) < 0.001)

		tt_a = 0.0
		tt_b = 0.0

		for i in xrange(100):
			for tc, i_min, i_max in i_limits:
				for n_min in [0, 1000, 10000, 10000]:
					n = random.randint(0, n_min)
					py_v = [i_min, i_max]
					py_v += [random.randint(i_min, i_max) for i in xrange(n)]
					random.shuffle(py_v)

					pr_v = self.pointless.PointlessPrimVector(tc, py_v)

					t_0 = time.time()
					py_v.sort()
					t_1 = time.time()
					pr_v.sort()
					t_2 = time.time()

					tt_a += t_1 - t_0
					tt_b += t_2 - t_1

					self.assert_(len(py_v) == len(pr_v))
					self.assert_(all(a == b for a, b in zip(py_v, pr_v)))

					py_v = [random.uniform(-10000.0, +10000.0) for i in xrange(n)]
					random.shuffle(py_v)
					pr_v = self.pointless.PointlessPrimVector('f', py_v)

					t_0 = time.time()
					py_v.sort()
					t_1 = time.time()
					pr_v.sort()
					t_2 = time.time()

					tt_a += t_1 - t_0
					tt_b += t_2 - t_1

					self.assert_(len(py_v) == len(pr_v))
					self.assert_(all(close_enough(a, b) for a, b in zip(py_v, pr_v)))

		#print
		#print '     prim sort %.2fx faster' % (tt_a / tt_b,)

	def testProjSort(self):
		# pure python projection sort
		def my_proj_sort(proj, v):
			def proj_cmp(i_a, i_b):
				for vv in v:
					c = cmp(vv[i_a], vv[i_b])
					if c != 0:
						return c
				return 0
			proj.sort(cmp = proj_cmp)

		random.seed(0)

		# run some number of iterations
		tt_a = 0.0
		tt_b = 0.0

		for i in xrange(100):
			# generate projection with indices in the range [i_min, i_max[
			i_min = random.randint(0, 1000)
			i_max = random.randint(i_min, i_min + 60000)
			py_proj = range(i_min, i_max)

			tc = ['i32', 'u32']

			if i_max < 2**16:
				tc.append('u16')
			if i_max < 2**15:
				tc.append('i16')
			if i_max < 2**8:
				tc.append('u8')
			if i_max < 2**7:
				tc.append('i8')

			# create an equivalent primary vector projection, using any of the possible primitive range types
			# since it is important to test them all
			tc = random.choice(tc)
			pp_proj = self.pointless.PointlessPrimVector(tc, py_proj)

			# create 1 to 16 value vectors
			n_attributes = random.randint(1, 16)
			pp_vv = [RandomPrimVector(i_max, None, self.pointless) for i in xrange(n_attributes)]
			py_vv = [list(pp_vv[i]) for i in xrange(n_attributes)]

			# run both python and pointless projection sorts
			t_0 = time.time()
			my_proj_sort(py_proj, py_vv)
			t_1 = time.time()
			pp_proj.sort_proj(*pp_vv)
			t_2 = time.time()

			tt_a += t_1 - t_0
			tt_b += t_2 - t_1

			self.assert_(len(py_proj) == len(pp_proj))

			for a, b in zip(py_proj, pp_proj):
				if a != b:
					t_a = [pp_vv[i][a] for i in xrange(n_attributes)]
					t_b = [py_vv[i][b] for i in xrange(n_attributes)]
					
					# since the pointless sort is not stable, we have to account for equivalence
					if t_a == t_b:
						continue

					self.assert_(False)

		#print
		#print '     proj sort %.2fx faster' % (tt_a / tt_b,)

	def testSerialize(self):
		random.seed(0)

		tcs = ['i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'f']

		for tc in tcs:
			n_random = range(100)

			for i in xrange(1000):
				n_random.append(random.randint(101, 100000))

			for n in n_random:
				v_in = RandomPrimVector(n, tc, self.pointless)
				buffer = v_in.serialize()
				v_out = self.pointless.PointlessPrimVector(buffer)

				self.assert_(v_in.typecode == v_out.typecode)
				self.assert_(len(v_in) == len(v_out))
				self.assert_(a == b for a, b in zip(v_in, v_out))

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

	def testSlice(self):
		# vector types, and their ranges
		v_info = [
			['i8',   -2**7,   2**7 - 1],
			['u8',       0,   2**8 - 1],
			['i16', -2**15,  2**15 - 1],
			['u16',      0,  2**16 - 1],
			['i32', -2**31,  2**31 - 1],
			['u32',      0,  2**32 - 1],
			['f',     None,       None]
		]

		random.seed(0)

		def v_eq(v_a, v_b):
			if len(v_a) != len(v_b):
				return False

			for a, b in zip(v_a, v_b):
				if type(a) == types.FloatType and type(b) == types.FloatType:
					if not (abs(a - b) < 0.001):
						return False
				elif a != b:
					return False

			return True

		# we do multiple iterations
		for i in xrange(100):
			# select type and range
			v_type, v_min, v_max = random.choice(v_info)

			n = random.randint(0, 1000)

			v_a = self.pointless.PointlessPrimVector(v_type)
			v_b = [ ]

			for j in xrange(n):
				if v_type == 'f':
					v = random.uniform(-10000.0, 10000.0)
				else:
					v = random.randint(v_min, v_max)

				v_a.append(v)
				v_b.append(v)

			for j in xrange(1000):
				i_min = random.randint(-1000, 1000)
				i_max = random.randint( 1000, 1000)

				s_a = v_a[:]
				s_b = v_b[:]

				self.assert_(v_eq(s_a, s_b))

				s_a = v_a[i_min:]
				s_b = v_b[i_min:]

				self.assert_(v_eq(s_a, s_b))

				s_a = v_a[:i_max]
				s_b = v_b[:i_max]

				self.assert_(v_eq(s_a, s_b))

				s_a = v_a[i_min:i_max]
				s_b = v_b[i_min:i_max]

				self.assert_(v_eq(s_a, s_b))

				try:
					s_a = [v_a[i_min]]
				except IndexError:
					s_a = None

				try:
					s_b = [v_b[i_min]]
				except IndexError:
					s_b = None

				self.assert_((s_a == None) == (s_b == None))
				self.assert_(s_a == None or v_eq(s_a, s_b))

				try:
					s_a = [v_a[i_max]]
				except IndexError:
					s_a = None

				try:
					s_b = [v_b[i_max]]
				except IndexError:
					s_b = None

				self.assert_((s_a == None) == (s_b == None))
				self.assert_(s_a == None or v_eq(s_a, s_b))
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

def main():
	#pointless = ImportPointlessExt()
	#test_set_performance(pointless)
	
	#unittest.main()
	#s = suite()
	#print s
	suite = unittest.TestSuite(map(TestVector, ['testSerialize']))
	unittest.TextTestRunner(verbosity=2).run(suite)


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

if __name__ == '__main__':
	main()
