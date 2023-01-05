#!/usr/bin/python

import itertools, pointless, io

from twisted.trial import unittest

from .test_serialize import SimpleSerializeTestCases, AllBitvectorTestCases

# generic print implementation, should match C implementation
def PythonPrint(v, out, stack=None):
	if stack is None:
		stack = []

	if v is None:
		out.write('None')
	elif isinstance(v, bool):
		out.write('True' if v is True else 'False')
	elif isinstance(v, pointless.PointlessBitvector):
		out.write('')

		for i in range(len(v) - 1, -1, -1):
			b = v[i]
			assert(isinstance(b, bool))
			out.write('1' if b else '0')

		out.write('b')
	elif isinstance(v, float):
		out.write('%f' % (v,))
	elif isinstance(v, int):
		out.write('%i' % (v,))
	elif isinstance(v, pointless.PointlessVector):
		out.write('[')

		if len(v) > 0:
			if v.container_id in stack:
				out.write('...')
			else:
				for i, item in enumerate(v):
					assert(v[i] == item)
					stack.append(v.container_id)
					PythonPrint(item, out, stack)
					stack.pop()

					if i + 1 < len(v):
						out.write(', ')

		out.write(']')
	elif isinstance(v, pointless.PointlessSet):
		out.write('set([')

		if v.container_id in stack:
			out.write('...')
		else:
			for i, item in enumerate(v):
				stack.append(v.container_id)
				PythonPrint(item, out, stack)
				stack.pop()

				if i + 1 < len(v):
					out.write(', ')

		out.write('])')
	else:
		print(type(v))
		out.write('<TYPE: %s>' % (type(v),))

class TestPrint(unittest.TestCase):
	def testPrint(self):
		fname = 'test_print.map'

		# for each simple test case
		for v in itertools.chain(SimpleSerializeTestCases(), AllBitvectorTestCases()):

			# serialize it
			pointless.serialize(v, fname)

			# load it
			root = pointless.Pointless(fname).GetRoot()

			# do a Python version of print
			s_python = io.StringIO()
			PythonPrint(root, s_python)

			# and the Pointless version
			s_root = io.StringIO()
			s_root.write(str(root))

			self.assertEqual(s_root.getvalue(), s_python.getvalue())

			del root
