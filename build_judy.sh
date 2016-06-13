#!/bin/bash

if [[ -v CXX ]]; then
	JUDYCC=$CXX
elif [[ -v CC ]]; then
	JUDYCC=$CC
else
	JUDYCC=CC
fi

echo CC $JUDYCC

(cd judy-1.0.5/src; CC=$JUDYCC COPT='-DJU_64BIT -O0 -fPIC -fno-strict-aliasing -fno-aggressive-loop-optimizations' sh ./sh_build)
