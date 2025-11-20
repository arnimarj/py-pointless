#!/bin/bash

set -xe

mkdir -p ./wheelhouse

./build_judy.sh

pip install pipx
pipx run build --sdist --outdir ./wheelhouse .

# test it
pip install -U pip virtualenv
python -m virtualenv venv

./venv/bin/pip install ./wheelhouse/*.gz
