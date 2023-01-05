#!/usr/bin/python

import threading, random, pointless

import _thread as thread
from twisted.trial import unittest

class TestThreadSafe(unittest.TestCase):
	def testThreadOpenClose(self):
		n_files = 1
		n_threads = 32
		n_iter = 1000

		V = [None]
		V += range(10000)
		fnames = ['out.%i.map' % (i,) for i in range(n_files)]

		for fname in fnames:
			pointless.serialize(V, fname)

		p_list = [pointless.Pointless(fname) for i in range(n_files)]
		s_list = [threading.Event() for i in range(n_threads)]

		def thread_func(e):
			c = 0

			for i in range(n_iter):
				j = random.randint(0, len(p_list) - 1)
				c += len(p_list[j].GetRoot())

				if random.randint(0, 10) == 0:
					p_list[j] = pointless.Pointless(fnames[j])

			e.set()

		for i in range(n_threads):
			thread.start_new_thread(thread_func, (s_list[i],))

		for i in range(n_threads):
			s_list[i].wait()
