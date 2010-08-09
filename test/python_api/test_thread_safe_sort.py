#!/usr/bin/python

import random, time, sys

from twisted.internet import reactor, defer, threads
from twisted.python import failure
from twisted.trial import unittest

from common import pointless

class TestThreadSafe(unittest.TestCase):
	@defer.inlineCallbacks
	def testThreadedProjSort(self):
		N_DOMAIN = 1000
		N_ITEMS = 1000000
		N_SORTS = 32
		v = [ ]

		for i in xrange(10):
			v_ = pointless.PointlessPrimVector('u32', sequence = (random.randint(0, 100) for i in xrange(N_DOMAIN)), allow_print = False)
			v.append(v_)

		t = pointless.PointlessPrimVector('u32', sequence = (random.randint(0, N_DOMAIN - 1) for i in xrange(N_ITEMS)), allow_print = False)
		p = [ ]

		for i in xrange(N_SORTS):
			p_ = pointless.PointlessPrimVector('u32', sequence = t, allow_print = False)
			p.append(p_)

		THREADED = True
		d_list = [ ]

		for i in xrange(N_SORTS):
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

		for i in xrange(N_ITEMS):
			v = random.randint(0, 100000)
			p_.append(v)

		for i in xrange(N_SORTS):
			p.append(pointless.PointlessPrimVector('u32', sequence = p_, allow_print = False))

		d_list = []

		t_0 = time.time()

		def sort_wrapper(P):
			try:
				P.sort()
			except:
				f = failure.Failure()
				s = 'ERROR: %s\n' % (f.getErrorMessage(),)
				reactor.callFromThread(sys.stdout.write, s)
				reactor.callFromThread(sys.stdout.flush)

		THREAD = True

		for i in xrange(N_SORTS):
			if THREAD:
				d = threads.deferToThread(sort_wrapper, p[i])
				d_list.append(d)
			else:
				yield threads.deferToThread(sort_wrapper, p[i])

		if THREAD:
			yield defer.DeferredList(d_list)

		t_1 = time.time()
