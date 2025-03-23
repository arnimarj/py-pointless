#!/bin/bash

bash --version

set -xe

basename="$(dirname "$0")"

if [[ ${CC} ]]; then
	JUDYCC="$CC"
else
	JUDYCC=gcc
fi

# adding the aliasing and loop-optimization flags cause of
# http://sourceforge.net/p/judy/mailman/message/32417284/
if [[ ! ${COPT} ]]; then
	COPT="-DJU_64BIT -O0 -fPIC -fno-strict-aliasing -Wall -D_REENTRANT -D_GNU_SOURCE"
fi


if $JUDYCC -v 2>&1 >/dev/null | grep -E "clang|gcc version 4.6"; then
	COPT+=""
else
	COPT+=" -fno-aggressive-loop-optimizations"
fi

(cd "$basename"; cd judy-1.0.5/src; CC="$JUDYCC" COPT="$COPT" sh ./sh_build)
