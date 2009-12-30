#!/usr/bin/python

import unittest, itertools

from test_serialize import AllBitvectorTestCases
from common import pointless, VectorSlices

def sign(i):
	return min(max(-1, i), +1)

class TestCmp(unittest.TestCase):
	def setUp(self):
		self.fname_a = 'pointless_cmp_a.map'
		self.fname_b = 'pointless_cmp_b.map'
		self.fname_c = 'pointless_cmp_c.map'

	def _testPythonCmp(self, values):
		for (v_a, v_b) in itertools.product(values, values):
			pointless.serialize(v_a, self.fname_a)
			pointless.serialize(v_b, self.fname_b)

			p_a = pointless.Pointless(self.fname_a).GetRoot()
			p_b = pointless.Pointless(self.fname_b).GetRoot()

			p_cmp_a = sign(pointless.pointless_cmp(p_a, p_b))
			p_cmp_b = sign(pointless.pointless_cmp(p_a, v_b))
			p_cmp_c = sign(pointless.pointless_cmp(v_a, p_b))
			p_cmp_d = sign(pointless.pointless_cmp(v_a, v_b))

			self.assertEqual(p_cmp_a, p_cmp_b)
			self.assertEqual(p_cmp_a, p_cmp_c)
			self.assertEqual(p_cmp_a, p_cmp_d)

			# test against python
			py_cmp = sign(cmp(v_a, v_b))

			# we're only interested in pointless/python compatibility for equality matcing
			self.assert_(not ((py_cmp == 0 or p_cmp_a == 0) and p_cmp_a != py_cmp))

			del p_a
			del p_b

	def _testTotalOrder(self, values):
		# we're testing, for each 3-combination of values
		for (v_a, v_b, v_c) in itertools.product(values, values, values):
			# write the values out
			pointless.serialize(v_a, self.fname_a)
			pointless.serialize(v_b, self.fname_b)
			pointless.serialize(v_c, self.fname_c)

			# load them
			p_a = pointless.Pointless(self.fname_a).GetRoot()
			p_b = pointless.Pointless(self.fname_b).GetRoot()
			p_c = pointless.Pointless(self.fname_c).GetRoot()

			# test (a > b) <=> (a < b)
			cmp_ab = sign(pointless.pointless_cmp(p_a, p_b))
			cmp_ba = sign(pointless.pointless_cmp(p_b, p_a))
			self.assert_(cmp_ab == -cmp_ba)

			cmp_ac = sign(pointless.pointless_cmp(p_a, p_c))
			cmp_ca = sign(pointless.pointless_cmp(p_c, p_a))
			self.assert_(cmp_ac == -cmp_ca)

			cmp_bc = sign(pointless.pointless_cmp(p_b, p_c))
			cmp_cb = sign(pointless.pointless_cmp(p_c, p_b))
			self.assert_(cmp_bc == -cmp_cb)

			# a <= b and b <= a -> a == b
			self.assert_(not (cmp_ab in (-1, 0) and cmp_ba in (-1, 0) and cmp_ab != 0))

			# a <= b and b <= c -> a <= c
			self.assert_(not (cmp_ab in (-1, 0) and cmp_bc in (-1, 0) and cmp_ac not in (-1, 0)))

			# a <= b or b <= a
			self.assert_(cmp_ab in (-1, 0) or cmp_ba in (-1, 0))

			del p_a
			del p_b
			del p_c

	NUMBERS        = [-1000, -100, 0, 100, 1.0, -1.0, 0.0, False, True]
	NONE           = [None]
	STRINGS        = ['hello world', u'Hello world', u'Hello world', '', u'']
	VECTORS        = [[0, 1, 2], [], [[]], ['asdf', None, True, False, [0, 1, 2, ['asdf']]], ['a', ['a']]]
	SLICED_VECTORS = [v_py for (v_py, v_po) in VectorSlices('slice.map')]
	BITVECTORS     = AllBitvectorTestCases(pointless)

	def testNumbers(self):
		self._testPythonCmp(self.NUMBERS)
		self._testTotalOrder(self.NUMBERS)

	def testNone(self):
		self._testPythonCmp(self.NONE)
		self._testTotalOrder(self.NONE)

	def testStrings(self):
		self._testPythonCmp(self.STRINGS)
		self._testTotalOrder(self.STRINGS)

	def testVectors(self):
		self._testPythonCmp(self.VECTORS)
		self._testTotalOrder(self.VECTORS)

	def testSlicedVectors(self):
		self._testPythonCmp(self.SLICED_VECTORS)
		self._testTotalOrder(self.SLICED_VECTORS)

	def testBitVectors(self):
		self._testTotalOrder(self.BITVECTORS)

	def testAllCombinations(self):
		# all in one soup, just to test total-order
		values = self.NUMBERS + self.NONE + self.STRINGS + self.VECTORS + self.SLICED_VECTORS + self.BITVECTORS
		self._testTotalOrder(values)

