#!/usr/bin/python

import sys

def ImportPointlessExt():
	p = sys.path

	try:
		sys.path = ['/home/arni/py-pointless/trunk/build/lib.cygwin-1.7.1-i686-2.6/']
		import pointless as _pointless
	finally:
		sys.path = p

	return _pointless
