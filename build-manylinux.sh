#!/bin/bash

set -xe

mkdir -p ./wheelhouse

docker run --rm -e PLAT=manylinux2010_x86_64 -v "$(pwd):/io" quay.io/pypa/manylinux2010_x86_64    /io/travis/build-wheels.sh
docker run --rm -e PLAT=manylinux2014_x86_64 -v "$(pwd):/io" quay.io/pypa/manylinux2014_x86_64    /io/travis/build-wheels.sh

docker run --rm -e PLAT=manylinux2010_x86_64 -v "$(pwd):/io" pypywheels/manylinux2010-pypy_x86_64 /io/travis/build-wheels.sh

ls -l ./wheelhouse

# make sure we clean up since all this gets run as root
sudo rm -rf ./build/ ./pointless.egg-info/ ./dist/
