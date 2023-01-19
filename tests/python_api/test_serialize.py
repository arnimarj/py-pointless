#!/usr/bin/python

import random, pointless

from twisted.trial import unittest

def SimpleSerializeTestCases():
	# 1) deep vector
	yield [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]

	# 2) vector of vectors
	yield [list(range(10 + 1)) for i in range(10)]

	# 3) randomized bitvector
	random.seed(0)
	v = pointless.PointlessBitvector(100)

	for i in range(40):
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
	for bv in AllBitvectorTestCases():
		yield bv

# all cases for bitvectors
def AllBitvectorTestCases():
	return [
		# case 0) all-0
		pointless.PointlessBitvector(sequence = [0] * 1000),

		# case 1) all-1
		pointless.PointlessBitvector(sequence = [1] * 1000),

		# case 2) 0-1
		pointless.PointlessBitvector(sequence = [0] * 400 + [1] * 800),

		# case 3) 1-0
		pointless.PointlessBitvector(sequence = [1] * 400 + [0] * 800),

		# case 4) less than 27 bits
		pointless.PointlessBitvector(sequence = [0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1]),

		# case 5) a whole bunch of bits
		pointless.PointlessBitvector(sequence = [0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1])
	]

class TestSerialize(unittest.TestCase):
	def testSerialize(self):
		fname = 'test_serialize.map'

		for v in SimpleSerializeTestCases():
			pointless.serialize(v, fname)
			root = pointless.Pointless(fname).GetRoot()
			del root

	def testString(self):
		fname = 'test_string.map'
		# TBD: this test fails on pypy
		v = ['string', 'value', chr(1000)]
		pointless.serialize(v, fname)
		p = pointless.Pointless(fname)
		a, b, c = p.GetRoot()
		self.assertEqual(v[0], a)
		self.assertEqual(v[1], b)
		self.assertEqual(v[2], c)
