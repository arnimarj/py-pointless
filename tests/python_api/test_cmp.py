#!/usr/bin/python

import itertools, pointless, os

from twisted.trial import unittest

from .test_serialize import AllBitvectorTestCases
from .common import VectorSlices

def cmp(a, b):
	if a is None and b is None:
		return 0

	return (a > b) - (a < b)

def sign(i):
	return min(max(-1, i), +1)

def serialize_and_get(value):
	pointless.serialize(value, 'deleteme.map')

	try:
		return pointless.Pointless('deleteme.map').GetRoot()
	finally:
		os.unlink('deleteme.map')

class TestCmp(unittest.TestCase):
	def _testPythonCmp(self, values):
		for i, (v_a, v_b) in enumerate(itertools.product(values, values)):
			# test against python
			cmp_failure = False

			try:
				py_cmp = sign(cmp(v_a, v_b))
			except TypeError as e:
				if 'not supported between instances of ' in str(e):
					cmp_failure = True
				elif 'unorderable types' in str(e):
					cmp_failure = True
				else:
					raise

			p_a = serialize_and_get(v_a)
			p_b = serialize_and_get(v_b)

			if cmp_failure:
				self.assertRaises(TypeError, pointless.pointless_cmp, p_a, p_b)
				self.assertRaises(TypeError, pointless.pointless_cmp, p_a, v_b)
				self.assertRaises(TypeError, pointless.pointless_cmp, v_a, p_b)
				self.assertRaises(TypeError, pointless.pointless_cmp, v_a, v_b)
			else:
				p_cmp_a = sign(pointless.pointless_cmp(p_a, p_b))
				p_cmp_b = sign(pointless.pointless_cmp(p_a, v_b))
				p_cmp_c = sign(pointless.pointless_cmp(v_a, p_b))
				p_cmp_d = sign(pointless.pointless_cmp(v_a, v_b))

				self.assertEqual(p_cmp_a, p_cmp_b)
				self.assertEqual(p_cmp_a, p_cmp_c)
				self.assertEqual(p_cmp_a, p_cmp_d)

				# we're only interested in pointless/python compatibility for equality matcing
				self.assert_(not ((py_cmp == 0 or p_cmp_a == 0) and p_cmp_a != py_cmp))

			del p_a
			del p_b

	def _testTotalOrder(self, values):
		# we're testing, for each 3-combination of values
		for i, (v_a, v_b, v_c) in enumerate(itertools.product(values, values, values)):
			# print 'total order', i, len(values) ** 3
			# write the values out
			p_a = serialize_and_get(v_a)
			p_b = serialize_and_get(v_b)
			p_c = serialize_and_get(v_c)

			is_type_error_ab = False
			is_type_error_ac = False
			is_type_error_bc = False

			try:
				cmp(p_a, p_b)
			except TypeError:
				is_type_error_ab = True

			try:
				cmp(p_a, p_c)
			except TypeError:
				is_type_error_ac = True

			try:
				cmp(p_b, p_c)
			except TypeError:
				is_type_error_bc = True

			if not (is_type_error_ab or is_type_error_ac or is_type_error_bc):
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
			else:
				try:
					pointless.pointless_cmp(p_a, p_b)
					pointless.pointless_cmp(p_b, p_a)
					pointless.pointless_cmp(p_a, p_c)
					pointless.pointless_cmp(p_c, p_a)
					pointless.pointless_cmp(p_b, p_c)
					pointless.pointless_cmp(p_c, p_b)
					self.fail('some of these should have raised TypeError')
				except TypeError:
					pass

			del p_a
			del p_b
			del p_c

	NUMBERS        = [-1000, -100, 0, 100, 1.0, -1.0, 0.0, False, True]
	NONE           = [None]
	STRINGS        = ['hello world', u'Hello world', u'Hello world', '', u'']
	VECTORS        = [[0, 1, 2], [], [[]], ['asdf', None, True, False, [0, 1, 2, ['asdf']]], ['a', ['a']]]
	SLICED_VECTORS = [v_py for (v_py, v_po) in VectorSlices('slice.map', list(range(1000)))]
	BITVECTORS     = AllBitvectorTestCases()

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

	def testBitVectors(self):
		self._testTotalOrder(self.BITVECTORS)


#	# slooow
#	def testSlicedVectors(self):
#		self._testPythonCmp(self.SLICED_VECTORS)
#		self._testTotalOrder(self.SLICED_VECTORS)
#
#	def testAllCombinations(self):
#		# all in one soup, just to test total-order
#		values = self.NUMBERS + self.NONE + self.STRINGS + self.VECTORS + self.SLICED_VECTORS + self.BITVECTORS
#		random.shuffle(values)
#		self._testTotalOrder(values)
