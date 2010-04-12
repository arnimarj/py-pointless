#!/usr/bin/python

import unittest, threading, thread, random, sys

from common import pointless

class TestThreadSafe(unittest.TestCase):
	def testThreadOpenClose(self):
		n_files = 1
		n_threads = 32
		n_iter = 1000

		V = [None]
		V += range(10000)
		fnames = ['out.%i.map' % (i,) for i in xrange(n_files)]

		for fname in fnames:
			pointless.serialize(V, fname)

		p_list = [pointless.Pointless(fname) for i in xrange(n_files)]
		s_list = [threading.Event() for i in xrange(n_threads)]

		def thread_func(e):
			c = 0

			for i in xrange(n_iter):
				j = random.randint(0, len(p_list) - 1)
				c += len(p_list[j].GetRoot())

				if random.randint(0, 10) == 0:
					p_list[j] = pointless.Pointless(fnames[j])

				if i > 0 and i % 1000 == 0:
					sys.stdout.write('.')
					sys.stdout.flush()

			e.set()

		for i in xrange(n_threads):
			thread.start_new_thread(thread_func, (s_list[i],))

		for i in xrange(n_threads):
			s_list[i].wait()
