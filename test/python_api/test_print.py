#!/usr/bin/python

import cStringIO, unittest, types

from test_serialize import SimpleSerializeTestCases
from common import pointless

def PythonPrint(pointless, p, out):
	def pointless_print_rec(v, stack):
		if v == None:
			out.write('None')
		elif isinstance(v, types.BooleanType):
			out.write('True' if v == True else 'False')
		elif isinstance(v, pointless.PointlessBitvector):
			out.write('')

			for i in xrange(len(v) - 1, -1, -1):
				b = v[i]
				assert(isinstance(b, types.BooleanType))
				out.write('1' if b else '0')

			out.write('b')
		elif isinstance(v, types.UnicodeType):
			out.write(repr(v))
		elif isinstance(v, types.FloatType):
			out.write('%f' % (v,))
		elif isinstance(v, (types.IntType, types.LongType)):
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
						pointless_print_rec(item, stack)
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
					pointless_print_rec(item, stack)
					stack.pop()

					if i + 1 < len(v):
						out.write(', ')

			out.write('])')
		else:
			out.write('<TYPE: %s>' % (type(v),))

	pointless_print_rec(p, [])

class TestPrint(unittest.TestCase):
	def testPrint(self):
		fname = 'test_print.map'

		# for each simple test case
		for v in SimpleSerializeTestCases():
			# serialize it
			pointless.serialize(v, fname)
			# load it
			p = pointless.Pointless(fname)
			root = p.GetRoot()

			# do a Python version of print
			s_python = cStringIO.StringIO()
			PythonPrint(pointless, root, s_python)

			# and the Pointless version
			s_root = cStringIO.StringIO()
			print >> s_root, root,

			self.assertEqual(s_root.getvalue(), s_python.getvalue())

			del root
			del p
