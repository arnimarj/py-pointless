#!/usr/bin/python

import sys, unittest

from test_serialize import TestSerialize
from test_primvector import TestPrimVector
from test_print import TestPrint
from test_cmp import TestCmp
from test_set_map import TestSetMap
from test_thread_safe import TestThreadSafe

def print_usage_exit():
	s =  'usage: ./main.py OPTIONS\n'
	s += '\n'
	s += '      --pointless-serialize\n'
	s += '      --prim-vector\n'
	s += '      --pointless-print\n'
	s += '      --pointless-cmp\n'
	s += '      --pointless-set-map\n'
	s += '      --test-thread-safe\n'
	sys.exit(s)

def main():
	if len(sys.argv) != 2:
		print_usage_exit()

	sub_suites = [ ]

	if sys.argv[1] == '--pointless-serialize':
		sub_suites.append(TestSerialize)
	elif sys.argv[1] == '--prim-vector':
		sub_suites.append(TestPrimVector)
	elif sys.argv[1] == '--pointless-print':
		sub_suites.append(TestPrint)
	elif sys.argv[1] == '--pointless-cmp':
		sub_suites.append(TestCmp)
	elif sys.argv[1] == '--pointless-set-map':
		sub_suites.append(TestSetMap)
	elif sys.argv[1] == '--test-thread-safe':
		sub_suites.append(TestThreadSafe)
	else:
		print_usage_exit()

	sub_suites = [unittest.TestLoader().loadTestsFromTestCase(ss) for ss in sub_suites]
	suite = unittest.TestSuite(sub_suites)
	unittest.TextTestRunner(verbosity = 2).run(suite)

if __name__ == '__main__':
	main()
