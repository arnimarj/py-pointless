#!/usr/bin/python

import pointless, random

def VectorSlices(fname, py_vector):
	pointless.serialize(py_vector, fname)
	po_vector = pointless.Pointless(fname).GetRoot()

	# first, the original/pointless vector
	yield (py_vector, po_vector)

	# then further slices of those two
	for i in range(100):
		i_0 = random.randint(-100, len(py_vector) * 2)
		i_1 = random.randint(-100, len(py_vector) * 2)

		s_py_vector = py_vector[i_0:i_1]
		s_po_vector = po_vector[i_0:i_1]

		yield s_py_vector, s_po_vector

		# and further slices of those
		for j in range(100):
			ii_0 = random.randint(-100, len(py_vector) // 2)
			ii_1 = random.randint(-100, len(py_vector) // 2)

			ss_py_vector = s_py_vector[ii_0:ii_1]
			ss_po_vector = s_po_vector[ii_0:ii_1]

			yield ss_py_vector, ss_po_vector
