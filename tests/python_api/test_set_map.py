#!/usr/bin/python

import pointless

from twisted.trial import unittest

class TestSetMap(unittest.TestCase):
	def testMapUnicode(self):
		v = {
			'abcdef': 1,
			u'abc': 2
		}

		fname = 'test_uncode_str_lookup.map'

		pointless.serialize(v, fname)
		vv = pointless.Pointless(fname).GetRoot()

		for a, b in v.items():
			self.assert_(a in vv)
			bb = vv[a]
			self.assertEquals(b, bb)

	def testEvilCases(self):
		for value, comparable in self.bad_case_iter():
			pointless.serialize(value, 'file.map')
			svalue = pointless.Pointless('file.map').GetRoot()

			if comparable:
				self.assertEquals(pointless.pointless_cmp(value, svalue), 0)

			s_a = str(value)
			s_b = str(svalue)

			# stringifaction of dicts/set does not have a consistent ordering
			if comparable:
				if not self._contains_dict_or_set(value):
					self.assertEquals(s_a, s_b)

	def bad_case_iter(self):
		value = { }
		value[1] = value
		yield value, False

		value = {
			(314,): []
		}
		value[(314,)].append(value)
		yield value, False

		value = {}
		yield value, False

		value = {
			():   [],
		}

		value[()].append(value)
		value[(1,)] = value[()]
		yield value, False

		value[(1,)].append(value[(1,)])
		yield value, False

		value = []
		value.append(value)
		yield value, False

	def testMap(self):
		maps = [
			{ 1: 0 },
			{ 2: 'asdf' },
			{ 'asdf':2 },
			{
				1: 2,
				2: 3,
				4: 4,
				'asdf': 123, 
				(1,2): [4,5,'hello world']
			}
		]

		maps[-1][(1,2)].append(maps)

		fname = 'test_map.map'

		for test_i, m in enumerate(maps):
			pointless.serialize(m, fname)
			root_a = pointless.Pointless(fname).GetRoot()

			for k in m:
				self.assert_(k in root_a)

				v_m = m[k]
				v_a = root_a[k]

				if not self._contains_dict_or_set(v_m):
					self.assert_(pointless.pointless_cmp(v_m, v_a) == 0)

			del root_a

	def _contains_dict_or_set(self, v):
		if isinstance(v, (tuple, list, pointless.PointlessVector)):
			return any(map(self._contains_dict_or_set, v))

		if isinstance(v, (dict, pointless.PointlessMap, set, pointless.PointlessSet)):
			return True

		return False


	def _list_to_tuple(self, v):
		return tuple(map(self._list_to_tuple, v)) if isinstance(v, list) else v

	def testSet(self):
		fname_a = 'test_set_contains_a.map'
		fname_b = 'test_set_contains_b.map'
		fname_c = 'test_set_contains_c.map'

		values = [
			[], [[]], [[], []],
			[[[], False], False, False, False],
			[0], ['hello world'],
			[0, u'hello'], ['hello world', (1, 2, 3)],
			[False, True, 1.0, -1.0, 0.0, 1, 200],
			[0, 1, 2, 3, 4],
			['a', 'b', 'c', ['d', 'e', 1.0]],
			[None, [None]],
			[list(range(10)), list(range(100)), list(range(1000)), [list(range(10)), ['asdf'] * 10]],
			[[0]], [[-1]], [[-1.0, 0.0]], [[0, False]], [[0, True]],
			[list(range(10, -100, -1)), [[list(range(10))]]]
		]

		# for each set V of values
		for v in values:
			# create set A out of it
			w = self._list_to_tuple(v)
			pointless.serialize(set(w), fname_a)
			root_a = pointless.Pointless(fname_a).GetRoot()

			# for each element in V
			for i in v:
				ii = self._list_to_tuple(i)

				# serialize it to value B
				pointless.serialize(i, fname_b)
				root_b = pointless.Pointless(fname_b).GetRoot()

				# serialize tuple'd value of it to C
				pointless.serialize(ii, fname_c)
				root_c = pointless.Pointless(fname_c).GetRoot()

				# element <- A, C <- A, B <- A, 
				self.assert_(ii in root_a)
				self.assert_(root_c in root_a)
				self.assert_(root_b in root_a)

				del root_b
				del root_c

			del root_a
