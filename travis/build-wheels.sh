#!/bin/bash

test -n "$(shopt -s nullglob; echo /opt/python/*pypy3*)"
is_mypy=$?

set -ex

mkdir -p ./wheels

_clean() {
	rm -rf /io/build
	rm -rf /io/dist
	rm -rf /io/judy-1.0.5/src/libJudy.a
}

for PYBIN in /opt/python/*/bin; do
	# pypy, but only pypy3
	if [ $is_mypy -eq 0 ]; then
		if [[ $PYBIN == *"pypy3"* ]]; then
			_clean;
			"${PYBIN}/pip" install -U pip
			"${PYBIN}/pip" wheel /io/ -w wheels/
		fi
	# cpython
	else
		if [[ $PYBIN =~ cp36|cp37|cp38|cp39 ]]; then
			_clean;
			"${PYBIN}/pip" install -U pip
			"${PYBIN}/pip" wheel /io/ -w wheels/
		fi
	fi
done

for whl in wheels/*.whl; do
	auditwheel repair "$whl" --plat "$PLAT" -w ./wheelhouse/
done

cp wheelhouse/*.whl /io/wheelhouse/
