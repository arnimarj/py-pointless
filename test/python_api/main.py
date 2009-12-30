#!/usr/bin/python

import sys, unittest

from primvector import TestPrimVector

def print_usage_exit():
	s =  'usage: ./main.py OPTIONS\n'
	s += '\n'
	s += '      --prim-vector\n'
	sys.exit(s)

def main():
	if len(sys.argv) != 2:
		print_usage_exit()

	sub_suites = [ ]

	if sys.argv[1] == '--prim-vector':
		sub_suites.append(TestPrimVector)
	else:
		print_usage_exit()

	sub_suites = [unittest.TestLoader().loadTestsFromTestCase(ss) for ss in sub_suites]
	suite = unittest.TestSuite(sub_suites)
	unittest.TextTestRunner().run(suite)

if __name__ == '__main__':
	main()
