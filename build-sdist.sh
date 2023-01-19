#!/bin/bash

set -xe

mkdir -p ./wheelhouse

python3 -m build 

mv ./dist/*.gz ./wheelhouse/
