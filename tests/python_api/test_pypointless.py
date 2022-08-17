#!/usr/bin/python

import operator, pointless, random

from twisted.trial import unittest

from .common import VectorSlices

class TestPointless(unittest.TestCase):
	def test_vector_slice(self):
		random.seed(0)
		vector = list(range(1000))

		for v_py, v_po in VectorSlices('deleteme.map', vector):
			c = pointless.pointless_cmp(v_py, v_po)
			self.assertEquals(c, 0)

	# slow test to check for an old regression
	'''
	def test_serialize_leak(self):
		fname = 'deleteme.map'

		n = 10000
		n_items = 100000

		for i in xrange(n):
			if i % 100 == 0:
				print float(i) / n

			v = range(n_items)
			pointless.serialize(v, fname)
			p = pointless.Pointless(fname)
			p.GetRoot()

			d = dict((i, '%i' % (i,)) for i in xrange(n))
			pointless.serialize(d, fname)
			p = pointless.Pointless(fname)
			p.GetRoot()

			s = set(xrange(n))
			pointless.serialize(s, fname)
			p = pointless.Pointless(fname)
			p.GetRoot()
	'''

	def testCompression(self):
		# None, or compressed vector type -> values in vector
		vm = [
			('i8',  list(range(-128, 128))),
			('u8',  list(range(0, 256))),
			('i16', list(range(-600, 600))),
			('u16', list(range(0, 1200))),
			('i32', list(range(-1, 33000))),
			('u32', list(range(0, 66000))),
			('f',   [0.0, 1.0]),
			(None,  []),
			(None,  ['asdf', 'foo'])
		]

		fname = 'typecode.map'

		for tc, v_a in vm:
			pointless.serialize(v_a, fname)
			v_b = pointless.Pointless(fname).GetRoot()

			if tc == None:
				self.assertRaises(ValueError, operator.attrgetter('typecode'), v_b)
			else:
				self.assert_(v_b.typecode == tc)

		del v_b

	def testMixedVectors(self):
		v = [
			[1.0, 2.0, 3  ],  # 001
			[1.0, 2  , 3.0],  # 010
			[1.0, 2  , 3  ],  # 011
			[1  , 2.0, 3.0],  # 100
			[1  , 2.0, 3  ],  # 101
			[1  , 2  , 3.0]   # 110
		]

		for v_a in v:
			pointless.serialize(v_a, 'mixed.map')
			v_b = pointless.Pointless('mixed.map').GetRoot()

			self.assertEquals(len(v_a), 3)
			self.assertEquals(len(v_b), 3)

			for a, b in zip(v_a, v_b):
				if isinstance(a, int) and isinstance(b, int):
					self.assertEquals(a, b)
				else:
					self.assertApproximates(a, b, 0.001)

			del v_b
