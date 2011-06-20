#!/usr/bin/python

import sys, random

def ImportPointlessExt():
	p = sys.path

	try:
		# sys.path = ['/home/arni/py-pointless/trunk/build/lib.cygwin-1.7.1-i686-2.6/']
		sys.path = ['/home/arni/dohop/trunk/cpython/py-pointless/build/lib.linux-x86_64-2.7']
		import pointless as _pointless
	finally:
		sys.path = p

	return _pointless

pointless = ImportPointlessExt()

def VectorSlices(fname):
	py = range(10000)
	pointless.serialize(py, fname)
	po = pointless.Pointless(fname).GetRoot()

	vv = [ ]

	c = pointless.pointless_cmp(py, po)

	if c != 0:
		raise ValueError('initial comparison of VectorSlices() failed')

	vv.append((py, po))

	random.seed(0)

	for i in xrange(100):
		i_0 = random.randint(-100, 15000)
		i_1 = random.randint(-100, 15000)

		s_py = py[i_0:i_1]
		s_po = po[i_0:i_1]

		vv.append((s_py, s_po))

		for j in xrange(100):
			ii_0 = random.randint(-100, 6000)
			ii_1 = random.randint(-100, 6000)

			ss_py = s_py[ii_0:ii_1]
			ss_po = s_po[ii_0:ii_1]

			vv.append((ss_py, ss_po))

	return vv
