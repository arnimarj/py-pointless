#!/bin/bash

if [ -z CC ]; then
	JUDYCC=$CC
else
	JUDYCC=cc
fi

COPT="-DJU_64BIT -O0 -fPIC -fno-strict-aliasing"

if $JUDYCC -v 2>&1 >/dev/null | egrep "clang|gcc version 4.6"; then
	COPT+=""
else
	COPT+=" -fno-aggressive-loop-optimizations"
fi

(cd judy-1.0.5/src; CC=$JUDYCC COPT="$COPT" sh ./sh_build)
