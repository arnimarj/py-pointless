#!/bin/bash

set -xe

mkdir -p ./wheelhouse

python setup.py sdist --dist-dir ./wheelhouse

# test it
pip install -U pip virtualenv
python -m virtualenv venv
./venv/bin/pip install ./wheelhouse/*.gz
