#!/bin/bash

INCLUDE="-I../include -I../judy-1.0.5/src/"

#FLAGS="-pedantic -Wall -std=c99 -D_REENTRANT -D_GNU_SOURCE    -O2 -g -pg -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls"
#FLAGS="-pg -g -fno-omit-frame-pointer -O2 -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls -fno-inline"

FLAGS="-Wall -D_REENTRANT -D_GNU_SOURCE  -g  -O2"
SOURCE="../src/*.c ./c_api/*.c  ../judy-1.0.5/src/libJudy.a"

LDFLAGS="-lpthread -ldl"
#-liconv"

g++ -std=c++11 $FLAGS $INCLUDE $SOURCE $LDFLAGS
