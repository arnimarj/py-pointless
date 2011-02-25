#!/usr/bin/python

import sys, random, unittest, types

from common import pointless

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
		return pointless.PointlessPrimVector(tc, sequence = (random.uniform(-10000.0, 10000.0) for i in xrange(n)))

	return pointless.PointlessPrimVector(tc, sequence = (random.randint(i_min, i_max) for i in xrange(n)))

class TestPrimVector(unittest.TestCase):
	def testIndex(self):
		for tc in ['i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'f']:
			for i in xrange(100):
				if i > 0 and i % 10 == 0:
					sys.stdout.write('.')

					if i % 100 == 0:
						sys.stdout.write(' ')

					sys.stdout.flush()

				v = RandomPrimVector(i, tc, pointless)
				w = list(v)

				for i in xrange(len(w)):
					self.assert_(w.index(w[i]) == v.index(w[i]))
					self.assert_(w.index(v[i]) == v.index(v[i]))

				for i in xrange(1000):
					if len(w) > 0:
						self.assert_((i in w) == (i in v))

						if i in w:
							continue

						self.assertRaises(ValueError, w.index, i)
						self.assertRaises(ValueError, v.index, i)

			sys.stdout.write(' | ')
			sys.stdout.flush()

	def testRemove(self):
		for tc in ['i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'f']:
			for i in xrange(100):
				if i > 0 and i % 10 == 0:
					sys.stdout.write('.')

					if i % 100 == 0:
						sys.stdout.write(' ')

					sys.stdout.flush()

				v = RandomPrimVector(i, tc, pointless)
				w = list(v)

				for i in xrange(100):
					if len(w) > 0:
						V = random.choice(v)

						w.remove(V)
						v.remove(V)
						assert(w == list(v))

						V = random.randint(0, 1000)

						if V not in v:
							self.assertRaises(ValueError, w.remove, V)
							self.assertRaises(ValueError, v.remove, V)


			sys.stdout.write(' | ')
			sys.stdout.flush()

	def testFastRemove(self):
		for tc in ['i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'f']:
			for i in xrange(100):
				if i > 0 and i % 10 == 0:
					sys.stdout.write('.')

					if i % 100 == 0:
						sys.stdout.write(' ')

					sys.stdout.flush()

				v = RandomPrimVector(i, tc, pointless)
				w = list(v)

				for i in xrange(100):
					if len(w) > 0:
						V = random.choice(v)

						w.remove(V)
						v.fast_remove(V)
						assert(sorted(w) == sorted(v))

						V = random.randint(0, 1000)

						if V not in v:
							self.assertRaises(ValueError, w.remove, V)
							self.assertRaises(ValueError, v.fast_remove, V)

			sys.stdout.write(' | ')
			sys.stdout.flush()

	def testPop(self):
		w = pointless.PointlessPrimVector('u32')
		self.assertRaises(IndexError, w.pop)

		w = pointless.PointlessPrimVector('u32', sequence = xrange(1000))

		self.assert_(len(w) == 1000)

		for i in xrange(1000):
			n = w.pop()
			self.assert_(n == 1000 - i - 1)

		self.assert_(len(w) == 0)
		self.assertRaises(IndexError, w.pop)

	def testTypeCode(self):
		t = ['i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'f']

		for tc in t:
			sys.stdout.write('.')
			sys.stdout.flush()
			r = range(0, 10) if tc != 'f' else [0.0, 1.0, 2.0, 3.0]
			v = pointless.PointlessPrimVector(tc, sequence = r)
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
			sys.stdout.write('.')
			sys.stdout.flush()

			v = pointless.PointlessPrimVector(v_type)
			v.append(v_min)
			v.append(0)
			v.append(v_max)

			a, b, c = v

			self.assert_(a == v_min and b == 0 and c == v_max)

			vv = pointless.PointlessPrimVector(v_type, sequence = v)

			self.assert_(len(v) == len(vv))

			a, b, c = vv

			self.assert_(a == v_min and b == 0 and c == v_max)

			for aa, bb in zip(v, vv):
				self.assert_(aa == bb)

		# illegal values, which must fail
		for v_type, v_min, v_max in int_info:
			v = pointless.PointlessPrimVector(v_type)

			self.assertRaises(ValueError, v.append, v_min - 1)
			self.assertRaises(ValueError, v.append, v_max + 1)

		# floating point
		if True:
			v = pointless.PointlessPrimVector('f')
			v.append(-100.0)
			v.append(-0.5)
			v.append(0.0)
			v.append(+100.0)
			v.append(+0.5)

			vv = pointless.PointlessPrimVector('f', sequence = v)

			self.assert_(len(v) == len(vv))

			for aa, bb in zip(v, vv):
				self.assert_(aa == bb)

			v = pointless.PointlessPrimVector('f')

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

		for i in xrange(100):
			sys.stdout.write('.')
			sys.stdout.flush()

			for tc, i_min, i_max in i_limits:
				for n_min in [0, 1000, 10000, 10000]:
					n = random.randint(0, n_min)
					py_v = [i_min, i_max]
					py_v += [random.randint(i_min, i_max) for i in xrange(n)]
					random.shuffle(py_v)

					pr_v = pointless.PointlessPrimVector(tc, sequence = py_v)

					py_v.sort()
					pr_v.sort()

					self.assert_(len(py_v) == len(pr_v))
					self.assert_(all(a == b for a, b in zip(py_v, pr_v)))

					py_v = [random.uniform(-10000.0, +10000.0) for i in xrange(n)]
					random.shuffle(py_v)
					pr_v = pointless.PointlessPrimVector('f', sequence = py_v)

					py_v.sort()
					pr_v.sort()

					self.assert_(len(py_v) == len(pr_v))
					self.assert_(all(close_enough(a, b) for a, b in zip(py_v, pr_v)))

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
		for i in xrange(10):
			sys.stdout.write('.')
			sys.stdout.flush()

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

			sys.stdout.write('.')
			sys.stdout.flush()

			# create an equivalent primary vector projection, using any of the possible primitive range types
			# since it is important to test them all
			tc = random.choice(tc)
			pp_proj = pointless.PointlessPrimVector(tc, sequence = py_proj)

			sys.stdout.write('.')
			sys.stdout.flush()

			# create 1 to 16 value vectors
			n_attributes = random.randint(1, 16)
			pp_vv = [RandomPrimVector(i_max, None, pointless) for i in xrange(n_attributes)]
			py_vv = [list(pp_vv[i]) for i in xrange(n_attributes)]

			sys.stdout.write('.')
			sys.stdout.flush()

			# run both python and pointless projection sorts
			my_proj_sort(py_proj, py_vv)
			pp_proj.sort_proj(*pp_vv)

			self.assert_(len(py_proj) == len(pp_proj))

			for a, b in zip(py_proj, pp_proj):
				if a != b:
					t_a = [pp_vv[i][a] for i in xrange(n_attributes)]
					t_b = [py_vv[i][b] for i in xrange(n_attributes)]

					# since the pointless sort is not stable, we have to account for equivalence
					if t_a == t_b:
						continue

					self.assert_(False)

	def testSerialize(self):
		random.seed(0)

		tcs = ['i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'f']

		for tc in tcs:
			n_random = range(100)

			for i in xrange(1000):
				n_random.append(random.randint(101, 100000))

			sys.stdout.write('.')
			sys.stdout.flush()

			for n in n_random:
				v_in = RandomPrimVector(n, tc, pointless)
				buffer = v_in.serialize()
				v_out = pointless.PointlessPrimVector(buffer = buffer)

				self.assert_(v_in.typecode == v_out.typecode)
				self.assert_(len(v_in) == len(v_out))
				self.assert_(a == b for a, b in zip(v_in, v_out))

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
			sys.stdout.write('.')
			sys.stdout.flush()

			# select type and range
			v_type, v_min, v_max = random.choice(v_info)

			n = random.randint(0, 1000)

			v_a = pointless.PointlessPrimVector(v_type)
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
