#!/bin/bash

test -n "$(shopt -s nullglob; echo /opt/python/*pypy3*)"
is_mypy=$?

set -ex

for PYBIN in /opt/python/*/bin; do
	# pypy, but only pypy3
	if [ $is_mypy -eq 0 ]; then
		if [[ $PYBIN == *"pypy3"* ]]; then
			"${PYBIN}/pip" wheel /io/ -w wheelhouse/
		fi
	# cpython
	else
		if [[ $PYBIN =~ cp35|cp36|cp37|cp38 ]]; then
			"${PYBIN}/pip" wheel /io/ -w wheelhouse/
		fi
	fi
done

for whl in wheelhouse/*.whl; do
	auditwheel repair "$whl" --plat "$PLAT"
done

cp wheelhouse/*.whl /io/wheelhouse/
