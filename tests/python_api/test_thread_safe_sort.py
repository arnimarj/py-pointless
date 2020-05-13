#!/usr/bin/python

import random
import pointless

from twisted.internet import defer, threads
from twisted.trial import unittest

class TestThreadSafe(unittest.TestCase):
	@defer.inlineCallbacks
	def testThreadedProjSort(self):
		N_DOMAIN = 1000
		N_ITEMS = 1000000
		N_SORTS = 32
		v = [ ]

		for i in range(10):
			v_ = pointless.PointlessPrimVector('u32', sequence = (random.randint(0, 100) for i in range(N_DOMAIN)), allow_print = False)
			v.append(v_)

		t = pointless.PointlessPrimVector('u32', sequence = (random.randint(0, N_DOMAIN - 1) for i in range(N_ITEMS)), allow_print = False)
		p = [ ]

		for i in range(N_SORTS):
			p_ = pointless.PointlessPrimVector('u32', sequence = t, allow_print = False)
			p.append(p_)

		THREADED = True
		d_list = [ ]

		for i in range(N_SORTS):
			if THREADED:
				d = threads.deferToThread(p[i].sort_proj, *v)
				d_list.append(d)
			else:
				yield threads.deferToThread(p[i].sort_proj, *v)

		if THREADED:
			yield defer.DeferredList(d_list)

	@defer.inlineCallbacks
	def testThreadedSort(self):
		N_ITEMS = 100000
		N_SORTS = 32
		p = []
		p_ = pointless.PointlessPrimVector('u32', allow_print = False)

		for i in range(N_ITEMS):
			v = random.randint(0, 100000)
			p_.append(v)

		for i in range(N_SORTS):
			p.append(pointless.PointlessPrimVector('u32', sequence = p_, allow_print = False))

		d_list = []

		def sort_wrapper(P):
			P.sort()

		THREAD = True

		for i in range(N_SORTS):
			if THREAD:
				d = threads.deferToThread(sort_wrapper, p[i])
				d_list.append(d)
			else:
				yield threads.deferToThread(sort_wrapper, p[i])

		if THREAD:
			yield defer.DeferredList(d_list)
